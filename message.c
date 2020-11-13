#include "message.h"
#include "tlv.h"
#include "info.h"
#include "inputReader.h"
#include "neighbourManager.h"

#include <assert.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#define MAGIC 93
#define VERSION 2
#define MAX_RECEIVED 4096

static int debug = 0;

struct tlv_list {
    struct tlv* tlv;
    struct tlv_list* next;
};

struct msg {
    uint8_t magic;
	uint8_t version;
	uint16_t body_length;
    struct tlv_list* first_tlv;
};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


/*******************/
/*       Lock      */
/*******************/

static void lock(const char* func_name) {
    if( pthread_mutex_lock(&mutex) != 0 ) {
        perror(func_name);
        exit(EXIT_FAILURE);
    }
}

static void unlock(const char* func_name) {
    if( pthread_mutex_unlock(&mutex) != 0 ) {
        perror(func_name);
        exit(EXIT_FAILURE);
    }
}

/****************************/
/*        Constructors      */
/****************************/

static struct tlv_list* create_tlv_list(struct tlv* t) {
    struct tlv_list* tlvl = malloc(sizeof(struct tlv_list));
    if(tlvl == NULL) {
        perror("create_tlv_list: malloc() failed.");
        exit(EXIT_FAILURE);
    }
    tlvl->tlv = t;
    tlvl->next = NULL;
    return tlvl;
}

struct msg* create_msg() {
    struct msg* m = malloc(sizeof(struct msg));
    if(m == NULL) {
        perror("create_msg: malloc() failed.");
        exit(EXIT_FAILURE);
    }
    m->magic = MAGIC;
    m->version = VERSION;
    m->body_length = 0;
    m->first_tlv = NULL;
    return m;
}

struct msg* data_to_msg(uint8_t* data, size_t len) {
    if(len < 4 || data[0] != MAGIC || data[1] != VERSION || ntohs(((uint16_t*)data)[1]) != len-4 ) {
        
        if(debug) {
            if(debug) printn("Message incorrect: ");
            if(len < 4) printn("len < 4 (%ld)", len);
            if(data[0] != MAGIC) printn("data[0] != MAGIC (%d != %d)", data[0], MAGIC);
            if(data[1] != VERSION) printn("data[1] != VERSION (%d != %d)", data[1], VERSION);
            if(ntohs(((uint16_t*)data)[1]) != len-4) printn("ntohs(((uint16_t*)data)[1]) != len-4 (%d != %ld)", ((uint16_t*)data)[1], len-4);
        }
        
        return NULL;
    }

    struct msg* m = create_msg();
    uint8_t* ptr = data+4;

    uint8_t dlen;

    while( (ptr-data) < len) {
        switch( *ptr ) {
        case 0:
            ptr++;
            break;

        case 1:
            add_padn_tlv(m, ptr[1]);
            ptr += 2 + ptr[1];
            break;

        case 2:
            if(ptr[1] == 8) {
                ptr += 2;
                add_hello_short_tlv(m, ((uint64_t*)ptr)[0] );
                ptr += 8;
            } else {
                ptr += 2;
                add_hello_long_tlv(m, ((uint64_t*)ptr)[0], ((uint64_t*)ptr)[1] );
                ptr += 16;
            }
            break;  

        case 3:
            ptr += 2;
            add_neighbour_tlv(m, ((uint128_t*)ptr)[0], ((uint16_t*)ptr)[8]);
            ptr += 18;
            break;

        case 4:
            dlen = ptr[1];
            ptr +=2;
            add_data_tlv(m, ((uint64_t*)ptr)[0], ((uint32_t*)ptr)[2], ptr[12], ptr+13, dlen-13);
            ptr += dlen;
            break;

        case 5:
            ptr +=2;
            add_ack_tlv(m, ((uint64_t*)ptr)[0], ((uint32_t*)ptr)[2]);
            ptr += 12;
            break;

        case 6:
            dlen = ptr[1];
            ptr +=2;
            add_goAway_tlv(m, ptr[0], ptr+1, dlen-1);
            ptr += dlen;
            break;

        case 7:
            dlen = ptr[1];
            ptr +=2;
            add_warning_tlv(m, ptr, dlen);
            ptr += dlen;
            break;
            
        default:
            fprintn(stderr, "[TLV] Type invalide: %d.", *ptr);
            destroy_msg(m);
            return NULL;
        }
    }

    return m;
}

/****************************/
/*        Destructors       */
/****************************/

static void destroy_tlv_list(struct tlv_list* tlvl) {
    if( tlvl != NULL) {
        destroy_tlv_list(tlvl->next);
        destroy_tlv(tlvl->tlv);
        free(tlvl);
    }
}

void destroy_msg(struct msg* m) {
    if(m != NULL) {
        destroy_tlv_list(m->first_tlv);
        free(m);
    }
}

/****************************/
/*          Add TLV         */
/****************************/

static void add_tlv(struct msg* m, struct tlv* t) {
    assert(t != NULL);
    
    m->body_length += get_tlv_length(t);
    struct tlv_list* tlvl = create_tlv_list(t);
    
    if(m->first_tlv == NULL) {
        m->first_tlv = tlvl;
        return;
    }

    struct tlv_list* tmp = m->first_tlv;
    while(tmp->next != NULL)
        tmp = tmp->next;

    tmp->next = tlvl;
}

void add_padn_tlv(struct msg* m, uint8_t len) {
    add_tlv(m, create_padn_tlv(len));
}

void add_hello_short_tlv(struct msg* m, uint64_t id) {
    add_tlv(m, create_hello_short_tlv(id));
}

void add_hello_long_tlv(struct msg* m, uint64_t source_id, uint64_t dest_id) {
    add_tlv(m, create_hello_long_tlv(source_id, dest_id));
}

void add_neighbour_tlv(struct msg* m, uint128_t ip, uint16_t port) {
    add_tlv(m, create_neighbour_tlv(ip, port));
}

void add_data_tlv(struct msg* m, uint64_t sender_id, uint32_t nonce, uint8_t type, uint8_t* data, size_t data_len) {
    add_tlv(m, create_data_tlv( sender_id, nonce, type, data, data_len));
}

void add_ack_tlv(struct msg* m, uint64_t sender_id, uint32_t nonce) {
    add_tlv(m, create_ack_tlv(sender_id, nonce));
}

void add_goAway_tlv(struct msg* m, uint8_t code, uint8_t* message, size_t message_len) {
    add_tlv(m, create_goAway_tlv(code, message, message_len));
}

void add_warning_tlv(struct msg* m, uint8_t* message, size_t message_len) {
    add_tlv(m, create_warning_tlv(message, message_len));
}


/***************************/
/*           Send          */
/***************************/

short send_msg(struct msg* m, struct sockaddr *dest, size_t dest_len) {
    
    int req_size = m->body_length + 4;
    uint8_t req[req_size];
    req[0] = m->magic;
    req[1] = m->version;
    ((uint16_t*)req)[1] = htons(m->body_length);

    int pos = 4;
    struct tlv_list* tlvl = m->first_tlv;
    while(tlvl != NULL) {
        tlv_to_data(tlvl->tlv, req+pos);
        pos += get_tlv_length(tlvl->tlv);
        tlvl = tlvl->next;
    }

    lock("send_msg");

    int rc;
    int to = 5;
    fd_set writefds;
    int s = get_socket(); 

 again:

    FD_ZERO(&writefds);
    FD_SET(s, &writefds);
    struct timeval tv = {to, 0};
    rc = select(s + 1, NULL, &writefds, NULL, &tv);
    if(rc < 0) {
        perror("select");
        exit(1);
    }
    if(rc > 0) {
        if(FD_ISSET(s, &writefds)) {
            
            // réponse bien reçue, on peut la gérer

            int rc = sendto(get_socket(), req, req_size, 0, dest, dest_len);
       
            if( rc < 0) {
                if(errno == EAGAIN) {
                    goto again;
                } else {
                    perror("send_to");
                    unlock("send_to");
                    return 0;
                }
            }
            
            unlock("receive_msg");
            return 1;
            
        } else {
            fprintf(stderr, "FD_ISSET : Descripteur de fichier inattendu.\n");
            exit(1);
        }
    }
    // timeout
    
    unlock("send_msg");
    return 0;
}

struct msg* receive_msg(struct sockaddr *dest, socklen_t *dest_len) {
    
    if(debug) printn("Reception d'un message...");
    
    uint8_t data[MAX_RECEIVED];

    lock("receive_msg");

    int rc;
    int to = 5;
    fd_set readfds;
    int s = get_socket(); 

 again:
    
    FD_ZERO(&readfds);
    FD_SET(s, &readfds);
    struct timeval tv = {to, 0};
    rc = select(s + 1, &readfds, NULL, NULL, &tv);
    if(rc < 0) {
        perror("select");
        exit(1);
    }
    if(rc > 0) {
        if(FD_ISSET(s, &readfds)) {
            
            // réponse bien reçue, on peut la gérer

            rc = recvfrom(s, data, MAX_RECEIVED, 0, dest, dest_len);
                        
            if( rc < 0) {
                if(errno == EAGAIN) {
                    goto again;
                } else {
                    perror("receive_msg");
                    unlock("receive_msg");
                    return NULL;
                }
            }
    
            if(debug) printn("Message reçu.");
    
            unlock("receive_msg");

            struct msg* m = data_to_msg(data, rc);
            // Si le message a un bon format.
            if(m != NULL)
                interpret_msg(m, (struct sockaddr_in6*)dest);
            // Sinon
            else {
                printf("Message invalide.\n");
                struct msg* goAway3 = create_msg();
                char goAway_msg[] = "Invalid message";
                add_goAway_tlv(goAway3, 3, (uint8_t*)goAway_msg, strlen(goAway_msg)-1);

                send_msg(goAway3, dest, *dest_len);
                struct sockaddr_in6* dest6 = (struct sockaddr_in6*)dest;
                struct neighbour* n = get_neighbour(((uint128_t*)dest6->sin6_addr.s6_addr)[0], ntohs(dest6->sin6_port));
                if( n != NULL ) {
                    remove_from_neighbours(n);
                    add_potential_neighbour(n);
                }
            }
            
            return m;
            
        } else {
            fprintf(stderr, "FD_ISSET : Descripteur de fichier inattendu.\n");
            exit(1);
        }
    }
    // timeout
    
    unlock("receive_msg");
    return NULL;
}

/********************/
/*  Interpretation  */
/********************/


void interpret_msg(const struct msg* m, struct sockaddr_in6* sockaddr) {
    
    // byte ordre pour ip ?
    for( struct tlv_list* aux = m->first_tlv; aux != NULL; aux = aux->next )
        interpret_tlv(aux->tlv, ((uint128_t*)sockaddr->sin6_addr.s6_addr)[0], sockaddr->sin6_port) ;
}
