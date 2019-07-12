#ifndef PROTOCOL_H_20190710_174035
#define PROTOCOL_H_20190710_174035

extern const int DOMAIN;
extern const int TYPE;
extern const int PROTOCOL;

/* Max packet data length */
#define MAX_LEN 1000
extern const int DATA_MULT;

typedef uint16_t message_t;
extern const message_t KEY;
extern const message_t KEY_REPLY;
extern const message_t KEY_GOOD;
extern const message_t KEY_RESET;
extern const message_t CLOSE_CONNECTION;

struct prot_head {
    uint32_t md5[4];
    message_t type;
    uint16_t length;
};

int kx_client (int sock);
int kx_server (struct prot_head init_head, char *pack, int sock, ssize_t len,
    struct sockaddr *peer_addr, socklen_t peer_addr_len);

#endif /* PROTOCOL_H_20190710_174035 */
