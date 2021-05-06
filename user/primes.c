#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
bloom_filter(int *left_pipe) {
    int base;
    if(read(left_pipe[0], &base, sizeof(base)) > 0) {
        fprintf(1, "prime %d\n", base);

        int number;
        int right_pipe[2];
        if(pipe(right_pipe) < 0) {
            fprintf(2, "create pipe error\n");
            exit(1);
        }

        if(fork() == 0) {
            close(right_pipe[1]);
            bloom_filter(right_pipe);
        } else {
            close(right_pipe[0]);
            while (read(left_pipe[0], &number, sizeof(number)) > 0) {
                if(number % base != 0) {
                    write(right_pipe[1], &number, sizeof(number));
                }
            }
            close(right_pipe[1]);
            wait(0);
        }
    }
    exit(0);
}

int
main(int argc, char *argv[])
{
    int main_pipe[2];

    if(pipe(main_pipe) < 0) {
        fprintf(2, "create pipe error\n");
        exit(1);
    }

    if(fork() == 0) {
        close(main_pipe[1]);
        bloom_filter(main_pipe);
    } else {
        close(main_pipe[0]);
        for(int number = 2; number <= 35; number++) {
            write(main_pipe[1], &number, sizeof(number));
        }
        close(main_pipe[1]);
        wait(0);
        exit(0);
    }
    return 0;
}