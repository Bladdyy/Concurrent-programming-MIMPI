/**
 * This file is for implementation of mimpirun program.
 * */

#include "mimpi_common.h"
#include "channel.h"

#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>

// Zamyka niepotrzebne dekryptory.
void close_desc(int num, int size){
    int dir;
    for (int i = 0; i < size; i++){
        if (i != num){
            dir = 23 + size * 4 + (size - 1) * 2 * i;
            // Zamyka wszystkie deskryptory do wpisywania innych procesów.
            for (int j = 0; j < size - 1; j++){
                ASSERT_ZERO(close(dir + 2 * j));
            }
        }
        dir = 22 + size * 4 + (size - 1) * 2 * i;
        int dodge;  // Deskryptory aktualnego procesu do czytania,
        if (i == num){
            dodge = -5;
        }
        else{
            dodge = 22 + size * 4 + num * 2 + i * (size - 1) * 2;
            if (i < num){
                dodge -= 2;
            }
        }
        for (int y = 0; y < size - 1; y++){  // Zamyka deskryptory do czytania innych procesów.
            if (dir + y * 2 != dodge){
                ASSERT_ZERO(close(dir + y * 2));
            }
        }
    }
}
int main(int argc, char** argv) {
    if (argc < 3){  // Program dostał mniej, niż 2 argumenty.
        return -1;
    }
    int n = atoi(argv[1]);  // Ilość procesów do odpalenia.

    // Zmienna środowiskowa z ilością procesów odpalonych.
    ASSERT_ZERO(setenv("WORLD_SIZE",argv[1], 0));

    int desc[20];  // Deskyptory, które należy zamknąć po otwarciu potrzebnych.
    int pipefd[2];
    pipefd[0] = 0;
    pipefd[1] = 0;
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
    for (int i = 0; i < n * (n + 1) + 1; i++) {
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
    in[n] = n;  // Ile procesów w systemie.
    // Umieszcza tablicę procesów w pierwszym pipie.
    ASSERT_SYS_OK(chsend(21, in, sizeof(int) * (n + 1)));
    free(in);

    // Inicjalizacja nowych procesów.
    for (int i = 0; i < n; i++){
        pid_t pid = fork();
        ASSERT_SYS_OK(pid);

        if (!pid){
            close_desc(i, n);  // Zamyka deskryptory niepotrzebne temu dziecku.
            char val[64];
            sprintf(val, "%d", i);
            // Zmienna środowiskowa informująca o rank procesu.
            ASSERT_ZERO(setenv("MY_RANK",val, 0));
            sprintf(val, "%d", 0);
            // Zmienna środowiskowa informująca o tym, czy proces jest w bloku MIMPI.
            ASSERT_ZERO(setenv("ENTERED",val, 0));
            ASSERT_SYS_OK(execvp(argv[2], &argv[2])); // Odpalanie nowego procesu.
        }
    }

    // Oczekiwanie na zakończenie wszystkich procesów potomnych.
    for (int i = 0; i < n; i++) {
        ASSERT_SYS_OK(wait(NULL));
    }

    // Zamykanie otwartych deskryptorów.
    for (int i = 0; i < 2 * (n * (n + 1) + 1); i++){
        ASSERT_ZERO(close(i + 20));
    }

}