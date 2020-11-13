#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef unsigned __int128 uint128_t;

struct msg;

/*******************/
/*  Constructeurs  */
/*******************/

/*
 * Creee un message.
 */
struct msg* create_msg();

/*******************/
/*   Destructeurs  */
/*******************/

/*
 * Lebere la memoire allouee par le message m.
 */
void destroy_msg(struct msg* m);

/*******************/
/*      Ajout      */
/*******************/

/*
 * Ajoute un tlv padn au message m.
 */
void add_padn_tlv(struct msg* m, uint8_t len);

/*
 * Ajoute un tlv hello court au message m.
 */
void add_hello_short_tlv(struct msg* m, uint64_t id);

/*
 * Ajoute un tlv hello long au message m.
 */
void add_hello_long_tlv(struct msg* m, uint64_t source_id, uint64_t dest_id);

/*
 * Ajoute un tlv neighbour au message m.
 */
void add_neighbour_tlv(struct msg* m, uint128_t ip, uint16_t port);

/*
 * Ajoute un tlv data au message m.
 */
void add_data_tlv(struct msg* m, uint64_t sender_id, uint32_t nonce, uint8_t type, uint8_t* data, size_t data_len);

/*
 * Ajoute un tlv ack au message m.
 */
void add_ack_tlv(struct msg* m, uint64_t sender_id, uint32_t nonce);

/*
 * Ajoute un tlv goaway au message m.
 */
void add_goAway_tlv(struct msg* m, uint8_t code, uint8_t* message, size_t message_len);

/*
 * Ajoute un tlv warning au message m.
 */
void add_warning_tlv(struct msg* m, uint8_t* message, size_t message_len);

/********************/
/* Envoie/Reception */
/********************/

/*
 * Envoie le message m a dest. Renvoie 0 si l'envoie du message a echoue et 1 sinon.
 */
short send_msg(struct msg* m, struct sockaddr *dest, size_t dest_len);

/*
 * Receptionne un message m.
 */
struct msg* receive_msg(struct sockaddr *dest, socklen_t* dest_len);

/********************/
/*  Interpretation  */
/********************/

/*
 * Interprete le message par rapport au protocol.
 */
void interpret_msg(const struct msg* m, struct sockaddr_in6* sockaddr);

#endif /* MESSAGE_H */
