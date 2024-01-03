/**
 * This file is for implementation of MIMPI library.
 * */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "channel.h"
#include "mimpi.h"
#include "mimpi_common.h"




void MIMPI_Init(bool enable_deadlock_detection) {
    channels_init();
    char* initalized = getenv("ENTERED");
    // Sprawdza, czy proces wchodził już do bloku MIMPI.
    if (initalized != NULL && atoi(initalized) == 0) {
        char val[4];
        sprintf(val, "%d", 1);
        setenv("ENTERED", val, 1);  // Udziela pozowolenia na wejście.
    }
}

void MIMPI_Finalize() {
    char* initalized = getenv("ENTERED");
    // Sprawdza, czy proces jest w bloku MIMPI.
    if (initalized != NULL && atoi(initalized) == 1) {
        int rank = MIMPI_World_rank();
        // Wysyła do pozostałych procesów wiadomość, że zakończył swój blok MIMPI.
        int *data = malloc(sizeof(int) * MIMPI_World_size());  // Tablica procesów w bloku MIMPI.
        ASSERT_SYS_OK(chrecv(20, data, sizeof(int) * MIMPI_World_size()));
        data[rank] = 0;  // Zaznacza w tablicy, że zakończył swój blok MIMPI.
        ASSERT_SYS_OK(chsend(21, data, sizeof(int) * MIMPI_World_size()));
        for (int i = 0; i < MIMPI_World_size(); i++){
            if (i != rank) {
                MIMPI_Send(NULL, 0, i, -1);
            }
        }
        char val[4];
        sprintf(val, "%d", -1);
        setenv("ENTERED", val, 1);  // Odbiera dostęp do funkcji MIMPI.

        channels_finalize();
    }
}

int MIMPI_World_size() {
    char* initalized = getenv("ENTERED");
    // Sprawdza, czy proces jest w bloku MIMPI.
    if (initalized != NULL && atoi(initalized) == 1) {
        char *size = getenv("WORLD_SIZE");
        if (size != NULL) {
            return atoi(size);  // Zwraca ile procesów zostało utworzonych.
        } else {
            return -3;
        }
    }
    else{
        return -5;
    }
}

int MIMPI_World_rank() {
    char* initalized = getenv("ENTERED");
    // Sprawdza, czy proces jest w bloku MIMPI.
    if (initalized != NULL && atoi(initalized) == 1) {
        char* rank = getenv("MY_RANK");  // Zwraca wartość rank procesu sprawdzającego.
        if (rank != NULL){
            return atoi(rank);
        }
        else{
            return -4;
        }
    }
    else{
        return -5;
    }
}

MIMPI_Retcode MIMPI_Send(
    void const *data,
    int count,
    int destination,
    int tag
) {
    char *initalized = getenv("ENTERED");
    if (destination == MIMPI_World_rank()) {  // Sprawdza, czy proces nie próbuje wysłać wiadomości do siebie.
        return 1;
    } else if (destination >= MIMPI_World_size()) {  // Sprawdza, czy proces nie próbuje wysłać wiadomości do nieistniejącego procesu.
        return 2;
    } else if (initalized != NULL && atoi(initalized) == 1) {  // Sprawdza, czy proces jest w bloku MIMPI.
        // Numer deskryptora, do którego zapisywana będzie wiadomość.
        int dir = 23 + MIMPI_World_size() * 4 + destination * 2 + MIMPI_World_rank() * (MIMPI_World_size() - 1) * 2;
        // Jeśli rank odbiorcy jest większy od rank nadawcy.
        if (destination > MIMPI_World_rank()) {
            dir -= 2;
        }
        if (tag == -1){  // Wiadomość wysyłana dotyczy zakończenia bloku MIMPI.
            int x = -1;
            ASSERT_SYS_OK(chsend(dir, &x, sizeof(int)));
            return 0;
        }
        else{
            int *in = malloc(sizeof(int) * MIMPI_World_size());  // Tablica procesów w bloku MIMPI.
            ASSERT_SYS_OK(chrecv(20, in, sizeof(int) * MIMPI_World_size()));
            ASSERT_SYS_OK(chsend(21, in, sizeof(int) * MIMPI_World_size()));
            if (in[destination] == 0) {  // Jeśli procesu już nie ma w bloku MIMPI.
                return 3;
            }
            void* msg = malloc(512);  // Wiadomość wysyłana.
            memcpy(msg, &tag, sizeof(int));  // Dodawanie tagu wiadomości.
            memcpy(msg + sizeof(int), data, sizeof(*data)); // Dodawanie treści wiadomości.
            ASSERT_SYS_OK(chsend(dir, msg, count + sizeof(int)));
            free(msg);
            free(in);
            return 0;
        }
    }
    else{
        return -5;
    }
}

MIMPI_Retcode MIMPI_Recv(
    void *data,
    int count,
    int source,
    int tag
) {
    char* initalized = getenv("ENTERED");
    if (source == MIMPI_World_rank()){  // Sprawdza, czy proces nie próbuje odczytać wiadomości do siebie.
        return 1;
    }
    else if (source >= MIMPI_World_size()){  // Sprawdza, czy proces nie próbuje odczytać wiadomości do nieistniejącego procesu.
        return 2;
    }
    else if (initalized != NULL && atoi(initalized) == 1) {  // Sprawdza, czy proces jest w bloku MIMPI.
        // Numer deskryptora, z którego będzie czytana wiadomość.
        int dir = 22 + MIMPI_World_size() * 4 + MIMPI_World_rank() * 2 + source * (MIMPI_World_size() - 1) * 2;
        // Jeśli rank odbiorcy jest większy od rank nadawcy.
        if (source < MIMPI_World_rank()) {
            dir -= 2;
        }
        int newtag;  // Tag odczytywanej wiadomości.
        void* buff = malloc(sizeof(int));
        ASSERT_SYS_OK(chrecv(dir, buff, sizeof(int)));
        memcpy(&newtag, buff, sizeof(int));
        if (newtag == -1){  // Jeśli tag wiadomości oznacza wyjście z bloku MIMPI.
            return 3;
        }
        ASSERT_SYS_OK(chrecv(dir, data, count));
        return 0;
    }
    else{
        return -5;
    }
}

MIMPI_Retcode MIMPI_Barrier() {
    TODO
}

MIMPI_Retcode MIMPI_Bcast(
    void *data,
    int count,
    int root
) {
    TODO
}

MIMPI_Retcode MIMPI_Reduce(
    void const *send_data,
    void *recv_data,
    int count,
    MIMPI_Op op,
    int root
) {
    TODO
}