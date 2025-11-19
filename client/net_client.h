#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include "../common/net_protocol.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
    typedef int SOCKET;
#endif

int net_connect(const char *ip);
void net_send(Packet *pkt);
int net_receive(Packet *pkt);
void net_close();

#endif