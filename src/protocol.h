#ifndef PROTOCOL_H_20190710_174035
#define PROTOCOL_H_20190710_174035

extern const int DOMAIN;
extern const int TYPE;
extern const int PROTOCOL;

typedef uint16_t message_t;
extern const message_t DH_INIT;
extern const message_t DH_INIT_REPLY;
extern const message_t DH_CLI_COMB;
extern const message_t DH_CLI_COMB_REPLY;
extern const message_t DH_SERV_COMB;
extern const message_t DH_SERV_COMB_REPLY;
extern const message_t DH_RESET;
extern const message_t CLOSE_CONNECTION;

struct prot_head {
    uint32_t md5[4];
    message_t type;
    uint16_t length;
};

int dhkx_client (int sock);
int dhkx_server (int sock, ssize_t len, struct sockaddr *peer_addr,
    socklen_t peer_addr_len);

#endif /* PROTOCOL_H_20190710_174035 */
