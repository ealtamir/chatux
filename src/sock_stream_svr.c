#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../lib/common_headers.h"
#include "../lib/error_functions.h"

#define IP_ADDR     "127.0.0.1"

//struct in_addr {        /* IPv4 4-byte address */
//    in_addr_t s_addr;   /* Unsigned 32-bit integer */
//}
//
//struct sockaddr_in {
//  sa_family_t     sin_family;
//  in_port_t       sin_port;
//  struct in_addr  sin_addr;
//  unsigned char   __pad[X];
//}

int main(int argc, const char *argv[])
{
    int len     = sizeof(struct sockaddr_in);
    int result  = 0;
    int sfd     = -1;
    struct in_addr ip_addr;
    struct sockaddr_in sockaddr, csockaddr;

    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1)
        errExitEN(errno, "Error when initializing server socket.");

    memset(&sockaddr, 0, len);
    ip_addr.s_addr = htonl(INADDR_LOOPBACK);
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htonl(INADDR_ANY);
    sockaddr.sin_addr = ip_addr;

    result = bind(sfd, (struct sockaddr *) &sockaddr, len);
    if (result == -1)
        errExitEN(errno, "Error when trying to bind socket to addrs.");

    result = listen(sfd, SOMAXCONN);
    if (result == -1)
        errExitEN(errno, "Error when trying to listen to the socket.");

    result = accept(sfd, (struct sockaddr *) &csockaddr, &len);
    if (result == -1)
        errExitEN(errno, "Error when trying to an incoming connection.");

    printf("Connection Accepted.\n");

    close(sfd);

    return 0;
}
