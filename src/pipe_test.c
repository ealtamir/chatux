#include "../lib/common_headers.h"
#include "../lib/error_functions.h"
#include <sys/wait.h>

#define     BUF_SIZE            10
#define     INPUT_BUF_SIZE      500

int main(int argc, const char *argv[])
{
    char buf[BUF_SIZE];
    char c = 0;
    char gen_buf[INPUT_BUF_SIZE];
    int index = 0;
    int pfd[2];
    ssize_t numRead;

    if (argc != 2 || strcmp(argv[1], "--help") == 0)
        usageErr("%s string\n", argv[0]);
    else
        printf("%s\n", argv[1]);

    if (pipe(pfd) == -1)
        errExit("pipe");

    switch (fork()) {
    case -1:
        errExit("fork");
    break;
    case 0: /* child gets its PID set to 0 */
        if (close(pfd[1]) == -1)
            errExit("close");

        while(1) {
            index = read(pfd[0], &gen_buf, INPUT_BUF_SIZE);
            gen_buf[index] = '\0';
            printf("Child read: %s\n", gen_buf);
        }

    break;
    default: /* parent */
        if (close(pfd[0]) == -1)
            errExit("close");

        while(1) {
            while((gen_buf[index] = getchar()) != '\n' &&
                    index < INPUT_BUF_SIZE) {
                write(pfd[1], &gen_buf[index], 1);
                index++;
            }
        }
    break;
    }

    return 0;
}
