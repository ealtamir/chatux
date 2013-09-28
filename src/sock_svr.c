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
        errExitEN(errno, "Server couldn't ignore SIGPIPE signal.");

    len = sizeof(struct addrinfo);
    memset(&hints, 0, len);
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    // getaddrinfo, returns a set of socket address structures
    // given a hostname.and service name.
    // result - contains the returned socket address.
    if (getaddrinfo(NULL, PORT_NUM, &hints, &result) != 0)
        errExit("getaddrinfo");

    // Try to connect to the hostname/port given by
    // each of the returned data structures.
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

    // Accept connections.
    if (listen(lfd, BACKLOG) == -1)
        errExit("listen");

    freeaddrinfo(result);

    for(;;) {
        addrlen = sizeof(struct sockaddr_storage);
        cfd = accept(lfd, (struct sockaddr *) &claddr, &addrlen);
        if (cfd == -1) {
            errMsg("Error when accepting the client address.");
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

int
connectToServer(const char *service, int type,
        socklen_t *addrlen, bool setListen, int backlog) {

    struct addrinfo hints;
    struct addrinfo *result;
    struct addrinfo *rp;
    int sfd     = -1;
    int val     = 0;
    int optval  = 0;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_addr       = NULL;
    hints.ai_next       = NULL;
    hints.ai_canonname  = NULL;
    hints.ai_addrlen    = 0;
    hints.ai_protocol   = 0;
    hints.ai_family     = AF_UNSPEC;
    hints.ai_flags      = AI_PASSIVE | AI_NUMERICSERV;
    hints.ai_socktype   = type;

    val = getaddrinfo(NULL, PORT_NUM, &hints, &result);
    if (val != 0)
        errExitEN(errno, "Call to getaddrinfo() was unsuccessful.");

    for(rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1)
            continue;

        if (setListen == TRUE) {
            // Set socket options, like allowing its reuse.
            // optval is used as a buffer where these values
            // are returned.
            // THIS IS NEEDED TO CREATE THIS A LISTENING SERVER.
            val = setsockopt(sfd, SOL_SOCKET,
                    SO_REUSEADDR, &optval, sizeof(optval));

            if (val != -1) {
                close(sfd);
                freeaddrinfo(result);
                return -1;
            }
        }

        val = bind(sfd, rp->ai_addr, rp->ai_addrlen);
        if (val == 0)
            break;

        close(sfd); // bind() failed
    }

    if (rp != NULL && setListen == TRUE) {
        val = listen(sfd, BACKLOG);
        if (val == -1) {
            freeaddrinfo(result);
            return -1;
        }
    }

    if (rp != NULL && addrlen != NULL)
        *addrlen = rp->ai_addrlen;

    freeaddrinfo(result);

    return (rp == NULL) ? -1 : sfd;
}
