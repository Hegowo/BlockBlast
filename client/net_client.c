#include <stdio.h>
#include <string.h>
#include "net_client.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    #include <fcntl.h>
    #include <errno.h>
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
    typedef int SOCKET;
#endif

static SOCKET sock = INVALID_SOCKET;

#ifdef _WIN32
static int wsa_initialized = 0;
#endif

int net_connect(const char *ip, int port) {
    struct sockaddr_in server_addr;
    
#ifdef _WIN32
    WSADATA wsaData;
    u_long mode = 1;
    
    if (!wsa_initialized) {
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            printf("WSAStartup failed\n");
            return 0;
        }
        wsa_initialized = 1;
    }
#endif
    
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        return 0;
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((unsigned short)port);
    
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        printf("Invalid address: %s\n", ip);
        closesocket(sock);
        sock = INVALID_SOCKET;
        return 0;
    }
    
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Connection failed to %s:%d\n", ip, port);
        closesocket(sock);
        sock = INVALID_SOCKET;
        return 0;
    }
    
#ifdef _WIN32
    ioctlsocket(sock, FIONBIO, &mode);
#else
    {
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    }
#endif
    
    printf("Connected to %s:%d\n", ip, port);
    return 1;
}

void net_send(Packet *pkt) {
    if (sock == INVALID_SOCKET) {
        return;
    }
    
    send(sock, (const char *)pkt, sizeof(Packet), 0);
}

int net_receive(Packet *pkt) {
    int len;
    
    if (sock == INVALID_SOCKET) {
        return 0;
    }
    
    memset(pkt, 0, sizeof(Packet));
    
    len = recv(sock, (char *)pkt, sizeof(Packet), 0);
    
    if (len > 0) {
        return 1;
    }
    
#ifdef _WIN32
    if (WSAGetLastError() == WSAEWOULDBLOCK) {
        return 0;
    }
#else
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return 0;
    }
#endif
    
    if (len == 0 || len < 0) {
        return 0;
    }
    
    return 0;
}

void net_close(void) {
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
    
#ifdef _WIN32
    if (wsa_initialized) {
        WSACleanup();
        wsa_initialized = 0;
    }
#endif
}

int net_is_connected(void) {
    return sock != INVALID_SOCKET;
}
