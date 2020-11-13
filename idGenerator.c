#include "idGenerator.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

static uint64_t my_id = 0;

void generate_id() {
    // Ouvre /dev/urandom.
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "open\n");
        exit(EXIT_FAILURE);
    }

    // Lis 32 bits.
    int rc = read(fd, &my_id, 8);
    if (rc < 0) {
        fprintf(stderr, "read\n");
        exit(EXIT_FAILURE);
    }

    // Ferme /dev/urandom.
    close(fd);
}

uint64_t get_my_id() {
    return my_id;
}
