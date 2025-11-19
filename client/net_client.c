#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "net_client.h"

SOCKET sock = INVALID_SOCKET;

int net_connect(const char *ip) {
    // Si déjà connecté, on renvoie OK direct
    if (sock != INVALID_SOCKET) return 1;

    #ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return 0;
    #endif

    struct sockaddr_in serv_addr;
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) return 0;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) return 0;

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        net_close();
        return 0;
    }
    
    // Mode Non-Bloquant
    #ifdef _WIN32
        unsigned long mode = 1;
        ioctlsocket(sock, FIONBIO, &mode);
    #else
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    #endif
    
    return 1;
}

void net_send(Packet *pkt) {
    if (sock != INVALID_SOCKET) {
        send(sock, (const char*)pkt, sizeof(Packet), 0);
    }
}

int net_receive(Packet *pkt) {
    if (sock == INVALID_SOCKET) return 0;
    
    // On s'assure que la mémoire est propre
    memset(pkt, 0, sizeof(Packet));
    
    int len = recv(sock, (char*)pkt, sizeof(Packet), 0);
    if (len > 0) return 1;
    return 0;
}

void net_close() {
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
    #ifdef _WIN32
    WSACleanup();
    #endif
}