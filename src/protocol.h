#ifndef PROTOCOL_H_20190710_174035
#define PROTOCOL_H_20190710_174035

extern const int DOMAIN;
extern const int TYPE;
extern const int PROTOCOL;

/* Max packet data length */
#define MAX_LEN 1024
extern const int DATA_MULT;

typedef uint16_t message_t;
extern const message_t KEY;
extern const message_t KEY_REPLY;
extern const message_t KEY_GOOD;
extern const message_t KEY_RESET;

extern const message_t REQUEST_DATA;
extern const message_t SEND_DATA;
extern const message_t CLOSE_CONNECTION;

extern struct prot_head head;
extern char data[];
extern char packet[];

extern struct sockaddr_storage peer_addr;
extern socklen_t peer_addr_len;

struct prot_head {
    uint32_t md5[4];
    message_t type;
    uint16_t length;
    uint16_t stop;
    uint16_t segment;
};

ssize_t get_packet_c (int sock);
ssize_t get_packet_s (int sock, char *host, char *service);

ssize_t send_packet_c (int sock);
ssize_t send_packet_s (int sock);

int kx_client (int sock);
int kx_server (int sock);

#endif /* PROTOCOL_H_20190710_174035 */
