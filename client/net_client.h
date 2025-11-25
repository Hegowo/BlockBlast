#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include "../common/net_protocol.h"

/* Connect to server */
int net_connect(const char *ip, int port);

/* Send packet to server */
void net_send(Packet *pkt);

/* Receive packet from server (non-blocking) */
int net_receive(Packet *pkt);

/* Close connection */
void net_close(void);

/* Check if connected */
int net_is_connected(void);

#endif

