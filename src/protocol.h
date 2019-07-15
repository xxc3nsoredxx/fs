#ifndef PROTOCOL_H_20190710_174035
#define PROTOCOL_H_20190710_174035

/* Max packet data length */
#define ONE_K 1024
#define MAX_LEN (ONE_K * 15)
/* Hash algorithm output length */
#define HASH_LEN 20

typedef uint16_t message_t;
typedef uint32_t param_t;
struct prot_head {
    uint32_t blake2[5];
    message_t type;
    uint16_t length;
    uint16_t stop;
    uint16_t segment;
};

extern const int DOMAIN;
extern const int TYPE;
extern const int PROTOCOL;

extern const int DATA_MULT;

extern const message_t KEY;
extern const message_t KEY_REPLY;
extern const message_t KEY_GOOD;
extern const message_t KEY_RESET;

extern const message_t REQUEST_DATA;
extern const message_t SEND_DATA;

extern const message_t CLOSE_CONNECTION;

extern const param_t DIR_INFO;
extern const param_t FILE_CONTENTS;

extern struct prot_head head;
extern char data[];
extern char packet[];

extern struct sockaddr_storage peer_addr;
extern socklen_t peer_addr_len;

extern int key;
extern gcry_md_hd_t hd;
extern const int ALGO;
extern const int FLAGS;

ssize_t get_packet_c (int sock);
ssize_t get_packet_s (int sock, char *host, char *service);

ssize_t send_packet_c (int sock);
ssize_t send_packet_s (int sock);

int kx_client (int sock);
int kx_server (int sock);

gcry_error_t setup_gcrypt ();

ssize_t request_data (int sock, param_t param1, const void *param2);
ssize_t send_data (int sock, void *d, size_t len);

void handle_request_data (int sock);

#endif /* PROTOCOL_H_20190710_174035 */
