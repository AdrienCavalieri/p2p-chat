#include "neighbour.h"

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <arpa/inet.h>

#include <string.h>

#define MAX_AGE 120

//static short debug = 0;

struct neighbour {
    uint128_t ip;
    uint16_t port;
    uint64_t id;
    unsigned long hello_date;  // SI (current_date - hello_date < 2min) ALORS (récent)
    unsigned long long_hello_date; // SI (current_date - long_hello_date < 2min) ALORS (symétrique) SINON (non symétrique)
};

/*******************/
/*  Constructeurs  */
/*******************/

struct neighbour* create_neighbour(uint128_t ip, uint16_t port, uint64_t id) {
    struct neighbour* n = malloc(sizeof(struct neighbour));
    if(n == NULL) {
        fprintf(stderr, "malloc() failed.");
        exit(1);
    }

    n->ip = ip;
    n->port = port;
    n->id = id;
    n->hello_date = (unsigned long)time(NULL);
    n->long_hello_date = (unsigned long)time(NULL);

    return n;
}

/*******************/
/*   Destructeurs  */
/*******************/

void destroy_neighbour(struct neighbour* n) {
    free(n);
}


/*******************/
/*   Comparator    */
/*******************/

short equals_neighbours(struct neighbour* n1, struct neighbour* n2) {
    return n1->ip == n2->ip && n1->port == n2->port;
}

/*******************/
/*     Getters     */
/*******************/

uint128_t get_ip(struct neighbour* n) {
    return n->ip;
}

uint128_t get_hip(struct neighbour* n) {
    uint128_t res = 0;
    uint16_t buf[8];
    for(int i=0; i<8; i++)
        buf[i] = ntohs( ((uint16_t*)&n->ip)[i] );

    memmove(&res, buf, sizeof(uint128_t));
    return res;
}

uint16_t get_port(struct neighbour* n) {
    return n->port;
}

uint64_t get_id(struct neighbour* n) {
    return n->id;
}

unsigned long last_hello_age(struct neighbour* n) {
    return ((unsigned long)time(NULL)) - n->hello_date;
}

unsigned long last_longHello_age(struct neighbour* n) {
    return ((unsigned long)time(NULL)) - n->long_hello_date;
}

short get_sockaddr6(struct neighbour* n, struct sockaddr_in6 *dest) {
    dest->sin6_family = AF_INET6;
    dest->sin6_port = n->port;
    memmove(&dest->sin6_addr, &n->ip, sizeof(uint128_t));
    return 1;
}

short is_symmetric(struct neighbour* n) {
    return last_longHello_age(n) < MAX_AGE;
}


/*******************/
/*       MAJ       */
/*******************/

void update_hello_date(struct neighbour* n) {
    n->hello_date = ((unsigned long)time(NULL));
}

void update_longHello_date(struct neighbour* n) {
    n->long_hello_date = ((unsigned long)time(NULL));
}
