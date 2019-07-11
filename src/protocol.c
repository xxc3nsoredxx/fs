#include <sys/socket.h>
#include <sys/types.h>

#include "protocol.h"

/* Socket domain: IPv4 */
const int DOMAIN = AF_INET;
/* Use UDP sockets */
const int TYPE = SOCK_DGRAM;
/* Determine protocol based on socket type */
const int PROTOCOL = 0;
