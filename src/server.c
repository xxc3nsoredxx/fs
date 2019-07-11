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

/* Port number for incoming requests */
char *port;
/* File descriptor for socket */
int serv_sock;
/* Connected peer stuff */
struct sockaddr_storage peer_addr;
socklen_t peer_addr_len;

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

int create_server_socket () {
    struct addrinfo hints;
    struct addrinfo *result;
    struct addrinfo *rp;

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

void begin_server_mode (char *p) {
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

    /* Test get data */
    while (1) {
        ssize_t nread;
        int i;
        char host[NI_MAXHOST];
        char service[NI_MAXSERV];

        peer_addr_len = sizeof(struct sockaddr_storage);
        nread = recvfrom(serv_sock, &i, sizeof(int), 0,
            (struct sockaddr*) &peer_addr, &peer_addr_len);

        if (!getnameinfo((struct sockaddr*) &peer_addr, peer_addr_len, host,
            NI_MAXHOST, service, NI_MAXSERV, NI_NUMERICSERV)) {
            printf("Got %d from client\n", i);
            i = -i;
        } else {
            printf("Failed request\n");
        }

        if (sendto(serv_sock, &i, nread, 0, (struct sockaddr*) &peer_addr,
            peer_addr_len) != nread) {
            printf("Failed to send response\n");
        }
    }

    /* Cleanup */
    close(serv_sock);

    exit(EXIT_SUCCESS);
}
