#include <ifaddrs.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#include "protocol.h"
#include "server.h"

#ifndef PACKET
#define PACKET
char packet[sizeof(head) + MAX_LEN];
#endif

char test1[100];
char test2[500];

void generate_test_data () {
    int cx;
    for (cx = 0; cx < 500; cx++) {
        *(test2 + cx) = cx % 10;
        if (cx < 100) {
            *(test1 + cx) = cx % 5;
        }
    }
}

/* Port number for incoming requests */
char *port;
/* File descriptor for socket */
int serv_sock;

/* Display the IPv4 address for any interfaces */
void display_server_ip () {
    struct ifaddrs *ifaddr;
    struct ifaddrs *ifa;

    if (getifaddrs(&ifaddr) == -1) {
        printf("Error getting server IP info\n");
        return;
    }

    for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
        struct sockaddr *adr = ifa->ifa_addr;
        int family;
        char host[NI_MAXHOST];

        if (!adr || !strcmp(ifa->ifa_name, "lo")) {
            continue;
        }

        family = adr->sa_family;

        if (family != DOMAIN ||
            getnameinfo(adr, sizeof(struct sockaddr_in), host, NI_MAXHOST,
                NULL, 0, NI_NUMERICHOST)) {
            continue;
        }

        printf("Interface: %s\n", ifa->ifa_name);
        printf("\tAddress: %s\n", host);
    }

    freeifaddrs(ifaddr);
}

/* Create the server-side socket */
int create_server_socket () {
    struct addrinfo hints;
    struct addrinfo *result;
    struct addrinfo *rp;

    memset(&hints, 0, sizeof(hints));
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
        serv_sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (serv_sock == -1) {
            continue;
        }

        if (!bind(serv_sock, rp->ai_addr, rp->ai_addrlen)) {
            /* Successfully bind */
            break;
        }

        close(serv_sock);
    }

    if (!rp) {
        printf("Failed to bind\n");
        return -1;
    }

    printf("Successfully bound socket to port %s\n", port);

    freeaddrinfo(result);

    return 0;
}

/* Entry point into server mode */
void begin_server_mode (char *p) {
    int active = 1;
    int key = 0;
    ssize_t nread;
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];

    /* Testing data */
    generate_test_data();

    port = p;
    printf("Begin server mode\n");
    display_server_ip();
    printf("Port: %s\n", port);

    if (!create_server_socket()) {
        printf("Successfully created server socket\n");
    } else {
        printf("Failed to create server socket\n");
        exit(EXIT_FAILURE);
    }

    /* Read packets from the socket */
    while (active) {
        peer_addr_len = sizeof(socklen_t);

        /* Get the packet */
        nread = get_packet_s(serv_sock, host, service);
        if (nread > 0) {
            /* Parse the header */
            if (head.type == KEY) {
                /* Begin the key exchange on the server */
                printf("Got a KEY from client\n");
                key = kx_server(serv_sock);
                if (key > 0) {
                    printf("Key: %d\n", key);
                }
            } else if (head.type == KEY_RESET) {
                printf("Got a KEY_RESET from client\n");
                key = 0;
            } else if (head.type == REQUEST_DATA) {
                printf("Got a REQUEST_DATA from client\n");
            } else if (head.type == CLOSE_CONNECTION) {
                printf("Got a CLOSE_CONNECTION from client\n");
                active = 0;
            } else {
                printf("Invalid message %d from client\n", head.type);
            }
        }
    }

    /* Cleanup */
    close(serv_sock);

    exit(EXIT_SUCCESS);
}
