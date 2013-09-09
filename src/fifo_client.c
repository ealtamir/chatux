#include <fcntl.h>
#include <signal.h>
#include "../lib/fifo_server.h"
#include "../lib/common_headers.h"
#include "../lib/error_functions.h"
#include "../lib/helpers.h"


#define INFINITE_LOOP       1
#define INPUT_BUFFER_SIZE   100

int main(int argc, const char *argv[])
{
    if (argc > 1 && strcmp(argv[1], "--help") == 0)
        usageErr("%s string\n", argv[0]);

    Request cl_request;
    Response sv_response;
    char client_info[CLIENT_FIFO_SIZE];
    char input_buff[INPUT_BUFFER_SIZE];
    char* req_buffer = 0;
    int dummyfd;
    int serverfd, clientfd;
    int request_num = 0;

    pid_t pid = getpid();
    snprintf(client_info, CLIENT_FIFO_SIZE,
            CLIENT_FIFO_TEMPLATE, (long int) pid);

    clientfd = open(client_info, O_RDONLY);
    if (clientfd == -1)
        errExit("failed to open created FIFO");

    // Open dummy FIFO write to prevent seeing EOF.
    dummyfd = open(client_info, O_WRONLY);
    if (dummyfd == -1)
        errExit("failed to open dummy FIFO writer");

    while (INFINITE_LOOP) {

        cl_request.pid = pid;
        cl_request.data_len = getUserInput(input_buff, INPUT_BUFFER_SIZE);

        if (cl_request.data_len == 1)
            continue;

        serverfd = open(SERVER_PATHNAME, O_WRONLY);
        if (serverfd == -1)
            errExit("Couldn't open server FIFO at: %s", SERVER_PATHNAME);

        if (write(serverfd, &cl_request, sizeof(Request))
                != sizeof(Request))
            errExit("client write failed");
        if (write(serverfd, input_buff, cl_request.data_len)
                != cl_request.data_len)
            errExit("client write failed");

        close(serverfd);

        if (read(clientfd, &sv_response, sizeof(Response))
                != sizeof(Response))
            errExit("client fifo read failed");
        if (read(clientfd, input_buff, sv_response.data_len)
                != sv_response.data_len)
            errExit("client fifo read failed");

        printf("response from server: %s\n", input_buff);
    }
}
