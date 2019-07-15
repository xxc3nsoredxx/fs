#include <gcrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "client.h"
#include "server.h"

/*
 * Commandline args:
 * -s       start server mode
 * -c [ip]  connect to server on ip
 * -p [p]   if -s, start server on port p
 *          if -c, connect on port p
 */
const char *OPTSTR = "sc:p:";

void usage () {
    printf("Usage: ./fs [-s] [-c ip] [-p port]\n");
    printf("\t-s\t\tstart server mode\n");
    printf("\t-c ip\t\tconnect to server on ip\n");
    printf("\t-p port\t\tif -s, start server on port\n");
    printf("\t\t\tif -c, connect to port\n");
}

int is_valid_port (const char *p) {
    int port = atoi(p);
    return port > 0 && port <= 65535;
}

int is_valid_ip (const char *ip) {
    int i[4] = {0,0,0,0};
    sscanf(ip, " %d.%d.%d.%d", i, (i + 1), (i + 2), (i + 3));
    return *i >= 0 && *i <= 255 &&
            *(i + 1) >= 0 && *(i + 1) <= 255 &&
            *(i + 2) >= 0 && *(i + 2) <= 255 &&
            *(i + 3) >= 0 && *(i + 3) <= 255;
}

int main (int argc, char **argv) {
    int opt;
    int server = 0;
    int client = 0;
    char *port = "-1";
    char *ip = "-1";

    if (argc == 1) {
        goto fail;
    }

    /* Initialize gcrypt */
    if (!gcry_check_version(GCRYPT_VERSION)) {
        printf("Libgcrypt version mismatch\n");
        exit(EXIT_FAILURE);
    }
    gcry_control(GCRYCTL_DISABLE_SECMEM, 0);
    gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);

    /* Seed random */
    srand(time(NULL));

    /* Parse commandline args */
    while ((opt = getopt(argc, argv, OPTSTR)) != -1) {
        switch (opt) {
        case 's':
            server = 1;
            break;
        case 'c':
            client = 1;
            ip = strdup(optarg);
            break;
        case 'p':
            port = strdup(optarg);
            break;
        default:
            goto fail;
        }
    }

    if (server && client) {
        printf("Cannot run both server and client mode\n");
        goto fail;
    }

    /* Server mode */
    if (server && is_valid_port(port)) {
        begin_server_mode(port);
    } else if (server) {
        printf("Valid port required\n");
        goto fail;
    }

    /* Client mode */
    if (is_valid_ip(ip) && is_valid_port(port)) {
        begin_client_mode(ip, port);
    } else if (is_valid_ip(ip)) {
        printf("Valid port required\n");
        goto fail;
    } else if (is_valid_port(port)) {
        printf("Valid IP address required\n");
        goto fail;
    } else {
        printf("Valid port and IP address required\n");
        goto fail;
    }

fail:
    usage();
    exit(EXIT_FAILURE);
}
