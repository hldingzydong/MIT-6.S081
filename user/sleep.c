#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    if(argc < 2) {
        fprintf(2, "Usage: sleep Xms...\n");
        exit(1);
    }

    int time;
    time = atoi(argv[1]);
    if(sleep(time) < 0) {
        fprintf(2, "sleep: failed to sleep %d seconds...\n", time);
    }
    exit(0);
}