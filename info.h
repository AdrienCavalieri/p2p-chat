#ifndef INFO_H
#define INFO_H

/*
 * Stocke la socket. 
 */
void init_info(int socket);

/*
 * Renvoie la socket stockee et -1 si aucune socket n'a ete stockee.
 */
int get_socket();

#endif /* INFO_H */
