#include <netinet/in.h>
#include <sys/socket.h>
#include <signal.h>
#include "common_headers.h"          /* Declaration of readLine() */
#include "error_functions.h"

#define PORT_NUM "50000"        /* Port number for server */

#define INT_LEN 30              /* Size of string able to hold largest integer (including terminating '\n') */




