#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include "neighbour.h"

#include <stdint.h>

struct received_data;

/******************/
/*  Constructeur  */
/******************/

/*
 * Creee une donee recement recue. (data est copiee)
 */
struct received_data* create_received_data(uint64_t id, uint32_t nonce, uint8_t type, const uint8_t* data, size_t data_len);

/*****************/
/*  Destructeur  */
/*****************/

/*
 * Libere la memoire allouee par la donee recement recue rd.
 */
void destroy_received_data(struct received_data* rd);

/*******************/
/* Getters/Setters */
/*******************/

/*
 * Renvoie la donee recues (id,nonce) si elle existe et null sinon.
 */
struct received_data* get_received_data(uint64_t id, uint32_t nonce);

/*
 * Met le voisin n dans rd en etat "a recu". Thread-safe.
 */
void received(struct received_data* rd, struct neighbour* n);

/*******************/
/*   Comparator    */
/*******************/

/*
 * Compare deux donnee recues (cad leurs ids et nonces).
 */
short equals_data(struct received_data* d1, struct received_data* d2);

/******************/
/*      Ajout     */
/******************/

/*
 * Ajoute un voisin symetrique a la liste d'attente pour recevoir rd.
 */
void add_symmetric(struct received_data* rd, struct neighbour* n);

/*
 * Ajoute la donnee recue a la liste des donnees recement recues.
 */
short add_received_data(struct received_data* rd);

/*
 * Envoie une donnee depuis le pair courant.
 */
void add_my_data(const uint8_t* d, size_t len);

/*******************/
/*   Suppression   */
/*******************/

/*
 * Supprime le voisin dont l'id est id de la liste des pairs
 * a qui envoyer rd.
 */
void remove_symmetric(struct received_data* rd, uint64_t id);


/*******************/
/*   Innondation   */
/*******************/

/*
 * Lance l'innondation pour la donee rd.
 */
void inondation(struct received_data* rd);

#endif /* DATA_MANAGER */
