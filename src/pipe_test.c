#include <sys/wait.h>
#include "auxheader.h"

#define BUF_SIZE 10

int main(int argc, const char *argv[])
{
    int pdf[2];
    char buf[BUF_SIZE];
    ssize_t numRead;

    if (argc != 2 || strcmp(argv[1], "--help") == 0)
        usageErr("%s string\n", argv[0]);

    if (pipe(pfd) == -1)
        errExit("pipe");
    return 0;
}
