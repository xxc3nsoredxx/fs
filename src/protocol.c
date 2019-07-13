#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "protocol.h"

/* Socket domain: IPv4 */
const int DOMAIN = AF_INET;
/* Use UDP sockets */
const int TYPE = SOCK_DGRAM;
/* Determine protocol based on socket type */
const int PROTOCOL = 0;

/* The data length is given in multiple of 4 bytes */
const int DATA_MULT = 4;

/* Message Tpyes */
const message_t KEY = 0;
const message_t KEY_REPLY = 1;
const message_t KEY_GOOD = 2;
const message_t KEY_RESET = 3;

const message_t REQUEST_DATA = 4;
const message_t SEND_DATA = 5;

const message_t CLOSE_CONNECTION = 65535;

/* Protocol header */
struct prot_head head;
/* Protocol data block */
char data[MAX_LEN];
/* Full protocol packet */
#ifndef PACKET
#define PACKET
char packet[sizeof(head) + MAX_LEN];
#endif

/* Connected peer stuff */
struct sockaddr_storage peer_addr;
socklen_t peer_addr_len;

/* Get a packet from the client socket */
ssize_t get_packet_c (int sock) {
    ssize_t ret = 0;

    /* Clear variables */
    memset(&head, 0, sizeof(head));
    memset(data, 0, sizeof(data));
    memset(packet, 0, sizeof(packet));

    /* Read from socket */
    if ((ret = read(sock, packet, sizeof(packet))) == -1) {
        printf("Failed to read from socket\n");
        close(sock);
        exit(EXIT_FAILURE);
    }

    /* Extract header and data */
    memcpy(&head, packet, sizeof(head));
    memcpy(data, packet + sizeof(head), sizeof(data));

    return ret;
}

/* Get a packet from the server socket */
ssize_t get_packet_s (int sock, char *host, char *service) {
    ssize_t ret = 0;

    /* Clear variables */
    memset(packet, 0, sizeof(packet));
    memset(&head, 0, sizeof(head));
    memset(data, 0, sizeof(data));

   /* Read from socket */
    ret = recvfrom(sock, packet, sizeof(packet), 0,
        (struct sockaddr*) &peer_addr, &peer_addr_len);
    if (getnameinfo((struct sockaddr*) &peer_addr, peer_addr_len,
        host, NI_MAXHOST, service, NI_MAXSERV, NI_NUMERICSERV)) {
        printf("Failed to get request\n");
        ret = -1;
    } else {
        /* Extract header and data */
        memcpy(&head, packet, sizeof(head));
        memcpy(data, packet + sizeof(head), sizeof(data));
    }

    return ret;
}

/* Send a packet over the client socket */
ssize_t send_packet_c (int sock) {
    memset(packet, 0, sizeof(packet));
    memcpy(packet, &head, sizeof(head));
    memcpy(packet + sizeof(head), data, sizeof(data));

    return write(sock, packet, sizeof(packet)) == sizeof(packet);
}

/* Send a packet over the server socket */
ssize_t send_packet_s (int sock) {
    memset(packet, 0, sizeof(packet));
    memcpy(packet, &head, sizeof(head));
    memcpy(packet + sizeof(head), data, sizeof(data));

    return sendto(sock, packet, sizeof(packet), 0,
        (struct sockaddr*) &peer_addr, peer_addr_len) == sizeof(packet);
}

/* Perform a key exchange over the socket */
int kx_client (int sock) {
    int key = 0;
    int recv_key = 0;

start_kx:
    /* Create the key on the client */
    key = rand();
    key += (key == 0) ? 1 : 0;

    /* Send a KEY to the server */
    printf("Sending a KEY to server\n");
    printf("Sending key to server: %d\n", key);
    /* Set header */
    memset(&head, 0, sizeof(head));
    head.type = KEY;
    head.length = sizeof(key) / DATA_MULT;
    head.length += (sizeof(key) % DATA_MULT) ? 1 : 0;
    /* Set data */
    memset(data, 0, sizeof(data));
    memcpy(data, &key, sizeof(key));
    if(!send_packet_c(sock)) {
        printf("Failed to write everything\n");
        close(sock);
        exit(EXIT_FAILURE);
    }

    /* Wait for KEY_REPLY from server */
    do {
        get_packet_c(sock);
    } while(head.type != KEY_REPLY);
    printf("Got a KEY_REPLY from server\n");

    /* Test if key good */
    memcpy(&recv_key, data, sizeof(recv_key));
    printf("Got key from server: %d\n", recv_key);
    if (key == recv_key) {
        /* Send KEY_GOOD to server */
        printf("Sending a KEY_GOOD to server\n");
        memset(&head, 0, sizeof(head));
        head.type = KEY_GOOD;
        if(!send_packet_c(sock)) {
            printf("Failed to write everything\n");
            close(sock);
            exit(EXIT_FAILURE);
        }

        return key;
    } else {
        /* Send a KEY_RESET to server */
        printf("Sending a KEY_RESET to server\n");
        memset(&head, 0, sizeof(head));
        head.type = KEY_RESET;
        if(!send_packet_c(sock)) {
            printf("Failed to write everything\n");
            close(sock);
            exit(EXIT_FAILURE);
        }
        goto start_kx;
    }
}

int kx_server (int sock) {
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];
    int key = 0;

    /* Extract the key */
    memcpy(&key, packet + sizeof(head), sizeof(key));
    printf("Got key from client: %d\n", key);

    /* Send a KEY_REPLY to the client */
    printf("Sending a KEY_REPLY to client\n");
    /* Set header */
    memset(&head, 0, sizeof(head));
    head.type = KEY_REPLY;
    head.length = sizeof(key) / DATA_MULT;
    head.length += (sizeof(key) % DATA_MULT) ? 1 : 0;
    /* Set data */
    memset(data, 0, sizeof(data));
    memcpy(data, &key, sizeof(key));
    if (!send_packet_s(sock)) {
        printf("Failed to send response\n");
    }

    /* Wait for a KEY_GOOD or KEY_RESET from the client */
    get_packet_s(sock, host, service);
    if (head.type == KEY_GOOD) {
        printf("Got a KEY_GOOD from client\n");
        return key;
    } else if (head.type == KEY_RESET) {
        printf("Got a KEY_RESET from client\n");
        return 0;
    }

    return 0;
}
