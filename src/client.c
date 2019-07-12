#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#include "protocol.h"
#include "client.h"

/* IP address to connect to */
char *ip;
/* Port to connect to */
char *port;
/* File descriptor for socket */
int cli_sock;
/* Header to send */
struct prot_head head;
/* Data to send */
char *data;

/* Create the client-side socket */
int create_client_socket () {
    struct addrinfo hints;
    struct addrinfo *result;
    struct addrinfo *rp;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = DOMAIN;
    hints.ai_socktype = TYPE;
    hints.ai_protocol = PROTOCOL;

    /* Get info for ip/port */
    if (!getaddrinfo(ip, port, &hints, &result)) {
        printf("Successfully got info for %s:%s\n", ip, port);
    } else {
        printf("Unable to get info for %s:%s\n", ip, port);
        return -1;
    }

    /* Attempt to connect */
    for (rp = result; rp; rp = rp->ai_next) {
        cli_sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (cli_sock == -1) {
            continue;
        }

        if (connect(cli_sock, rp->ai_addr, rp->ai_addrlen) != -1) {
            /* Successfully connect */
            break;
        }

        close(cli_sock);
    }

    if (!rp) {
        printf("Failed to connect\n");
        return -1;
    }

    printf("Successfully connected to %s:%s\n", ip, port);

    freeaddrinfo(result);

    return 0;
}

/* Entry point into client mode */
void begin_client_mode (char *i, char *p) {
    int key = 0;
    /* int j = 0; */
    /* int k = 0; */
    /* ssize_t len = sizeof(int); */
    ip = i;
    port = p;

    printf("Begin client mode\n");
    printf("Server IP address: %s\n", ip);
    printf("Server port: %s\n", port);

    if (!create_client_socket()) {
        printf("Successfully created soccet\n");
    } else {
        printf("Failed to create client socket\n");
        exit(EXIT_FAILURE);
    }

    /* Begin the Diffie-Hellmann key exchange on the client */
    key = dhkx_client(cli_sock);
    printf("Key: %d\n", key);

    memset(&head, 0, sizeof(head));
    head.type = CLOSE_CONNECTION;
    if (write(cli_sock, &head, sizeof(head)) != sizeof(head)) {
        printf("Failed to write everything\n");
        close(cli_sock);
        exit(EXIT_FAILURE);
    }

    /* Test sending data */
    /*
    printf("> ");
    scanf(" %d", &j);
    while (j >= 0) {
        if (write(cli_sock, &j, len) != len) {
            printf("Failed to write everything\n");
            close(cli_sock);
            exit(EXIT_FAILURE);
        }

        if (read(cli_sock, &k, len) == -1) {
            printf("Failed to read from socket\n");
            close(cli_sock);
            exit(EXIT_FAILURE);
        }

        printf("Got %d from server\n", k);
        printf("> ");
        scanf(" %d", &j);
    }
    */

    /* Cleanup */
    close(cli_sock);

    exit(EXIT_SUCCESS);
}
