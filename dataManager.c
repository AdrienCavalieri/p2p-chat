#include "dataManager.h"

#include "neighbour.h"
#include "neighbourManager.h"
#include "message.h"
#include "idGenerator.h"
#include "inputReader.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#define MAX_RECEIVED 16
#define MAX_SEND 4

static short debug = 0;

struct symmetric_neighbour_list {
    struct neighbour* neighbour;
    short received;
    int send_count;
    struct symmetric_neighbour_list* next;
};

struct received_data {
    uint64_t id;
    uint32_t nonce;
    uint8_t type;
    uint8_t* data;
    size_t data_len;
    struct symmetric_neighbour_list* sym_list;
    struct received_data* next;
};

static struct received_data* head = NULL;
static struct received_data* end = NULL;
static int receivedCount = 0;

static int my_nonce_count = 0;

static pthread_mutex_t syms_mutex = PTHREAD_MUTEX_INITIALIZER;

/*******************/
/*       Lock      */
/*******************/

static void lock(const char* func_name) {
    if( pthread_mutex_lock(&syms_mutex) != 0 ) {
        perror(func_name);
        exit(EXIT_FAILURE);
    }
}

static void unlock(const char* func_name) {
    if( pthread_mutex_unlock(&syms_mutex) != 0 ) {
        perror(func_name);
        exit(EXIT_FAILURE);
    }
}

/******************/
/*  Constructeur  */
/******************/

struct received_data* create_received_data(uint64_t id, uint32_t nonce, uint8_t type, const uint8_t* data, size_t data_len) {
    struct received_data* rd = malloc(sizeof(struct received_data));
    if(rd == NULL) {
        fprintf(stderr, "malloc() failed.");
        exit(1);
    }

    rd->id = id;
    rd->nonce = nonce;
    rd->type = type;
        
    rd->data = malloc( sizeof(uint8_t)*data_len );
    memmove(rd->data, data, data_len);
    if(rd->data == NULL) {
        fprintf(stderr, "malloc() failed.");
        exit(1);
    }
    rd->data_len = data_len;
    
    rd->sym_list = NULL;
    rd->next = NULL;
    return rd;
}

static struct symmetric_neighbour_list* create_sym_list(struct neighbour* n) {
    struct symmetric_neighbour_list* l = malloc(sizeof(struct symmetric_neighbour_list));
    if(l == NULL) {
        fprintf(stderr, "malloc() failed.");
        exit(1);
    }

    l->neighbour = n;
    l->received = 0;
    l->send_count = 0;
    l->next = NULL;
    return l;
}

/*****************/
/*  Destructeur  */
/*****************/

static void destroy_sym_list_cell(struct symmetric_neighbour_list* cell) {
    free(cell);
}

static void destroy_sym_list(struct symmetric_neighbour_list* l_head) {
    if(l_head != NULL) {
        destroy_sym_list(l_head->next);
        destroy_sym_list_cell(l_head);
    }    
}

void destroy_received_data(struct received_data* rd) {
    free(rd->data);
    destroy_sym_list(rd->sym_list);
    free(rd);
}

/*******************/
/* Getters/Setters */
/*******************/

struct received_data* get_received_data(uint64_t id, uint32_t nonce) {

    if( head == NULL || (head->id == id && head->nonce == nonce) )
        return head;
    
    struct received_data* aux;
    for(aux = head->next; aux != head; aux = aux->next)
        if( aux->id == id && aux->nonce == nonce )
            return aux;

    return NULL;
}

void received(struct received_data* rd, struct neighbour* n) {
    lock("received");

    struct symmetric_neighbour_list* aux;
    
    for(aux = rd->sym_list; aux != NULL; aux = aux->next)
        if( equals_neighbours(n, aux->neighbour) ) {
            aux->received = 1;
            break;
        }
    
    unlock("received");
}

static short is_received(struct symmetric_neighbour_list* l_cell) {
    short res;
    lock("is_received");
    res = l_cell->received;
    unlock("is_received");
    return res;
}

/*******************/
/*   Comparator    */
/*******************/

short equals_data(struct received_data* d1, struct received_data* d2) {
    return d1->id == d2->id && d1->nonce == d2->nonce;
}

/******************/
/*      Ajout     */
/******************/

void add_symmetric(struct received_data* rd, struct neighbour* n) {
    struct symmetric_neighbour_list* l = create_sym_list(n);
    
    l->next = rd->sym_list;
    rd->sym_list = l;
}

short add_received_data(struct received_data* rd) {
    assert(rd != NULL);
    struct received_data* aux;

    // Si on a déjà la donnée, on ne l'ajoute pas.
    if(head != NULL) {
        if( equals_data(head, rd) )
            return 0;
        
        aux = head;
        while( aux != head ) {
            if( equals_data(head, rd) )
                return 0;
            aux = aux->next;
        }
    }
        
    receivedCount++;
    
    if(head == NULL && end == NULL) {
        head = rd;
        end = rd;
        rd->next = rd;
        return 1;
    }
    
    if(head != NULL && end != NULL) {
        rd->next = head;
        end->next = rd;
        head = rd;

        if(receivedCount == MAX_RECEIVED+1) {
            aux = head;
            while(aux->next != end && aux->next != head)
                aux = aux->next;

            if(aux->next == head)
                assert(0);
            
            aux->next = head;
            destroy_received_data(end);
            end = aux;
        }
        return 1;
    }

    receivedCount--;
    assert(0);
}

void add_my_data(const uint8_t* d, size_t len) {
    struct received_data* rd = create_received_data(get_my_id(), my_nonce_count++, 0, d, len);
    init_symeterics(rd);
    add_received_data(rd);
    inondation(rd);
}

/*******************/
/*   Suppression   */
/*******************/

void remove_symmetric(struct received_data* rd, uint64_t id) {
    struct symmetric_neighbour_list *aux, *tmp;
    aux = rd->sym_list;

    if(aux == NULL)
        return;
    
    if( get_id(aux->neighbour) == id ) {
        rd->sym_list = aux->next;
        destroy_sym_list_cell(aux);
        aux = NULL;
        return;
    }
    
    while( aux->next != NULL && get_id(aux->next->neighbour) != id)
        aux = aux->next;

    if( aux->next == NULL )
        return;
    
    tmp = aux->next;
    aux->next = aux->next->next;
    destroy_sym_list_cell(tmp);
    tmp = NULL;
}


/*******************/
/*   Innondation   */
/*******************/


static void* inondation_t(void* d) {
    struct received_data* rd = (struct received_data*)d;
    struct sockaddr_in6 sockaddr;

    struct msg* data = create_msg();
    add_data_tlv(data, rd->id, rd->nonce, rd->type, rd->data, rd->data_len);

    struct msg* goAway = create_msg();
    char* error = "You are too slow or inactive.";
    add_goAway_tlv(goAway, 2, (uint8_t*)error, strlen(error)-1);

    int min, max;

    struct symmetric_neighbour_list *aux;

    while(rd->sym_list != NULL) {
        for(aux = rd->sym_list; aux != NULL; aux = aux->next) {

            if( is_received(aux) ) {
                remove_symmetric( rd, get_id(aux->neighbour) );
                break;
            }
            
            get_sockaddr6(aux->neighbour, &sockaddr);
        
            if(aux->send_count > MAX_SEND) {
                send_msg(goAway, (struct sockaddr*)&sockaddr, sizeof(sockaddr));
                
                remove_from_neighbours(aux->neighbour);
                add_potential_neighbour(aux->neighbour);
                remove_symmetric( rd, get_id(aux->neighbour) );
                break;
                
            } else {
                
                min = (int)pow(2, aux->send_count-1);
                max = (int)pow(2, aux->send_count);
                sleep( (random() % (max - min)) + min );
                send_msg(data, (struct sockaddr*)&sockaddr, sizeof(sockaddr));
                aux->send_count++;
                
            }
            
        }
    }
    
    destroy_msg(data);
    destroy_msg(goAway);
    
    return NULL;
}
                           

void inondation(struct received_data* rd) {

    if(debug)
        printn("Inondation: start.");

    pthread_t thread;

    int rc = pthread_create( &thread, NULL, inondation_t, rd);
    if( rc != 0) {
        if( rc == EAGAIN ) {
            sleep(5);
            inondation(rd);
            return;
        }
        fprintf(stderr, "Error: Thread create\n");
        exit(EXIT_FAILURE);
    }

    
}
