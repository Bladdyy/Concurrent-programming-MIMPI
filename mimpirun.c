/**
 * This file is for implementation of mimpirun program.
 * */

#include "mimpi_common.h"
#include "channel.h"

#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>

int main(int argc, char** argv) {
    if (argc < 3){  // Program dostał mniej, niż 2 argumenty.
        return -1;
    }
    int n = atoi(argv[1]);  // Ilość programów do odpalenia.
    ASSERT_ZERO(setenv("WORLD_SIZE",argv[1], 0));
    int desc[20];
    int pipefd[2];
    int index = 0;
    pipefd[0] = 0;
    pipefd[1] = 1;
    while (pipefd[1] < 19){
        ASSERT_ZERO(channel(pipefd));
        desc[2 * index] = pipefd[0];
        desc[(2 * index) + 1] = pipefd[1];
        index++;
    }
    index = (index) * 2;
    if (pipefd[1] > 19){
        ASSERT_ZERO(close(pipefd[1]));
        index--;
    }
    if (pipefd[0] > 19){
        ASSERT_ZERO(close(pipefd[0]));
        index--;
    }
    for (int i = 0; i < n * (n - 1) + 1; i++){
        ASSERT_ZERO(channel(pipefd));
        printf("%d %d\n", pipefd[0], pipefd[1]);
    }
    for (int i = 0; i < index; i++){
        ASSERT_ZERO(close(desc[i]));
    }
    int* in = malloc(sizeof(int) * n);
    for (int i = 0; i < n; i++){
        in[i] = 1;
    }
    printf("wpisuje %d\n", 2 * n * (n - 1) + 20 + 1);
    ASSERT_SYS_OK(chsend(2 * n * (n - 1) + 20 + 1, in, sizeof(int) * n));
    for (int i = 0; i < n; i++){
        pid_t pid = fork();
        ASSERT_SYS_OK(pid);
        if (!pid){
            char val[64];
            sprintf(val, "%d", i);
            ASSERT_ZERO(setenv("MY_RANK",val, 0));
            sprintf(val, "%d", 0);
            ASSERT_ZERO(setenv("ENTERED",val, 0));
            char* args[argc - 1];
            for (int x = 0; x < argc - 2; x++){
                args[x] = argv[x + 2];
            }
            args[argc - 2] = NULL;
            ASSERT_SYS_OK(execv(argv[2], args));
        }
    }
    for (int i = 0; i < n; i++) {
        ASSERT_SYS_OK(wait(NULL));
    }
    for (int i = 0; i < 2 * n * (n - 1) + 2; i++){
        ASSERT_ZERO(close(i + 20));
    }
}