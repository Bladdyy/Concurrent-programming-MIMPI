/**
 * This file is for implementation of mimpirun program.
 * */

#include "mimpi_common.h"
#include "channel.h"

#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char** argv) {
    if (argc < 3){  // Program dostał mniej, niż 2 argumenty.
        return -1;
    }
    int n = atoi(argv[1]);  // Ilość procesów do odpalenia.

    // Zmienna środowiskowa z ilością procesów odpalonych.
    ASSERT_ZERO(setenv("WORLD_SIZE",argv[1], 0));

    int desc[20];  // Deskyptory, które należy zamknąć po otwarciu potrzebnych.
    int pipefd[2];
    int index = 0;

    while (pipefd[1] < 19){  // Otwieranie niedozwolonych deskryptorów.
        ASSERT_ZERO(channel(pipefd));
        desc[2 * index] = pipefd[0];
        desc[(2 * index) + 1] = pipefd[1];
        index++;
    }
    index = (index) * 2;

    // Zamykanie nadliczbowo otwartych dobrych.
    if (pipefd[1] > 19){
        ASSERT_ZERO(close(pipefd[1]));
        index--;
    }
    if (pipefd[0] > 19){
        ASSERT_ZERO(close(pipefd[0]));
        index--;
    }

    // Otwieranie dobrych deskryptorów do komunikacji między składowymi MIMPI oraz procesami.
    for (int i = 0; i < n * (n + 1) + 2; i++) {
        ASSERT_ZERO(channel(pipefd));
    }

    // Zamykanie poprzednio otwartych złych deskryptorów.
    for (int i = 0; i < index; i++){
        ASSERT_ZERO(close(desc[i]));
    }

    // Tablica procesów, które wyszły z MIMPI.
    int* in = malloc(sizeof(int) * (n + 1));
    for (int i = 0; i < n; i++){
        in[i] = 1;
    }
    in[n] = n;
    // Umieszcza tablicę procesów w pierwszym pipie.
    ASSERT_SYS_OK(chsend(21, in, sizeof(int) * (n + 1)));
    free(in);
    // Umieszcza liczbę określającą ilość procesów w procedurze grupowej w drugim pipie.
    int* barrier = malloc(sizeof(int));
    *barrier = 0;
    ASSERT_SYS_OK(chsend(23, barrier, sizeof(int)));
    free(barrier);
    // Inicjalizacja nowych procesów.
    for (int i = 0; i < n; i++){
        pid_t pid = fork();
        ASSERT_SYS_OK(pid);
        if (!pid){
            char val[64];
            sprintf(val, "%d", i);
            // Zmienna środowiskowa informująca o rank procesu.
            ASSERT_ZERO(setenv("MY_RANK",val, 0));
            sprintf(val, "%d", 0);
            // Zmienna środowiskowa informująca o tym, czy proces jest w bloku MIMPI.
            ASSERT_ZERO(setenv("ENTERED",val, 0));
            char* args[argc - 1]; // Argumenty procesów odpalanych.
            for (int x = 0; x < argc - 2; x++){
                args[x] = argv[x + 2];
            }
            args[argc - 2] = NULL;
            ASSERT_SYS_OK(execv(argv[2], args)); // Odpalanie nowego procesu.
        }
    }

    // Oczekiwanie na zakończenie wszystkich procesów potomnych.
    for (int i = 0; i < n; i++) {
        ASSERT_SYS_OK(wait(NULL));
    }
    // Zamykanie otwartych deskryptorów.
    for (int i = 0; i < 2 * (n * (n + 1) + 2); i++){
        ASSERT_ZERO(close(i + 20));
    }
}