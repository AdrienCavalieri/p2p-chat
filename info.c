#include "info.h"

static int socket = -1;

void init_info(int sock) {
    socket = sock;
}

int get_socket() {
    return socket;
}
