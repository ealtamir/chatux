#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>

#include "../lib/common_headers.h"
#include "../lib/dispatcher.h"
#include "../lib/error_functions.h"
#include "../lib/get_num.h"
#include "../lib/server.h"
#include "../lib/thread_helpers.h"

void freeThreadData(ThreadData *td);
void* toThreadDelegator(void *args);
int delegateRequest(int fd, const struct sockaddr *sa, socklen_t salen, int thread_id);
int startListenSock(const char *service, socklen_t *addrlen, int backlog);
int startPassiveSocket(const char *service, int type,
        socklen_t *addrlen, Boolean setListen, int backlog);


int main(int argc, const char *argv[])
{
    int cfd     = -1;
    int len     = 0;
    int optval  = 0;
    int sfd     = -1;
    int thread_id = 0;
    socklen_t addrlen = 0;
    struct sockaddr_storage claddr;

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        errExitEN(errno, "Dispatcher couldn't ignore SIGPIPE signal.");

    sfd = startListenSock(PORT_NUM, &addrlen, BACKLOG);
    if (sfd == -1) {
        errMsg("Dispatcher couldn't bind socket.");
        kill(getppid(), SIGTERM);
        return -1;
    }

    for(;;) {
        addrlen = sizeof(struct sockaddr_storage);
        cfd = accept(sfd, (struct sockaddr *) &claddr, &addrlen);
        if (cfd == -1) {
            errMsg("Error when accepting the client address.");
            continue;
        }

        // Serve request in a new thread.
        // Thread should close its own cfd.
        delegateRequest(cfd, (struct sockaddr *) &claddr, addrlen, thread_id);
        fprintf(stdout, "New request delegated.\n");

        thread_id++;
    }

    close(sfd);

    return 0;
}

int delegateRequest(int fd, const struct sockaddr *sa, socklen_t salen, int thread_id) {

    char *host      = NULL;
    char *service   = NULL;
    int val         = -1;
    pthread_t new_thread;
    ThreadData *args = NULL;

    //
    // These variables are freed by the threads.
    //
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
    args->thread_id = thread_id;

    val = pthread_create(&new_thread, NULL, &toThreadDelegator, args);
    if (val > 0) {
        errMsg("There was an error in the creation of the thread: %d", val);
        return -1;
    }

    printf("New thread initialized.\n");
    pthread_detach(new_thread);

    return 0;
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
        printf("Trying to bind socket...\n");
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
