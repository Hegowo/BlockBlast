#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include "../common/net_protocol.h"

int net_connect(const char *ip, int port);

void net_send(Packet *pkt);

int net_receive(Packet *pkt);

void net_close(void);

int net_is_connected(void);

#endif

