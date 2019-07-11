#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#include "protocol.h"
#include "server.h"

/* Port number for incoming requests */
char *port;
/* File descriptor socket */
int sock;

int create_socket () {
    struct addrinfo hints;
    struct addrinfo *result;
    struct addrinfo *rp;
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = DOMAIN;
    hints.ai_socktype = TYPE;
    /* Wildcard IP addresses */
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = PROTOCOL;

    /* Get address info */
    if (!getaddrinfo(NULL, port, &hints, &result)) {
        printf("Successfully got info for port %s\n", port);
    } else {
        printf("Unable to get info for port %s\n", port);
        return -1;
    }

    /* Attempt to bind */
    for (rp = result; rp; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock == -1) {
            continue;
        }

        if (!bind(sock, rp->ai_addr, rp->ai_addrlen)) {
            /* Successfully bind */
            break;
        }

        close(sock);
    }

    if (!rp) {
        printf("Failed to bind\n");
        return -1;
    }

    printf("Successfully bound socket\n");

    freeaddrinfo(result);

    return 0;
}

void begin_server_mode (char *p) {
    port = p;
    printf("Begin server mode\n");
    printf("Port: %s\n", port);

    if (!create_socket()) {
        printf("Successfully created server socket\n");
    } else {
        printf("Failed to create server socket\n");
        exit(EXIT_FAILURE);
    }

    /* Cleanup */
    close(sock);

    exit(EXIT_SUCCESS);
}
