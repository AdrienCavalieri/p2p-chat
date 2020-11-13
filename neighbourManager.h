#ifndef NEIGHBOUR_MANAGER_H
#define NEIGHBOUR_MANAGER_H

#include "neighbour.h"
#include "dataManager.h"

#include <stdint.h>

/***************/
/*    Ajout    */
/***************/

/*
 * Ajoute un voisin.
 * Renvoie 1 si ajoute et 0 si il est déjà dans la liste.
 */
short add_neighbour(struct neighbour* n);


/*
 * Ajoute un voisin potentiel.
 * Renvoie 1 si ajoute et 0 si il est déjà dans la liste.
 */
short add_potential_neighbour(struct neighbour* n);


/***************/
/*   Getters   */
/***************/

/*
 * Recupere le voisin (ip,port) dans la liste de voisins (renvoie NULL
 * si il n'y est pas.
 */
struct neighbour* get_neighbour(uint128_t ip, uint16_t port);

/*
 * Renvoie 1 si n est dans la liste de voisins et 0 sinon.
 */
short is_neighbour(struct neighbour* n);

/***************/
/*    Init.    */
/***************/

/*
 * Initialise la liste de voisins symetriques de la donnee recement recu.
 */
void init_symeterics(struct received_data* rd);


/***************/
/* Suppression */
/***************/

/*
 * Supprime le voisin n de la liste des voisins potentiels. Thread-safe.
 */
void remove_from_potentials(struct neighbour* n);


/*
 * Supprime le voisin n de la liste des voisins. Thread-safe.
 */
void remove_from_neighbours(struct neighbour* n);


/**************/
/*  Protocol  */
/**************/

/*
 * Met a jour la liste de voisins en fonction des datte de hellos reçus.
 * Envoie des hellos court aux voisins potentiels si le nombre de voisins
 * symetriques est insufisant. (avec un interval minimum)
 */
void symetrics_maintenance();

/*
 * Envoie des hellos long a tous les voisins. (avec interval minimum)
 * Envoie des tlvs neighbour avec de temps en temps.
 */
void start_hello_sender();

#endif /* NEIGHBOUR_MANAGER */
