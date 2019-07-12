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
const message_t CLOSE_CONNECTION = 65535;

struct prot_head send_head;
struct prot_head recv_head;
char send_data[MAX_LEN];
char recv_data[MAX_LEN];
char send_packet[sizeof(struct prot_head) + MAX_LEN];
char recv_packet[sizeof(struct prot_head) + MAX_LEN];

/* Perform a key exchange over the socket */
int kx_client (int sock) {
    int key = 0;
    int recv_key = 0;
    ssize_t len = sizeof(struct prot_head);
    ssize_t data_len = 0;

start_kx:
    /* Create the key on the client */
    key = rand();
    key += (key == 0) ? 1 : 0;

    /* Send a KEY to the server */
    printf("Sending a KEY to server\n");
    printf("Sending key to server: %d\n", key);
    memset(&send_head, 0, len);
    send_head.type = KEY;
    send_head.length = sizeof(key) / DATA_MULT;
    send_head.length += (sizeof(key) % DATA_MULT) ? 1 : 0;
    data_len = send_head.length * DATA_MULT;
    memset(send_data, 0, data_len);
    memcpy(send_data, &key, sizeof(key));
    len = sizeof(send_head) + data_len;
    memcpy(send_packet, &send_head, sizeof(send_head));
    memcpy((send_packet + sizeof(send_head)), send_data, data_len);
    if(write(sock, send_packet, len) != len) {
        printf("Failed to write everything\n");
        close(sock);
        exit(EXIT_FAILURE);
    }

    /* Wait for KEY_REPLY from server */
    memset(&recv_head, 0, len);
    while(recv_head.type != KEY_REPLY) {
        memset(recv_packet, 0, sizeof(recv_packet));

        if (read(sock, recv_packet, len) == -1) {
            printf("Failed to read from socket\n");
            close(sock);
            exit(EXIT_FAILURE);
        }
        memcpy(&recv_head, recv_packet, sizeof(recv_head));
        memcpy(&recv_key, recv_packet + sizeof(recv_head), sizeof(recv_key));
    }
    printf("Got a KEY_REPLY from server\n");
    printf("Got key from server: %d\n", recv_key);

    /* Test if key good */
    if (key == recv_key) {
        /* Send KEY_GOOD to server */
        printf("Sending a KEY_GOOD to server\n");
        memset(&send_head, 0, len);
        send_head.type = KEY_GOOD;
        len = sizeof(send_head);
        memset(send_packet, 0, sizeof(send_packet));
        memcpy(send_packet, &send_head, sizeof(send_head));
        if(write(sock, send_packet, len) != len) {
            printf("Failed to write everything\n");
            close(sock);
            exit(EXIT_FAILURE);
        }

        return key;
    } else {
        /* Send a KEY_RESET to server */
        printf("Sending a KEY_RESET to server\n");
        memset(&send_head, 0, len);
        send_head.type = KEY_RESET;
        len = sizeof(send_head);
        memset(send_packet, 0, sizeof(send_packet));
        memcpy(send_packet, &send_head, sizeof(send_head));
        if(write(sock, send_packet, len) != len) {
            printf("Failed to write everything\n");
            close(sock);
            exit(EXIT_FAILURE);
        }
        goto start_kx;
    }
}

int kx_server (struct prot_head init_head, char *pack, int sock, ssize_t len,
    struct sockaddr *peer_addr, socklen_t peer_addr_len) {
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];
    int key = 0;
    ssize_t data_len = 0;

    /* Extract the key */
    memcpy(&key, pack + sizeof(init_head), sizeof(key));
    printf("Got key from client: %d\n", key);

    /* Send a KEY_REPLY to the client */
    printf("Sending a KEY_REPLY to client\n");
    memset(&send_head, 0, len);
    send_head.type = KEY_REPLY;
    send_head.length = sizeof(key) / DATA_MULT;
    send_head.length += (sizeof(key) % DATA_MULT) ? 1 : 0;
    data_len = send_head.length * DATA_MULT;
    memset(send_data, 0, data_len);
    memcpy(send_data, &key, sizeof(key));
    len = sizeof(send_head) + data_len;
    memcpy(send_packet, &send_head, sizeof(send_head));
    memcpy((send_packet + sizeof(send_head)), send_data, data_len);
    if (sendto(sock, send_packet, len, 0, peer_addr, peer_addr_len) != len) {
        printf("Failed to send response\n");
    }

    /* Wait for a KEY_GOOD or KEY_RESET from the client */
    memset(recv_packet, 0, sizeof(recv_packet));
    memset(&recv_head, 0, sizeof(recv_head));
    recvfrom(sock, recv_packet, sizeof(recv_packet), 0, peer_addr,
        &peer_addr_len);
    if (getnameinfo(peer_addr, peer_addr_len, host, NI_MAXHOST, service,
        NI_MAXSERV, NI_NUMERICSERV)) {
        printf("Failed request\n");
    }
    /* Get the header */
    memcpy(&recv_head, recv_packet, sizeof(recv_head));
    if (recv_head.type == KEY_GOOD) {
        printf("Got a KEY_GOOD from client\n");
        return key;
    } else if (recv_head.type == KEY_RESET) {
        printf("Got a KEY_RESET from client\n");
        return 0;
    }

    return 0;
}
