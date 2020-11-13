#ifndef TLV_H
#define TLV_H

#include <stdint.h>
#include <sys/socket.h>

/*
 * Enumeration listant les tlvs.
 */
enum tlv_type {PAD1, PADN, HELLO, NEIGHBOUR, DATA, ACK, GO_AWAY, WARNING};

typedef unsigned __int128 uint128_t;

struct tlv;

/****************************/
/*        Constructors      */
/****************************/

/*
 * Creee un tlv de type 'type' et dont le body est de longueur 'length'.
 */
struct tlv* create_tlv(uint8_t type, int length);

/*
 * Creee un tlv padn de 'len' 0.
 */
struct tlv* create_padn_tlv(uint8_t len);

/*
 * Creee un tlv hello court avec 'id' comme id source.
 */
struct tlv* create_hello_short_tlv(uint64_t id);

/*
 * Creee un tlv hello long avec l'id source et l'id destionation donnes.
 */
struct tlv* create_hello_long_tlv(uint64_t source_id, uint64_t dest_id);

/*
 * Creee un tlv neighbour "contenant" le voisin (ip,port).
 */
struct tlv* create_neighbour_tlv(uint128_t ip, uint16_t port);

/*
 * Creee un tlv data avec l'id source, son nonce, son type, son contenu et la longeur de ce dernier.
 */
struct tlv* create_data_tlv(uint64_t sender_id, uint32_t nonce, uint8_t type, uint8_t* data, size_t data_len);

/*
 * Creee un tlv ack l'id source et le nonce du message.
 */
struct tlv* create_ack_tlv(uint64_t sender_id, uint32_t nonce);

/*
 * Creee un tlv GoAway.
 */
struct tlv* create_goAway_tlv(uint8_t code, uint8_t* message, size_t message_len);

/*
 * Creee un tlv warning.
 */
struct tlv* create_warning_tlv(uint8_t* message, size_t message_len);

/****************************/
/*        Destructors       */
/****************************/

/*
 * Libere l'espace alloue par le tlv t.
 */
void destroy_tlv(struct tlv* t);

/*******************/
/*     Getters     */
/*******************/

/*
 * Renvoie la longueur du body du tlv 'tlv'.
 */
uint8_t get_tlv_length(struct tlv* tlv);

/*
 * Renvoie l'id source du tlv 'tlv' si il est d'un type qui en contient un, et -1 sinon.
 */
uint64_t get_source_id(struct tlv* tlv);

/*
 * Renvoie l'id destination du tlv 'tlv' si il est d'un type qui en contient un, et -1 sinon.
 */
uint64_t get_destination_id(struct tlv* tlv);

/*
 * Renvoie le nonce du tlv 'tlv' si il est d'un type qui en contient un, et -1 sinon.
 */
uint32_t get_nonce(struct tlv* tlv);

/*
 * Renvoie le type de donnee du tlv 'tlv' si il est de type data, et -1 sinon.
 */
uint8_t get_data_type(struct tlv* tlv);

/*
 * Si tlv est de type data, stocke sont body dans *buffer et sa longueur dans *length et renvoie 0.
 * Sinon renvoie -1;
 */
short get_data(struct tlv* tlv, uint8_t** buffer, size_t* length);

/********************/
/*  Interpretation  */
/********************/

/*
 * Interprete un tlv en fonction du protocole.
 */
void interpret_tlv(struct tlv* t, uint128_t ip, uint16_t port);

/*******************/
/*      Autre      */
/*******************/

/*
 * Stocke le tlv t dans le buffer donne.
 */
void tlv_to_data(struct tlv* t, uint8_t buffer[]);

#endif /* TLV_H */
