#include "neighbourManager.h"

#include "message.h"
#include "idGenerator.h"
#include "dataManager.h"
#include "inputReader.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#include <arpa/inet.h>

#define MIN_SYM 8
#define LONG_HELLO_INTERVAL 20

static short debug = 0;

// Envoie un Hello court si pas assez de voisins symétriques (par exple moins de 8)
// A la reception d'un TLV Neighbour, ajoute le pair dans la liste de voisins potentiels
struct neighbour_cell {
    struct neighbour* neighbour;
    struct neighbour_cell* next;
};

static struct neighbour_cell* potential_nl_head = NULL;
static struct neighbour_cell* nl_head = NULL;

static long last_hello_sending = 0;
static long last_neighbour_sending = 0;

static pthread_mutex_t nei_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t pnei_mutex = PTHREAD_MUTEX_INITIALIZER;

/*******************/
/*  Constructeurs  */
/*******************/

static struct neighbour_cell* create_neighbour_cell(struct neighbour* n) {
    struct neighbour_cell* nl = malloc(sizeof(struct neighbour_cell));
    if(nl == NULL) {
        fprintf(stderr, "malloc() failed.");
        exit(1);
    }

    nl->neighbour = n;
    nl->next = NULL;
    return nl;
}

/*******************/
/*   Destructeurs  */
/*******************/

static void destroy_neighbour_cell(struct neighbour_cell* nc) {
    if(nc != NULL)
        free(nc);
}

/*******************/
/*       Lock      */
/*******************/

static void lock(pthread_mutex_t* mutex, const char* func_name) {
    if( pthread_mutex_lock(mutex) != 0 ) {
        perror(func_name);
        exit(EXIT_FAILURE);
    }
}

static void unlock(pthread_mutex_t* mutex, const char* func_name) {
    if( pthread_mutex_unlock(mutex) != 0 ) {
        perror(func_name);
        exit(EXIT_FAILURE);
    }
}

/*******************/
/*     Getters     */
/*******************/

struct neighbour* get_neighbour(uint128_t ip, uint16_t port) {
    lock(&nei_mutex, "get_neighbour");
    struct neighbour_cell* aux;
    for(aux = nl_head; aux != NULL; aux = aux->next) {
        if( get_ip(aux->neighbour) == ip && get_port(aux->neighbour) == port ) {
            unlock(&nei_mutex, "get_neighbour");
            return aux->neighbour;
        }
    }

    unlock(&nei_mutex, "get_neighbour");
    return NULL;
}

short is_neighbour(struct neighbour* n) {
    lock(&nei_mutex, "is_neighbour");
    struct neighbour_cell* aux;
    for(aux = nl_head; aux != NULL; aux = aux->next) {
        if( equals_neighbours(aux->neighbour, n) ) {
            unlock(&nei_mutex, "is_neighbour");
            return 1;
        }
    }

    unlock(&nei_mutex, "is_neighbour");
    return 0;
}

/*******************/
/*      Ajouts     */
/*******************/

// return la tete de la liste
static struct neighbour_cell* add_to_list(struct neighbour_cell* head, struct neighbour* n) {
    
    // Si la liste est vide, on l'ajoute en tête.
    if(head == NULL)
        return  create_neighbour_cell(n);
    
    struct neighbour_cell* aux;
    for(aux = head; aux != NULL; aux = aux->next ) {
        // Si il est déjà dans la liste, on s'arrette.
        if( equals_neighbours(aux->neighbour, n) )
            return head;
    }
    
    // Si le pair n'est pas dans la liste, on met l'ajoute en tête.
    struct neighbour_cell* nc = create_neighbour_cell(n);
    nc->next = head;

    return nc; 
}

short add_neighbour(struct neighbour* n) {
    lock(&nei_mutex, "add_neighbour");
    
    nl_head = add_to_list(nl_head, n);

    unlock(&nei_mutex, "add_neighbour");
    
    if(debug && n == nl_head->neighbour) {
            uint128_t nip = get_ip(n);
            char str[INET6_ADDRSTRLEN];
            inet_ntop( AF_INET6, &nip, str, INET6_ADDRSTRLEN );
            printn("Ajout du voisin %s, %d)", str, ntohs(get_port(n)) );
    }
    
    return n == nl_head->neighbour;
}

short add_potential_neighbour(struct neighbour* n) {
    
    lock(&pnei_mutex, "add_neighbour");

    potential_nl_head = add_to_list(potential_nl_head, n);

    unlock(&pnei_mutex, "add_potential_neighbour");
    
    if(debug && n == potential_nl_head->neighbour) {
        uint128_t nip = get_ip(n);
        char str[INET6_ADDRSTRLEN];
        inet_ntop( AF_INET6, &nip, str, INET6_ADDRSTRLEN );
        printn("Ajout du voisin potentiel %s, %d)", str, ntohs(get_port(n)) );
    }
        
    return n == potential_nl_head->neighbour;    
}

void init_symeterics(struct received_data* rd) {
    lock(&nei_mutex, "init_symetrics");
    
    struct neighbour_cell* aux;
    for(aux = nl_head; aux != NULL; aux = aux->next)
        if( is_symmetric(aux->neighbour) )
            add_symmetric( rd, aux->neighbour );

    unlock(&nei_mutex, "init_symetrics");
}

/*******************/
/*   Suppression   */
/*******************/

static struct neighbour_cell* remove_from_list(struct neighbour* n, struct neighbour_cell* head) {
    
    struct neighbour_cell* nc;

    if(head == NULL)
        return NULL;
    
    if( equals_neighbours(head->neighbour, n) ) {
        nc = head->next;
        destroy_neighbour_cell(head);
        return nc;
    }

    struct neighbour_cell* tmp;
    
    for(nc = head; nc->next != NULL; nc = nc->next) {
        if( equals_neighbours(nc->next->neighbour, n) ) {
            tmp = nc->next->next;
            destroy_neighbour_cell(nc->next);
            nc->next = tmp;
            return head;
        }
    }

    return head;
}

void remove_from_potentials(struct neighbour* n) {
    lock(&pnei_mutex, "remove_potentials");

    potential_nl_head = remove_from_list(n, potential_nl_head);

    unlock(&pnei_mutex, "remove_potentials");
    
    if(debug) {
        uint128_t nip = get_ip(n);
        char str[INET6_ADDRSTRLEN];
        inet_ntop( AF_INET6, &nip, str, INET6_ADDRSTRLEN );
        printn("Suppression du voisin potentiel %s, %d)", str, ntohs(get_port(n)) );
    }
}

void remove_from_neighbours(struct neighbour* n) {
    lock(&nei_mutex, "remove_neighbour");

    nl_head = remove_from_list(n, nl_head);
 
    unlock(&nei_mutex, "remove_neighbour");

    if(debug) {
        uint128_t nip = get_ip(n);
        char str[INET6_ADDRSTRLEN];
        inet_ntop( AF_INET6, &nip, str, INET6_ADDRSTRLEN );
        printn("Suppression du voisin %s, %d)", str, ntohs(get_port(n)) );
    }
}

/*******************/
/*      Autres     */
/*******************/

void symetrics_maintenance(int socket, uint64_t my_id) {

    if( time(NULL) - last_hello_sending >= LONG_HELLO_INTERVAL/2 ) {

        struct neighbour_cell* n;
        int symetrics_count = 0;
        // On compte le nombre de voisins symetriques.
        for( n = nl_head; n != NULL; n = n->next )
            if( is_symmetric(n->neighbour) )
                symetrics_count++;
        
        // Si on a moins de MIN_SYM voisins symetriques, on envoie des hello court
        // aux voisins potentiels jusqu'à atteindre MIN_SYM ou la fin de la liste.
        if( symetrics_count < MIN_SYM ) {
            struct sockaddr_in6 dest;
            struct msg* hello = create_msg();
            
            add_hello_short_tlv(hello, get_my_id());
            
            if(debug) printn("Commence l'envoie de HELLO COURT à tous les voisins potentiels");
            
            for( n = potential_nl_head; n != NULL; n = n->next ) {
                get_sockaddr6(n->neighbour, &dest);
                send_msg(hello, (struct sockaddr*)&dest, sizeof(dest));
            }
            
            if(debug) printn("Envoie de HELLO COURT terminé.");
            
        }
    }
}

static void send_neighbours() {
    if( time(NULL) - last_hello_sending >= LONG_HELLO_INTERVAL*4 ) {

        if(debug) printn("Commence l'envoie de NEIGHBOUR à tous les voisins");
        
        last_hello_sending = time(NULL);

        struct msg* hello_nei;
        struct sockaddr_in6 dest;
        struct neighbour_cell *n, *n2;
    

        // Pour tous les voisins, on envoie nos autres voisins symétriques
        for(n = nl_head; n != NULL; n = n->next) {
            hello_nei = create_msg();
            add_hello_long_tlv(hello_nei, get_my_id(), get_id(n->neighbour));

            // On remplit le message avec nos voisins symétriques
            for(n2 = nl_head; n2 != NULL; n2 = n2->next) {
                if( n != n2 && is_symmetric(n2->neighbour) ) {
                    add_neighbour_tlv(hello_nei,
                                      get_ip(n2->neighbour),
                                      get_port(n2->neighbour));
                }
            }
            // On envoie le tlv.
            get_sockaddr6(n->neighbour, &dest);
            send_msg(hello_nei, (struct sockaddr*)&dest, sizeof(dest));
            destroy_msg(hello_nei);
            hello_nei = NULL;
        }

        if(debug) printn("Envoie de NEIGHBOUR terminé.");
    }

}

void start_hello_sender() {

    if( time(NULL) - last_neighbour_sending >= LONG_HELLO_INTERVAL ) {

        if(debug) printn("Commence l'envoie de HELLO LONG à tous les voisins");
        
        last_neighbour_sending = time(NULL);

        struct msg* hello;
        struct sockaddr_in6 dest;
        struct neighbour_cell* n;
    
        for(n = nl_head; n != NULL; n = n->next) {
            hello = create_msg();
            add_hello_long_tlv(hello, get_my_id(), get_id(n->neighbour));
            get_sockaddr6(n->neighbour, &dest);
            send_msg(hello, (struct sockaddr*)&dest, sizeof(dest));
            destroy_msg(hello);
            hello = NULL;
        }

        if(debug) printn("Envoie de HELLO LONG terminé.");
    }

    send_neighbours();
}
