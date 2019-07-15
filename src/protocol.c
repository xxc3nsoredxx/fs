#include <dirent.h>
#include <fcntl.h>
#include <gcrypt.h>
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
/* Key exchange messages */
const message_t KEY = 0;
const message_t KEY_REPLY = 1;
const message_t KEY_GOOD = 2;
const message_t KEY_RESET = 3;

/* Data exchange messages */
const message_t REQUEST_DATA = 4;
const message_t SEND_DATA = 5;

/* Control messages */
const message_t CLOSE_CONNECTION = 65535;

/* Data request parameters */
const param_t DIR_INFO = 0;
const param_t FILE_CONTENTS = 1;

/* Protocol header */
struct prot_head head;
/* Protocol data block */
#ifndef DATA
#define DATA
char data[MAX_LEN];
#endif
/* Full protocol packet */
#ifndef PACKET
#define PACKET
char packet[sizeof(head) + MAX_LEN];
#endif

/* Connected peer stuff */
struct sockaddr_storage peer_addr;
socklen_t peer_addr_len;

/* The key to be used in the BLAKE2s keyed hash */
int key = 0;
/* The gcrypt context */
gcry_md_hd_t hd;
/* The hash algorithm */
const int ALGO = GCRY_MD_BLAKE2B_160;
/* The flags used in gcry_md_open */
const int FLAGS = GCRY_MD_FLAG_HMAC;

/* Calculate BLAKE2s hash on the data */
char* blake2 (size_t len) {
    gcry_md_reset(hd);
    gcry_md_write(hd, data, len);
    return (char*) gcry_md_read(hd, ALGO);
}

/* Get a packet from the client socket */
ssize_t get_packet_c (int sock) {
    size_t len;
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

    /* Verify hash if packet has data and key is set */
    if (head.length && key) {
        len = head.length * DATA_MULT;
        len -= (DATA_MULT - 1) - head.stop;

        if (!memcmp(head.blake2, blake2(len), HASH_LEN)) {
            printf("Hash verified\n");
        } else {
            printf("Hash mismatch\n");
        }
    }

    return ret;
}

/* Get a packet from the server socket */
ssize_t get_packet_s (int sock, char *host, char *service) {
    size_t len;
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

        /* Verify hash if packet has data and key is set */
        if (head.length && key) {
            len = head.length * DATA_MULT;
            len -= (DATA_MULT - 1) - head.stop;
    
            if (!memcmp(head.blake2, blake2(len), HASH_LEN)) {
                printf("Hash verified\n");
            } else {
                printf("Hash mismatch\n");
            }
        }
    }

    return ret;
}

/* Send a packet over the client socket */
ssize_t send_packet_c (int sock) {
    size_t len;

    memset(packet, 0, sizeof(packet));
    memcpy(packet + sizeof(head), data, sizeof(data));

    /* Calculate hash if packet has any data and key is set */
    if (head.length && key) {
        len = head.length * DATA_MULT;
        len -= (DATA_MULT - 1) - head.stop;
        /* Copy hash into the header */
        memcpy(head.blake2, blake2(len), HASH_LEN);
    }
    memcpy(packet, &head, sizeof(head));

    return write(sock, packet, sizeof(packet)) == sizeof(packet);
}

/* Send a packet over the server socket */
ssize_t send_packet_s (int sock) {
    size_t len;

    memset(packet, 0, sizeof(packet));
    memcpy(packet + sizeof(head), data, sizeof(data));

    /* Calculate hash if packet has any data and key is set */
    if (head.length && key) {
        len = head.length * DATA_MULT;
        len -= (DATA_MULT - 1) - head.stop;
        /* Copy hash into the header */
        memcpy(head.blake2, blake2(len), HASH_LEN);
    }
    memcpy(packet, &head, sizeof(head));

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
    head.stop = (sizeof(key) - 1) % DATA_MULT;
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
    head.stop = (sizeof(key) - 1) % DATA_MULT;
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

/* Setup function for gcrypt */
gcry_error_t setup_gcrypt () {
    gcry_error_t ret = 0;

    printf("Setting up gcrypt context\n");
    ret = gcry_md_open(&hd, ALGO, FLAGS);
    ret = gcry_md_setkey(hd, &key, sizeof(key));

    return ret;
}

/* Request data */
ssize_t request_data (int sock, param_t param1, const void *param2) {
    ssize_t ret = 0;
    struct dirent *entries;
    int len;
    int cx;

    printf("Sending a REQUEST_DATA to server\n");
    memset(&head, 0, sizeof(head));
    head.type = REQUEST_DATA;

    /* Handle request types */
    if (param1 == DIR_INFO) {
        printf("Parameter: DIR_INFO\n");
    } else if (param1 == FILE_CONTENTS) {
        printf("Parameter: FILE_CONTENTS\n");
    } else {
        printf("Invalid parameter\n");
        return 0;
    }

    head.length = (sizeof(param1) + strlen(param2)) / DATA_MULT;
    head.stop = (sizeof(param1) + strlen(param2) - 1) % DATA_MULT;
    memset(data, 0, sizeof(data));
    memcpy(data, &param1, sizeof(param1));
    strcpy(data + sizeof(param1), param2);
    printf("Parameter: %s\n", data + sizeof(param1));

    if (!(ret = send_packet_c(sock))) {
        printf("Failed to write everything\n");
        close(sock);
        exit(EXIT_FAILURE);
    }

    /* Wait for data */
    do {
        get_packet_c(sock);
    } while(head.type != SEND_DATA);
    printf("Got a SEND_DATA from server\n");

    /* Print data */
    if (param1 == DIR_INFO) {
        /* Calculate the number of entries */
        len = head.length * DATA_MULT;
        len -= (DATA_MULT - 1) - head.stop;
        len /= sizeof(struct dirent);
        entries = (struct dirent*) data;

        /* Print each entry */
        for (cx = 0; cx < len; cx++) {
            printf("%s\n", (entries + cx)->d_name);
        }
    } else if (param1 == FILE_CONTENTS) {
        /* Calculate the file length */
        len = head.length * DATA_MULT;
        len -= (DATA_MULT - 1) - head.stop;

        /* Output the file to the screen */
        for (cx = 0; cx < len; cx++) {
            printf("%c", *(data + cx));
        }
        printf("\n");
    }

    return ret;
}

/* Send data */
ssize_t send_data (int sock, void *d, size_t len) {
    size_t ret = 0;

    printf("Sending a SEND_DATA to client\n");
    /* Set header */
    memset(&head, 0, sizeof(head));
    head.type = SEND_DATA;
    head.length = len / DATA_MULT;
    head.length += (len % DATA_MULT) ? 1 : 0;
    head.stop = (len - 1) % DATA_MULT;

    /* Set data */
    memset(data, 0, sizeof(data));
    memcpy(data, d, len);
    if (!send_packet_s(sock)) {
        printf("Failed to send response\n");
    }

    return ret;
}

/* Handle the data request */
void handle_request_data (int sock) {
    param_t param1;
    char *param2;
    DIR *dir;
    struct dirent *entries = NULL;
    struct dirent *e;
    int cx = 0;
    int file;
    off_t fsize;
    char *fbuf;

    /* Extract the request */
    memcpy(&param1, data, sizeof(param1));

    if (param1 == DIR_INFO) {
        printf("Got a DIR_INFO request from client\n");
        param2 = calloc(strlen(data + sizeof(param1)), 1);
        strcpy(param2, data + sizeof(param1));
        printf("Directory requested: %s\n", param2);

        /* Open the directory */
        dir = opendir(param2);

        /* Read the entries */
        while ((e = readdir(dir))) {
            cx++;
            entries = realloc(entries, cx * sizeof(struct dirent));
            memcpy(entries + (cx - 1), e, sizeof(struct dirent));
        }

        /* Send the directory info */
        send_data(sock, entries, cx * sizeof(struct dirent));

        free(entries);
        closedir(dir);
        free(param2);
    } else if (param1 == FILE_CONTENTS) {
        printf("Got a FILE_CONTENTS request from client\n");
        param2 = calloc(strlen(data + sizeof(param1)), 1);
        strcpy(param2, data + sizeof(param1));
        printf("File requested: %s\n", param2);

        /* Open file */
        file = open(param2, O_RDONLY);
        if (!file) {
            printf("Unable to open file\n");
            return;
        }

        /* Get file size */
        fsize = lseek(file, 0, SEEK_END);
        lseek(file, 0, SEEK_SET);

        /* Check if file too large */
        if (fsize > MAX_LEN) {
            printf("File too large\n");
            close(file);
            return;
        }

        /* Copy file to buffer and send */
        fbuf = calloc(fsize, 1);
        read(file, fbuf, fsize);
        send_data(sock, fbuf, fsize);

        free(fbuf);
        close(file);
    } else {
        printf("Got an invalid request from lient\n");
    }
}
