#include <sys/socket.h>
#include <netdb.h>

#include "../lib/common_headers.h"
#include "../lib/error_functions.h"
#include "../lib/get_num.h"

#define     PORT_NUM    "50000"
#define     BACKLOG     5
#define     ADDRSTRLEN  (NI_MAXHOST + NI_MAXSERV + 10)

int main(int argc, const char *argv[])
{

    char addrStr[ADDRSTRLEN];
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];
    int addrlen = 0;
    int len = 0;
    int lfd, cfd;
    int optval = 0;
    struct addrinfo *result;
    struct addrinfo *rp;
    struct addrinfo hints;
    struct sockaddr_storage claddr;
    lfd = cfd = -1;

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        errExit("signal");


    len = sizeof(struct addrinfo);
    memset(&hints, 0, len);
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    if (getaddrinfo(NULL, PORT_NUM, &hints, &result) != 0)
        errExit("getaddrinfo");

    optval = 1;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        lfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (lfd == -1)
            continue;

        if (setsockopt(lfd, SOL_SOCKET,
                    SO_REUSEADDR, &optval, sizeof(optval)) == -1)
            errExit("setsockopt");

        if (bind(lfd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;

        close(lfd);
    }

    if (rp == NULL)
        fatal("Could not bind socket to any address");

    if (listen(lfd, BACKLOG) == -1)
        errExit("listen");

    freeaddrinfo(result);

    for(;;) {
        addrlen = sizeof(struct sockaddr_storage);
        cfd = accept(lfd, (struct sockaddr *) &claddr, &addrlen);
        if (cfd == -1) {
            errMsg("accept");
            continue;
        }

        if (getnameinfo((struct sockaddr *) &claddr, addrlen,
                    host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0) {
            snprintf(addrStr, ADDRSTRLEN, "(%s, %s)", host, service);

        } else {
            snprintf(addrStr, ADDRSTRLEN, "(?UNKNOWN?)");
        }
        printf("Connection from %s\n", addrStr);

        break;
    }

    close(cfd);
    close(lfd);

    return 0;
}
