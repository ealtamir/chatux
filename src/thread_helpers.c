#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>

#include "../lib/common_headers.h"
#include "../lib/dispatcher.h"
#include "../lib/error_functions.h"
#include "../lib/get_num.h"
#include "../lib/helpers.h"
#include "../lib/server.h"
#include "../lib/thread_helpers.h"

#define     TELNET_ENCODE       5
#define     TELNET_END_CHARS    "\r\n"

int createThreadFifo(int thread_id, int *thr_fifo_fd, int *dummy_fd);
int getDataFromServer(int thr_fifo, void **svr_data);
int readClientData(int cfd, char *data_buffer, int data_len);
int sendDataToClient(ThreadData *td, void *svr_data);
int sendDataToServer(char *client_data, int data_len, int thread_id);
int sendResponse(ThreadData *td, int thr_fifo);
int serveRequest(ThreadData *td, int thr_fifo);
int startIOListener(ThreadData *td, int thr_fifo, int dummy_fd);
void freeThreadData(ThreadData *td);

pthread_mutex_t fifo_mtx = PTHREAD_MUTEX_INITIALIZER;


void* toThreadDelegator(void *args) {

    ThreadData *td = (ThreadData*) args;
    int val = -1;
    int thr_fifo = 0;
    int dummy_fd = 0;

    fprintf(stdout, "Thread created - (%s, %s)\n", td->host, td->service);

    val = createThreadFifo(td->thread_id, &thr_fifo, &dummy_fd);
    if (val == -1) {
        errExitEN(errno, "Dispatcher: thr fifo fd was -1.");
    }

    val = startIOListener(td, thr_fifo, dummy_fd);
    if (val == -1) {
        // Some error checking code.
    }

    //
    // Cleaning operations
    //
    close(td->cfd);         // Close client socket
    close(thr_fifo);
    close(dummy_fd);
    freeThreadData(td);

    return NULL;
}

int createThreadFifo(int thread_id, int *thr_fifo_fd, int *dummy_fd) {

    int var = 0;
    char thr_fifo_pname[MAX_FIFO_N_LEN];

    snprintf(thr_fifo_pname, MAX_FIFO_N_LEN, T_FIFO_PATH, thread_id);
    fprintf(stdout, "Dispatcher: thread fifo pathname - %s\n", thr_fifo_pname);

    remove(thr_fifo_pname);
    var = mkfifo(thr_fifo_pname, S_IRUSR | S_IWUSR | S_IWGRP);
    if (var == -1) {
        errExitEN(errno, "Dispatcher: Thread couldn't CREATE the fifo.");
    }
    *thr_fifo_fd = open(thr_fifo_pname, O_RDONLY | O_NONBLOCK);
    if (*thr_fifo_fd == -1) {
        errExitEN(errno, "Dispatcher: Thread couldn't OPEN the fifo.");
    }
    *dummy_fd = open(thr_fifo_pname, O_WRONLY);
    if (*dummy_fd == -1) {
        errExitEN(errno, "Dispatcher: Thread couldn't OPEN dummy fifo writer.");
    }

    return var;
}

void freeThreadData(ThreadData *td) {
    free(td->host);
    free(td->service);
    free(td);
}

int readClientData(int cfd, char *data_buffer, int data_len) {

    int index;
    int val = -1;

    for(index = 0; val != 0 && index < data_len; index++) {

        val = read(cfd, &data_buffer[index], 1);

        if (val == 1) {
            if (data_buffer[index] == '\n') {
                data_buffer[index] = '\0';
                break;
            }
        } else if (val == -1) {
            errMsg("Thread failed in a read operation.");
            return -1;
        }
    }

    return index;
}

int getDataFromServer(int thr_fifo, void **svr_data) {

    int val = -1;
    ThreadMsgHeader t_header;

    fprintf(stdout, "Dispatcher: Start reading from FIFO.\n");

    setBlocking(thr_fifo);

    val = read(thr_fifo, &t_header, sizeof(ThreadMsgHeader));
    if (val != sizeof(ThreadMsgHeader)) {
        errExitEN(errno, "Dispatcher: Error when thread tried to\
                read HEADER from fifo: header size mismatch.\
                read: %d, expected: %d", val, sizeof(ThreadMsgHeader));
    }


    *svr_data = malloc(t_header.msg_size + TELNET_ENCODE);
    val = read(thr_fifo, *svr_data, t_header.msg_size);
    if (val != t_header.msg_size) {
        free(svr_data);
        errExitEN(errno, "Dispatcher: Error when thread tried to read MSG from fifo: data size mismatch.");
    }

    fprintf(stdout, "Dispatcher: Data from server read - %s\n", (char*) *svr_data);


    return 0;
}

int sendDataToServer(char *client_data, int data_len, int thread_id) {

    ThreadMsgHeader t_header;
    int fifo_fd = -1;
    ssize_t val = -1;

    fifo_fd = open(SERVER_FIFO_PATHNAME, O_WRONLY);
    if (fifo_fd == -1) {
        errMsg("Error when opening server FIFO.");
        return -1;
    }

    t_header.msg_size = data_len;
    t_header.thread_id = thread_id;

    //
    // Lock the mutex.
    //
    pthread_mutex_lock(&fifo_mtx);

    fprintf(stdout, "Dispatcher: Writing data to server.\n");

    val = write(fifo_fd, &t_header, sizeof(ThreadMsgHeader));
    if (val != sizeof(ThreadMsgHeader)) {
        errMsg("Error when writing header to server FIFO: size mismatch.");
        pthread_mutex_unlock(&fifo_mtx); // If failure unlock mutex.
        return -1;
    }

    val = write(fifo_fd, client_data, data_len);
    if (val != data_len) {
        errMsg("Error when writing client data to server FIFO: size mismatch.");
        pthread_mutex_unlock(&fifo_mtx); // If failure unlock mutex.
        return -1;
    }

    //
    // Unlock the mutex.
    //
    pthread_mutex_unlock(&fifo_mtx);

    fprintf(stdout, "Dispatcher: Data sent to server.\n");

    return 0;
}

int sendDataToClient(ThreadData *td, void *svr_data) {
    int val = -1;
    int len = 0;

    fprintf(stdout, "Dispatcher: Sending data to client\n");

    strcat(svr_data, TELNET_END_CHARS);

    //fprintf(stdout, "Dispatcher: Sending to client - %s\n", (char*) svr_data);

    len = strlen((char*) svr_data);
    val = write(td->cfd, svr_data, len);
    if (val != len) {
        errExitEN(errno, "Dispatcher: Thread couldn't send message to client: data size mismatch");
    }

    fprintf(stdout, "Dispatcher: Data sent to client...\n");

    return 0;
}

int startIOListener(ThreadData *td, int thr_fifo, int dummy_fd) {

    fd_set readfds;
    int ready = -1;
    int nfds = 0;
    int val = 0;

    fprintf(stdout, "Dispatcher: Starting service loop...\n");

    for(;;) {
        FD_ZERO(&readfds);
        FD_SET(td->cfd, &readfds);
        FD_SET(thr_fifo, &readfds);

        nfds = max(td->cfd, thr_fifo) + 1;

        ready = select(nfds, &readfds, NULL, NULL, NULL);
        if (ready == -1) {
            errMsg("Error when using select() inside a thread.");
            break;
        }

        if (FD_ISSET(td->cfd, &readfds)) {
            val = serveRequest(td, thr_fifo);
        }
        if (FD_ISSET(thr_fifo, &readfds)) {
            val = sendResponse(td, thr_fifo);
        }

    }

    return (ready == -1)? -1: 0;
}

int serveRequest(ThreadData *td, int thr_fifo) {

    char client_data[MAX_DATA_SIZE];
    int data_len = 0;
    int val = -1;
    int svr_fifo = 0;

    val = readClientData(td->cfd, client_data, MAX_DATA_SIZE);
    if (val == -1)
        errExitEN(errno, "Error when reading data from a client");

    fprintf(stdout, "val: %d, cfd: %d.\n", val, td->cfd);

    // Create thread fifo before sending data to server.

    if (val != -1) {
        data_len = strlen(client_data);
        val = sendDataToServer(client_data, data_len, td->thread_id);
        printf("Dispatcher: Data sent to server: %s", client_data);

        if (val == -1)
            errExitEN(errno, "Error while sending data to server.");
    }

    return 0;
}

int sendResponse(ThreadData *td, int thr_fifo) {

    int val = 0;
    void* svr_data = NULL;

    val = getDataFromServer(thr_fifo, &svr_data);
    if (val == -1) {
        errMsg("Dispatcher: Error when getting data from server.");
    }

    if (val != -1) {
        val = sendDataToClient(td, svr_data);
        if (val != 0)
            errMsg("Error when sending data to client.");
    }

    return 0;
}
