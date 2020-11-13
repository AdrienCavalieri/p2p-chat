#include "tlv.h"

#include "idGenerator.h"
#include "neighbour.h"
#include "message.h"
#include "neighbourManager.h"
#include "dataManager.h"
#include "inputReader.h"

#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static short debug = 0;

struct tlv {
    uint8_t type;
    uint8_t body_length;
    uint8_t* body;
};

/****************************/
/*        Constructors      */
/****************************/

struct tlv* create_tlv(uint8_t type, int length) {
    struct tlv* t = malloc(sizeof(struct tlv));
    if(t == NULL) {
        perror("create_tlv: malloc() failed.");
        exit(EXIT_FAILURE);
    }
    t->type = type;
    t->body_length = length;
    t->body = malloc(length);
    memset(t->body, 0, length);
    return t;
}


struct tlv* create_padn_tlv(uint8_t len) {
    struct tlv* padn = create_tlv(1, len);
    memset(padn->body, 0, len);

    return padn;
}

struct tlv* create_hello_short_tlv(uint64_t id) {
    struct tlv* hello_short = create_tlv(2, 8);
    uint64_t* b = (uint64_t*)hello_short->body;
    b[0] = id;

    return hello_short;
}

struct tlv* create_hello_long_tlv(uint64_t source_id, uint64_t dest_id) {
    struct tlv* hello_long = create_tlv(2, 16);
    uint64_t* b = (uint64_t*)hello_long->body;
    b[0] = source_id;
    b[1] = dest_id;

    return hello_long;
}

struct tlv* create_neighbour_tlv(uint128_t ip, uint16_t port) {
    struct tlv* neighbour = create_tlv(3, 18);
    uint128_t* b = (uint128_t*)neighbour->body;
    b[0] = ip;
    ((uint16_t*)neighbour->body)[8] = port;

    return neighbour;
}

struct tlv* create_data_tlv(uint64_t sender_id, uint32_t nonce, uint8_t type, uint8_t* data, size_t data_len) {
    struct tlv* data_tlv = create_tlv(4, data_len + 13);
    ((uint64_t*)data_tlv->body)[0] = sender_id;
    ((uint32_t*)data_tlv->body)[2] = nonce;
    data_tlv->body[12] = type;
    memcpy(data_tlv->body+13, data, data_len);

    return data_tlv;
}

struct tlv* create_ack_tlv(uint64_t sender_id, uint32_t nonce) {
    struct tlv* ack = create_tlv(5, 12);
    ((uint64_t*)ack->body)[0] = sender_id;
    ((uint32_t*)ack->body)[2] = nonce;

    return ack;
}

struct tlv* create_goAway_tlv(uint8_t code, uint8_t* message, size_t message_len) {
    struct tlv* goAway = create_tlv(6, message_len + 1);
    goAway->body[0] = code;
    memcpy(goAway->body+1, message, message_len);

    return goAway;
}

struct tlv* create_warning_tlv(uint8_t* message, size_t message_len) {
    struct tlv* warning = create_tlv(7, message_len);
    memcpy(warning->body, message, message_len);

    return warning;
}


/****************************/
/*        Destructors       */
/****************************/

void destroy_tlv(struct tlv* t) {
    if(t != NULL) {
        free(t->body);
        free(t);
    }
}

/*******************/
/*     Getters     */
/*******************/

uint8_t get_tlv_length(struct tlv* tlv) {
    return tlv->body_length + 2;
}

uint64_t get_source_id(struct tlv* tlv) {
    // Si c'est un TLV Data, Ack, Hello court ou long ...
    if (tlv->type == 2 || tlv->type == 4 || tlv->type == 5)
        return ((uint64_t*)tlv->body)[0];

    return -1;
}

uint128_t get_neighbour_ip(struct tlv* tlv) {
    // Si c'est un TLV Data ...
    if (tlv->type == 3)
        return ((uint128_t*)tlv->body)[0];

    return -1;
}

uint64_t get_neighbour_port(struct tlv* tlv) {
    // Si c'est un TLV Data ...
    if (tlv->type == 3)
        return ((uint16_t*)tlv->body)[8];

    return -1;
}

uint64_t get_destination_id(struct tlv* tlv) {
    // Si c'est un TLV Hello long ...
    if (tlv->type == 2 && tlv->body_length == 16)
        return ((uint64_t*)tlv->body)[1];

    return -1;
}

uint32_t get_nonce(struct tlv* tlv) {
    // Si c'est un TLV Data ou Ack ...
    if (tlv->type == 4 || tlv->type == 5)
        return ((uint32_t*)tlv->body)[2];

    return -1;
}


uint8_t get_data_type(struct tlv* tlv) {
    // Si c'est un TLV Data ...
    if (tlv->type == 4)
        return ((uint8_t*)tlv->body)[12];

    return -1;
}

short get_data(struct tlv* tlv, uint8_t** buffer, size_t* length) {
    // Si c'est un TLV Data ...
    if (tlv->type != 4)
        return -1;

    *buffer = tlv->body+13;
    *length = tlv->body_length - 13;
    return 0;
}

static short print_warning(struct tlv* t) {
    if( t->type != 7)
        return -1;

    uint8_t buf[t->body_length+1];
    memmove(buf, t->body, t->body_length);
    buf[t->body_length] = '\0';
    printn("Warning : %s\n", buf);

    return 0;
}

/********************/
/*  Interpretation  */
/********************/

void interpret_tlv( struct tlv* t, uint128_t ip, uint16_t port) {
    struct neighbour* n;
    struct sockaddr_in6 sin6;

    uint8_t* buff;
    size_t len;

    struct msg* m;
    struct received_data* rd;

    switch(t->type) {
    case PAD1:
        if(debug)
            printn("Pad1 reçu.");
        break;
    case PADN:
        if(debug)
            printn("PadN reçu.");
        break;
    case HELLO:

        // Si jamais on boucle
        if( get_source_id(t) == get_my_id() ) {
            n = create_neighbour(ip, port, get_my_id());
            remove_from_potentials(n);
            destroy_neighbour(n);
            return;
        }

        if(debug)
            printn("HELLO %s reçu.", t->body_length == 16 ? "LONG" : "COURT");
        // On ajoute le voisin à la liste de voisins
        n = create_neighbour(ip, port, get_source_id(t));
        if( !add_neighbour(n) ) {
            destroy_neighbour(n);
            n = get_neighbour(ip, port);
        }else{
            remove_from_potentials(n);
        }

        assert(n != NULL);

        update_hello_date(n);
        if(t->body_length == 16 && get_destination_id(t) == get_my_id())
            update_longHello_date(n);

        else if(t->body_length == 16 && debug)
            fprintn(stderr, "Message rejeté, hello long avec un mauvais id-destination.(reçu: %lx, mon id: %lx)",(unsigned long)get_destination_id(t),(unsigned long)get_my_id());

        break;

    case NEIGHBOUR:

        if(debug)
            printn("Neighbour reçu.");

        n = create_neighbour(get_neighbour_ip(t), get_neighbour_port(t), get_source_id(t));
        if( !is_neighbour(n) ) {
            add_potential_neighbour(n);
        }else {
            destroy_neighbour(n);
            n = NULL;
        }
        break;

    case DATA:
        if(debug)
            printn("Data reçu.");

        get_data(t, &buff, &len);
        rd = create_received_data( get_source_id(t), get_nonce(t), get_data_type(t), buff, len);

        // Si on ne l'avait pas
        if( add_received_data(rd) ) {
            if(get_data_type(t) == 0)
                printn("%s", buff);
            init_symeterics(rd);
            inondation(rd);
        } else {
            destroy_received_data(rd);
            rd = get_received_data(get_source_id(t), get_nonce(t));
        }

        n = get_neighbour(ip, port);

        received(rd, n);

        if( n != NULL ) {
            m = create_msg();
            add_ack_tlv(m, get_source_id(t), get_nonce(t));
            get_sockaddr6(n, &sin6);
            send_msg(m, (struct sockaddr*)&sin6, sizeof(sin6));
            destroy_msg(m);
            m = NULL;
        }

        break;

    case ACK:

        if(debug)
            printn("Ack reçu.");

        n = get_neighbour(ip, port);
        rd = get_received_data(get_source_id(t), get_nonce(t));

        if(rd != NULL && n != NULL)
            received(rd, n);
        break;

    case GO_AWAY:

        if(debug)
            printn("GoAway reçu.");

        n = get_neighbour(ip, port);
        if( n != NULL ) {
            remove_from_neighbours(n);
            add_potential_neighbour(n);
        }
        break;

    case WARNING:

        if(debug)
            printn("Warning reçu.");

        print_warning(t);
        break;
    }
}


/*******************/
/*      Autre      */
/*******************/

void tlv_to_data(struct tlv* t, uint8_t buffer[]) {
    buffer[0] = t->type;
    buffer[1] = t->body_length;
    memcpy(buffer+2, t->body, t->body_length);
}
