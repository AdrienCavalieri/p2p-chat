#ifndef INPUT_READER_H
#define INPUT_READER_H

#include <stdio.h>

/*
 * Initialise le lecteur d'entrees.
 */
void init_inputReader();

/*
 * Lit une entree. Reste disponible pendant quelques secondes
 * et disparait temporairement si il n'y a eu aucune entree. 
 * (Pour ne pas bloquer le protocole)
 */
void read_input();

/*
 * Equivalent de fprintf mais avec un '\n' a la fin. (Et compatible avec inputReader)
 */
void fprintn(FILE* f, const char* format, ...);

/*
 * Equivalent de printf mais avec un '\n' a la fin. (Et compatible avec inputReader)
 */
void printn(const char* format, ...);

#endif /* INPUT_READER_H */ 
