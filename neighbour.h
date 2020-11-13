#ifndef NEIGHBOUR_H
#define NEIGHBOUR_H

#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef unsigned __int128 uint128_t;

struct neighbour;

/*******************/
/*  Constructeurs  */
/*******************/

/*
 * Creee un voisin.
 */
struct neighbour* create_neighbour(uint128_t ip, uint16_t port, uint64_t id);

/*******************/
/*   Destructeurs  */
/*******************/

/*
 * Lebere la memoire allouee par le voisin n.
 */
void destroy_neighbour(struct neighbour* n);

/*******************/
/*   Comparator    */
/*******************/

/*
 * Compare deux voisins (cad leurs ids et ports).
 */
short equals_neighbours(struct neighbour* n1, struct neighbour* n2);

/*******************/
/*     Getters     */
/*******************/

/*
 * Renvoie l'ip de n apres avoir changer l'ordre des octets si necessaire
 * (ntoh).
 */
uint128_t get_hip(struct neighbour* n);

/*
 * Renvoie l'ip de n.
 */
uint128_t get_ip(struct neighbour* n);

/*
 * Renvoie le port de n.
 */
uint16_t get_port(struct neighbour* n);

/*
 * Renvoie l'id de n.
 */
uint64_t get_id(struct neighbour* n);

/*
 * Renvoie le temps (en secondes) depuis la derniere reception d'un hello venant de n.
 */
unsigned long last_hello_age(struct neighbour* n);

/*
 * Renvoie le temps (en secondes) depuis la derniere reception d'un hello long venant de n.
 */
unsigned long last_longHello_age(struct neighbour* n);

/*
 * Renvoie la sockaddr6 de n (la stocke dans *dest).
 */
short get_sockaddr6(struct neighbour* n, struct sockaddr_in6 *dest);

/*
 * Renvoie 1 si n est symetrique et 0 sinon.
 */
short is_symmetric(struct neighbour* n);

/*******************/
/*     Setters     */
/*******************/

/*
 * Change l'id de n.
 */
void change_id(struct neighbour* n);

/*******************/
/*       MAJ       */
/*******************/

/*
 * Met a jour la date de reception d'un hello venant de n.
 */
void update_hello_date(struct neighbour* n);

/*
 * Met a jour la date de reception d'un hello long venant de n.
 */
void update_longHello_date(struct neighbour* n);

#endif /* NEIGHBOUR_H */
