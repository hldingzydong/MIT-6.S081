#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int p[2];
    char buf[10];

    // p[0] read, p[1] write
    pipe(p);

    if(fork() == 0) {
        read(p[0], buf, sizeof(buf));
        fprintf(1, "%d: received ping\n", getpid());
        write(p[1], "\n", 1);
    } else {
        write(p[1], "\n", 1);
        wait(0);
        read(p[0], buf, sizeof(buf));
        fprintf(1, "%d: received pong\n", getpid());
    }
    exit(0);
}