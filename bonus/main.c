#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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

int is_valid_port (int port) {
    return port > 0 && port <= 65535;
}

int is_valid_ip (const int *ip) {
    return *ip >= 0 && *ip <= 255 &&
            *(ip + 1) >= 0 && *(ip + 1) <= 255 &&
            *(ip + 2) >= 0 && *(ip + 2) <= 255 &&
            *(ip + 3) >= 0 && *(ip + 3) <= 255;
}

int main (int argc, char **argv) {
    int opt;
    int server = 0;
    int client = 0;
    int port = 0;
    int ip[4] = {0,0,0,0};

    if (argc == 1) {
        goto fail;
    }

    /* Parse commandline args */
    while ((opt = getopt(argc, argv, OPTSTR)) != -1) {
        switch (opt) {
        case 's':
            server = 1;
            break;
        case 'c':
            client = 1;
            sscanf(optarg, " %d.%d.%d.%d", ip, (ip + 1), (ip + 2), (ip + 3));
            break;
        case 'p':
            port = atoi(optarg);
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
        printf("Begin server mode\n");
        printf("Port: %d\n", port);
        exit(EXIT_SUCCESS);
    } else if (server) {
        printf("Valid port required\n");
        goto fail;
    }

    /* Client mode */
    if (is_valid_ip(ip) && is_valid_port(port)) {
        printf("Client mode\n");
        printf("IP: %d.%d.%d.%d\n", *ip, *(ip + 1), *(ip + 2), *(ip + 3));
        printf("Port: %d\n", port);
        exit(EXIT_SUCCESS);
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

    exit(EXIT_SUCCESS);
fail:
    usage();
    exit(EXIT_FAILURE);
}
