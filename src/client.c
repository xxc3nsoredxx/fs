#include <gcrypt.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#include "protocol.h"
#include "client.h"

#ifndef DATA
#define DATA
char data[MAX_LEN];
#endif

#ifndef PACKET
#define PACKET
char packet[sizeof(head) + MAX_LEN];
#endif

/* IP address to connect to */
char *ip;
/* Port to connect to */
char *port;
/* File descriptor for socket */
int cli_sock;

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
    const char *dir = "./";
    char *name;
    char *fname = NULL;
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

    /* Begin the key exchange on the client */
    key = kx_client(cli_sock);
    printf("Key: %d\n", key);

    /* Setup gcrypt on the client */
    if (setup_gcrypt()) {
        printf("Error setting up gcrypt\n");
        close(cli_sock);
        exit(EXIT_FAILURE);
    }

    /* Get directory info */
    request_data(cli_sock, DIR_INFO, dir);
    do {
        printf("%s> ", dir);
        scanf("%ms", &name);
        fname = realloc(fname, strlen(dir) + strlen(name) + 1);
        memset(fname, 0, strlen(dir) + strlen(name) + 1);
        strcat(fname, dir);
        strcat(fname, name);
        if (!strcmp(name, "quit")) {
            break;
        }
        request_data(cli_sock, FILE_CONTENTS, fname);
    } while (1);
    free(name);

    /* Close the connetion */
    printf("Sending a CLOSE_CONNECTION to server\n");
    memset(&head, 0, sizeof(head));
    head.type = CLOSE_CONNECTION;
    if (!send_packet_c(cli_sock)) {
        printf("Failed to write everything\n");
        close(cli_sock);
        exit(EXIT_FAILURE);
    }

    /* Cleanup */
    printf("Closing socket\n");
    close(cli_sock);
    printf("Cleaning up gcrypt\n");
    gcry_md_close(hd);

    exit(EXIT_SUCCESS);
}
