#include "message.h"
#include "info.h"
#include "idGenerator.h"
#include "neighbourManager.h"
#include "inputReader.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <fcntl.h>
#include <limits.h>

#include <string.h>

#include <unistd.h>


static int create_socket() {
    int s = socket(AF_INET6, SOCK_DGRAM, 0);
    if(s < 0) {
        perror("socket");
        exit(1);
    }
    return s;
}

static void set_options(int socket) {
    int ok=1;
	setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &ok, sizeof(ok));

    int rc = fcntl(socket, F_GETFL);
    if(rc < 0) {
        perror("fcntl(F_GETFL)");
        return;
    }
    rc = fcntl(socket, F_SETFL, rc | O_NONBLOCK);
    if(rc < 0) {
        perror("fcntl(F_SETFL, NONBLOCK)");
        return;
    }
}

static void init_first_neighbour(struct sockaddr_in6 *server, const char* ip, uint16_t port) {
    server->sin6_family = AF_INET6;
    server->sin6_port = htons(1212);
    int rc = inet_pton(AF_INET6, ip, &server->sin6_addr );
    if(rc == 0) {
        perror("inet_pton(bad address format)");
        exit(1);
    }
    if(rc < 0) {
        perror("inet_pton");
        exit(1);
    }

}

/**************/
/*    Main    */
/**************/

int main(int argc, char* args[]) {

    // Controle d'entrees.

    if( argc < 3 ) {
        fprintf(stderr, "Argument manuquant : Adresse ip du premier voisin et le port utilisé sont demandés.\n");
        return 1;
    }

    char* end = NULL;
    long port = strtol(args[2], &end, 10);

    if(*end != '\0') {
        fprintf(stderr, "Le port donné n'est pas un nombre.\n");
        return 1;
    }

    if(port < 0 || port > USHRT_MAX) {
        fprintf(stderr, "Le port donné n'est pas compris entre %d et %d.\n", 0, USHRT_MAX);
        return 1;
    }

    // initialisations

    init_inputReader();

    srandom(time(NULL));
    generate_id();

    int s = create_socket();
    set_options(s);

    init_info(s);

    struct sockaddr_in6 peer;
    socklen_t len = sizeof(peer);
    init_first_neighbour(&peer, args[1], port);

    // premier message

    struct msg* m = create_msg();
    add_hello_short_tlv(m, get_my_id());
    // Si l'envoie echoue on quitte.
    if( !send_msg(m, (struct sockaddr*)&peer, sizeof(peer)) )
        return 1;


    time_t last_overload = 0;
    int count;

    while(1) {
        m = receive_msg( (struct sockaddr*)&peer, &len);
        destroy_msg(m);
        m = NULL;

        // Filtre de survie anti-flood:
        // Si la socket est trop chargée on ne propose pas
        // d'entrée utilisateur tant que le flux n'est pas plus
        // calme.
        ioctl(s, FIONREAD, &count);
        if(count < 4 && time(NULL) - last_overload > 3)
            read_input();
        else if(count > 3)
            last_overload = time(NULL);

        start_hello_sender();
        symetrics_maintenance();
    }

    close(s);
    return 0;
}
