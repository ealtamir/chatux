#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>

#include "../lib/common_headers.h"
#include "../lib/error_functions.h"
#include "../lib/get_num.h"

#define     PORT_NUM    "50000"
#define     BACKLOG     5
#define     ADDRSTRLEN  (NI_MAXHOST + NI_MAXSERV + 10)

typedef struct {
    int cfd;
    char *host;
    char *service;
    const struct sockaddr *sa;
    socklen_t salen;
} ThreadData;

void freeThreadData(ThreadData *td);
void* toThreadDelegator(void *args);
int delegateRequest(int fd, const struct sockaddr *sa, socklen_t salen);
int startListenSock(const char *service, socklen_t *addrlen, int backlog);
int startPassiveSocket(const char *service, int type,
        socklen_t *addrlen, Boolean setListen, int backlog);


int main(int argc, const char *argv[])
{
    int cfd     = -1;
    int len     = 0;
    int optval  = 0;
    int sfd     = -1;
    socklen_t addrlen = 0;
    struct sockaddr_storage claddr;

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        errExitEN(errno, "Server couldn't ignore SIGPIPE signal.");

    sfd = startListenSock(PORT_NUM, &addrlen, BACKLOG);
    if (sfd == -1)
        errExitEN(errno, "Server couldn't bind socket.");

    for(;;) {
        addrlen = sizeof(struct sockaddr_storage);
        cfd = accept(sfd, (struct sockaddr *) &claddr, &addrlen);
        if (cfd == -1) {
            errMsg("Error when accepting the client address.");
            continue;
        }

        // Serve request in a new thread.
        delegateRequest(cfd, (struct sockaddr *) &claddr, addrlen);

        close(cfd);
    }

    close(sfd);

    return 0;
}
int delegateRequest(int fd, const struct sockaddr *sa, socklen_t salen) {

    char *host      = NULL;
    char *service   = NULL;
    int val         = -1;
    pthread_t new_thread;
    ThreadData *args = NULL;

    host = malloc(NI_MAXHOST);
    service = malloc(NI_MAXSERV);

    if (!host && !service) {
        errMsg("Couldn't allocate memory to start a new thread.");
        return -1;
    }

    val = getnameinfo(sa, salen, host, NI_MAXHOST, service, NI_MAXSERV, 0);
    if (val != 0) {
        errMsg("Couldn't get client information from the socket address");
        return -1;
    }

    args = malloc(sizeof(ThreadData));
    args->cfd = fd;
    args->host = host;
    args->service = service;
    args->sa = sa;
    args->salen = salen;

    val = pthread_create(&new_thread, NULL, &toThreadDelegator, args);
    if (val > 0) {
        errMsg("There was an error in the creation of the thread: %d", val);
        return -1;
    }

    printf("New thread initialized.\n");
    pthread_detach(new_thread);

    return 0;
}

void* toThreadDelegator(void *args) {

    ThreadData *td = (ThreadData*) args;

    fprintf(stdout, "Thread created - (%s, %s)\n", td->host, td->service);

    freeThreadData(td);

    return NULL;
}

void freeThreadData(ThreadData *td) {
    free(td->host);
    free(td->service);
    free(td);
}

int
startListenSock(const char *service, socklen_t *addrlen, int backlog) {
    return startPassiveSocket(service, SOCK_STREAM, addrlen, TRUE, backlog);
}

int
startPassiveSocket(const char *service, int type,
        socklen_t *addrlen, Boolean setListen, int backlog) {

    struct addrinfo hints;
    struct addrinfo *result;
    struct addrinfo *rp;
    int sfd     = -1;
    int val     = 0;
    int optval  = 1;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_addr       = NULL;
    hints.ai_next       = NULL;
    hints.ai_canonname  = NULL;
    hints.ai_addrlen    = 0;
    hints.ai_protocol   = 0;
    hints.ai_family     = AF_UNSPEC;
    hints.ai_flags      = AI_PASSIVE | AI_NUMERICSERV;
    hints.ai_socktype   = type;

    val = getaddrinfo(NULL, service, &hints, &result);
    if (val != 0)
        errExitEN(errno, "Call to getaddrinfo() was unsuccessful.");

    for(rp = result; rp != NULL; rp = rp->ai_next) {
        printf("reading...\n");
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1)
            continue;

        if (setListen == TRUE) {
            // Set socket options, like allowing its reuse.
            // optval is used as a buffer where these values
            // are returned.
            // THIS IS NEEDED TO CREATE THIS A LISTENING SERVER.
            val = setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
            if (val == -1) {
                errMsg("Couldn't set socket options.");
                close(sfd);
                freeaddrinfo(result);
                return -1;
            }
        }

        val = bind(sfd, rp->ai_addr, rp->ai_addrlen);
        if (val == 0)
            break;

        close(sfd); // bind() failed
        printf("Unsuccessful read to structure.\n");
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
