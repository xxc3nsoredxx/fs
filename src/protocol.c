#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#include "protocol.h"

/* Socket domain: IPv4 */
const int DOMAIN = AF_INET;
/* Use UDP sockets */
const int TYPE = SOCK_DGRAM;
/* Determine protocol based on socket type */
const int PROTOCOL = 0;

/* Message Tpyes */
const message_t DH_INIT = 0;
const message_t DH_INIT_REPLY = 1;
const message_t DH_CLI_COMB = 2;
const message_t DH_CLI_COMB_REPLY = 3;
const message_t DH_SERV_COMB = 4;
const message_t DH_SERV_COMB_REPLY = 5;
const message_t DH_RESET = 6;
const message_t CLOSE_CONNECTION = 65535;

/* Perform a Diffie-Hellmann key exchange over the socket */
int dhkx_client (int sock) {
    struct prot_head send_head;
    struct prot_head recv_head;
    int key = 0;
    ssize_t len = sizeof(struct prot_head);

    /* Send a DH_INIT to the server */
    printf("Sending a DH_INIT to server\n");
    memset(&send_head, 0, len);
    send_head.type = DH_INIT;
    if(write(sock, &send_head, len) != len) {
        printf("Failed to write everything\n");
        close(sock);
        exit(EXIT_FAILURE);
    }

    /* Wait for DH_INIT_REPLY from server */
    memset(&recv_head, 0, len);
    while(recv_head.type != DH_INIT_REPLY) {
        if (read(sock, &recv_head, len) == -1) {
            printf("Failed to read from socket\n");
            close(sock);
            exit(EXIT_FAILURE);
        }
    }
    printf("Got a DH_INIT_REPLY from server\n");

    /* Send a DH_CLI_COMB to the server */
    printf("Sending a DH_CLI_COMB to server\n");
    memset(&send_head, 0, len);
    send_head.type = DH_CLI_COMB;
    if(write(sock, &send_head, len) != len) {
        printf("Failed to write everything\n");
        close(sock);
        exit(EXIT_FAILURE);
    }

    /* Wait for DH_CLI_COMB_REPLY from server */
    memset(&recv_head, 0, len);
    while(recv_head.type != DH_CLI_COMB_REPLY) {
        if (read(sock, &recv_head, len) == -1) {
            printf("Failed to read from socket\n");
            close(sock);
            exit(EXIT_FAILURE);
        }
    }
    printf("Got a DH_CLI_COMB_REPLY from server\n");

    /* Wait for DH_SERV_COMB from server */
    memset(&recv_head, 0, len);
    while(recv_head.type != DH_SERV_COMB) {
        if (read(sock, &recv_head, len) == -1) {
            printf("Failed to read from socket\n");
            close(sock);
            exit(EXIT_FAILURE);
        }
    }
    printf("Got a DH_SERV_COMB from server\n");

    /* Send a DH_SERV_COMB_REPLY to the server */
    printf("Sending a DH_SERV_COMB_REPLY to server\n");
    memset(&send_head, 0, len);
    send_head.type = DH_SERV_COMB_REPLY;
    if(write(sock, &send_head, len) != len) {
        printf("Failed to write everything\n");
        close(sock);
        exit(EXIT_FAILURE);
    }

    return key;
}

int dhkx_server (int sock, ssize_t len, struct sockaddr *peer_addr,
    socklen_t peer_addr_len) {
    struct prot_head send_head;
    struct prot_head recv_head;
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];
    int key = 0;

    /* Send a DH_INIT_REPLY to the client */
    printf("Sending a DH_INIT_REPLY to client\n");
    memset(&send_head, 0, len);
    send_head.type = DH_INIT_REPLY;
    if (sendto(sock, &send_head, len, 0, peer_addr, peer_addr_len) != len) {
        printf("Failed to send response\n");
    }

    /* Wait for a DH_CLI_COMB from the client */
    memset(&recv_head, 0, len);
    while (recv_head.type != DH_CLI_COMB) {
        len = recvfrom(sock, &recv_head, sizeof(struct prot_head), 0,
            peer_addr, &peer_addr_len);
        if (getnameinfo(peer_addr, peer_addr_len, host, NI_MAXHOST, service,
            NI_MAXSERV, NI_NUMERICSERV)) {
                printf("Failed request\n");
        }
    }
    printf("Got a DH_CLI_COMB from client\n");

    /* Send a DH_CLI_COMB_REPLY to the client */
    printf("Sending a DH_CLI_COMB_REPLY to client\n");
    memset(&send_head, 0, len);
    send_head.type = DH_CLI_COMB_REPLY;
    if (sendto(sock, &send_head, len, 0, peer_addr, peer_addr_len) != len) {
        printf("Failed to send response\n");
    }

    /* Send a DH_SERV_COMB to the client */
    printf("Sending a DH_SERV_COMB to client\n");
    memset(&send_head, 0, len);
    send_head.type = DH_SERV_COMB;
    if (sendto(sock, &send_head, len, 0, peer_addr, peer_addr_len) != len) {
        printf("Failed to send response\n");
    }

    /* Wait for a DH_SERV_COMB_REPLY from the client */
    memset(&recv_head, 0, len);
    while (recv_head.type != DH_SERV_COMB_REPLY) {
        len = recvfrom(sock, &recv_head, sizeof(struct prot_head), 0,
            peer_addr, &peer_addr_len);
        if (getnameinfo(peer_addr, peer_addr_len, host, NI_MAXHOST, service,
            NI_MAXSERV, NI_NUMERICSERV)) {
                printf("Failed request\n");
        }
    }
    printf("Got a DH_SERV_COMB_REPLY from client\n");

    return key;
}
