#ifndef IDGENERATOR_H
#define IDGENERATOR_H

#include <stdint.h>

/*
 * Genere un id et le stocke.
 */
void generate_id();

/*
 * Renvoie l'id genere et 0 si aucun n'a ete genere.
 */
uint64_t get_my_id();

#endif /* IDGENERATOR_H */
