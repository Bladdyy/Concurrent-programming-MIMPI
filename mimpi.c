/**
 * This file is for implementation of MIMPI library.
 * */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "channel.h"
#include "mimpi.h"
#include "mimpi_common.h"

struct Header{
    int count;
    int tag;
    struct Header* next;
    struct Header* prev;
    void* text;
};
typedef struct Header Header;

Header* roots[16];
Header* last[16];
int bar = 0;


// Tworzy nowy header.
Header* createHeader(int tag, int size, int source){
    Header *newHeader = malloc(sizeof(Header));  // Tworzy header do przechowywania wiadomości.
    newHeader->tag = tag;
    newHeader->count = size;
    newHeader->next = NULL;
    newHeader->prev = NULL;
    newHeader->text = malloc(size);
    if (roots[source] == NULL) {  // Jeśli nie jeszcze zapisanej wiadomości od tego źródła.
        roots[source] = newHeader;
        last[source] = newHeader;
    } else {
        last[source]->next = newHeader;
        newHeader->prev = last[source];
        last[source] = newHeader;
    }
    return newHeader;
}

// Szuka wiadomości o podanych wymaganiach.
Header* findtext(int source, int tag, int count){
    Header* temp = roots[source];
    while (temp != NULL && (tag != temp->tag || count != temp->count)) {
        temp = temp->next;
    }
    return temp;
}

// Uwalnia pamięc po headerze i usuwa go ze struktury danych.
void clearHeader(Header* header, int source){
    if (header->prev != NULL){
        (header->prev)->next = header->next;
    }
    else{
        roots[source] = header->next;
    }
    if (header->next != NULL){
        (header->next)->prev = header->prev;
    }
    else{
        last[source] = header->prev;
    }
    free(header->text);
    free(header);
}

// Usuwa wszystkie zapisane wiadomości.
void delHeaders(){
    for (int i = 0; i < 16; i++){
        Header* temp = roots[i];
        Header* temp2;
        while (temp != NULL){
            temp2 = temp;
            temp = temp2->next;
            free(temp2->text);
            free(temp2);
        }
    }
}
void MIMPI_Init(bool enable_deadlock_detection) {
    channels_init();
    char* initalized = getenv("ENTERED");
    // Sprawdza, czy proces wchodził już do bloku MIMPI.
    if (initalized != NULL && atoi(initalized) == 0) {
        for (int i = 0; i < 16; i++){
            roots[i] = NULL;
            last[i] = NULL;
        }
        char val[4];
        sprintf(val, "%d", 1);
        setenv("ENTERED", val, 1);  // Udziela pozowolenia na wejście.
    }
}

// Zamyka deskryptory.
void closeDesc(int rank, int size){
    int dir = 23 + size * 4 + (size - 1) * 2 * rank;
    for (int i = 0; i < size - 1; i++){  // Zamyka swoje deskryptory do wysyłania.
        ASSERT_ZERO(close(dir + i * 2));

    }
    for (int i = 0; i < size; i++){  // Zamyka swoje deskryptory do odczytu.
        if (i != rank){
            dir = 22 + size * 4 + rank * 2 + i * (size - 1) * 2;
            // Jeśli rank odbiorcy jest większy od rank nadawcy.
            if (rank > i) {
                dir -= 2;
            }
            ASSERT_ZERO(close(dir));
        }
    }
}
void MIMPI_Finalize() {
    char* initalized = getenv("ENTERED");
    // Sprawdza, czy proces jest w bloku MIMPI.
    if (initalized != NULL && atoi(initalized) == 1) {
        int rank = MIMPI_World_rank();
        int size = MIMPI_World_size();
        int *data = malloc(sizeof(int) * (size + 1));  // Tablica procesów w bloku MIMPI.
        ASSERT_SYS_OK(chrecv(20, data, sizeof(int) * (size + 1)));
        data[rank] = 0;  // Zaznacza w tablicy, że zakończył swój blok MIMPI.
        data[size] = data[size] - 1; // Zaznacza w tablicy, że proces mniej jest w bloku;
        ASSERT_SYS_OK(chsend(21, data, sizeof(int) * (size + 1)));

        // Wysyła do pozostałych procesów wiadomość, że zakończył swój blok MIMPI.
        for (int i = 0; i < size; i++){
            if (i != rank) {
                MIMPI_Send(NULL, 0, i, -1);
            }
        }
        closeDesc(rank, size);
        delHeaders();
        for (int i = 0; i < size; i++){  // Budzenie wszystkich procesów z błędem.
            ASSERT_SYS_OK(chsend(23 + 2 * i, &bar, sizeof(int)));
        }
        free(data);

        // Closing the rest of descriptors.
        for (int i = 20; i <= 21 + size * 4; i++){
            ASSERT_ZERO(close(i));
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
        else{  // Wysyła zwykłą wiadomość.

            int size = MIMPI_World_size();
            int *in = malloc(sizeof(int) * (size + 1));  // Tablica procesów w bloku MIMPI.
            ASSERT_SYS_OK(chrecv(20, in, sizeof(int) * (size + 1)));
            ASSERT_SYS_OK(chsend(21, in, sizeof(int) * (size + 1)));
            if (in[destination] == 0) {  // Jeśli procesu już nie ma w bloku MIMPI.
                return 3;
            }
            free(in);

            void* msg = malloc(520);  // Wiadomość wysyłana.
            memcpy(msg, &tag, sizeof(int));  // Dodawanie tagu wiadomości.
            memcpy(msg + sizeof(int), &count, sizeof(int)); // Dodawanie wielkości wiadomości.
            if (count < 512){
                memcpy(msg + 2 * sizeof(int), data, count); // Dodawanie treści wiadomości.
                ASSERT_SYS_OK(chsend(dir, msg, sizeof(int) * 2 + count));
            }
            else{
                memcpy(msg + 2 * sizeof(int), data, 512);
                ASSERT_SYS_OK(chsend(dir, msg, 520));
            }
            int index = 1;
            while (count - 512 * (index + 1) > 0){
                memcpy(msg, data + 512 * index, 512); // Dodawanie dalszej treści wiadomości.
                ASSERT_SYS_OK(chsend(dir, msg, 512));
                index++;
            }
            int rest = count - 512 * index;
            if (rest > 0){
                memcpy(msg, data + count - rest, rest); // Dodawanie dalszej treści wiadomości.
                ASSERT_SYS_OK(chsend(dir, msg, rest));
            }
            free(msg);
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
        int size = MIMPI_World_size();
        int rank = MIMPI_World_rank();
        int dir = 22 + size * 4 + rank * 2 + source * (size - 1) * 2;
        // Jeśli rank odbiorcy jest większy od rank nadawcy.
        if (source < MIMPI_World_rank()) {
            dir -= 2;
        }
        Header* found = findtext(source, tag, count);  // Znajduje pierwszą wiadomość o danym tagu i rozmiarze.
        if (found != NULL){  // Jeśli znalazł wiadomość spełniającą wymagania.
            memcpy(data, found->text, count);
            clearHeader(found, source);  // Usuwa header.
        }
        else {  // Nie ma wiadomości spełniającej wymagania.
            bool exit = false;
            int loop = -1;
            while (!exit) {  // Odbiera wiadomości, póki nie otrzyma spełniającej wymagania.
                loop++;
                int newtag;  // Tag odczytywanej wiadomości.
                int msgsize;  // Wielkość odczytywanej wiadomości.
                void *buff = malloc(sizeof(int));
                ASSERT_SYS_OK(chrecv(dir, buff, sizeof(int)));  // Czytanie tagu nowej wiadomości.
                memcpy(&newtag, buff, sizeof(int));

                if (newtag == -1) {  // Jeśli tag wiadomości oznacza wyjście z bloku MIMPI.
                    return 3;
                }
                ASSERT_SYS_OK(chrecv(dir, buff, sizeof(int)));  // Czytanie rozmiaru nowej wiadomości.
                memcpy(&msgsize, buff, sizeof(int));
                void *save;
                int num;
                if (tag != newtag || count != msgsize) {  // Jeśli rozmiar lub tag nie pasują do wymaganych.
                    Header* newHeader = createHeader(newtag, msgsize, source);  // Tworzy nowy header.
                    save = newHeader->text;
                    num = msgsize;
                } else {
                    save = data;
                    num = count;
                    exit = true;
                }

                int index = 0;
                while (num - 512 * (index + 1) > 0) {
                    ASSERT_SYS_OK(chrecv(dir, save + 512 * index, 512));
                    index++;
                }
                int rest = num - 512 * index;
                if (rest > 0) {
                    ASSERT_SYS_OK(chrecv(dir, save + num - rest, rest));
                }
            }
        }
        return 0;
    }
    else{
        return -5;
    }
}

MIMPI_Retcode MIMPI_Barrier() {
    char *initalized = getenv("ENTERED");
    if (initalized != NULL && atoi(initalized) == 1) {  // Sprawdza, czy proces jest w bloku MIMPI.
        bar++;
        int size = MIMPI_World_size();
        int rank = MIMPI_World_rank();
        int recv = 22 + 2 * rank;
        static int msg = -1;
        int code = bar;  // Odczytywana wiadomość.
        if ((rank + 1) * 2 <= size){  // Odbiór wiadomości od lewego dziecka.
            void *buff = malloc(sizeof(int));
            while (code != -1) {  // Póki wiadomość nie jest od dziecka.
                if (code < bar) {  // Wiadomość nie od dziecka, a od procesu, który nie wszedł do aktualnej bariery.
                    return 3;
                }
                ASSERT_SYS_OK(chrecv(recv, buff, sizeof(int)));
                memcpy(&code, buff, sizeof(int));
            }
            code = bar;
        }
        if ((rank + 1) * 2 + 1 <= size){  // Odbiór wiadomości od prawego dziecka.
            void *buff = malloc(sizeof(int));
            while (code != -1) {  // Póki wiadomość nie jest od dziecka.
                if (code < bar) {  // Wiadomość nie od dziecka, a od procesu, który nie wszedł do aktualnej bariery.
                    return 3;
                }
                ASSERT_SYS_OK(chrecv(recv, buff, sizeof(int)));
                memcpy(&code, buff, sizeof(int));
            }
            code = bar;
        }
        if (rank > 0){  // Wysyłka i odbiór wiadomości od ojca.
            chsend(((rank - 1) / 2) * 2 + 23, &msg, sizeof(int));
            void *buff = malloc(sizeof(int));
            while (code != -1) {  // Póki wiadomość nie jest od dziecka.
                if (code < bar) {  // Wiadomość nie od dziecka, a od procesu, który nie wszedł do aktualnej bariery.
                    return 3;
                }
                ASSERT_SYS_OK(chrecv(recv, buff, sizeof(int)));
                memcpy(&code, buff, sizeof(int));
            }
            code = bar;
        }
        if ((rank + 1) * 2 <= size){  // Wysyłka wiadomości do lewego dziecka.
            chsend((rank * 2 + 1) * 2 + 23, &msg, sizeof(int));
        }
        if ((rank + 1) * 2 + 1 <= size){  // Wysyłka wiadomości do prawego dziecka.
            chsend(((rank + 1) * 2) * 2 + 23, &msg, sizeof(int));
        }
        return 0;
    }
    return -5;  // Proces nie jest w bloku MIMPI.
}

MIMPI_Retcode MIMPI_Bcast(
    void *data,
    int count,
    int root
) {
    int code = MIMPI_Barrier();
    if (code == 0){
        int rank = MIMPI_World_rank();
        int size = MIMPI_World_size();
        if (root == rank){
            for (int i = 0; i < size; i++){
                if (i != rank){
                    ASSERT_SYS_OK(chsend(25 + 2 * i, data, count));
                }
            }
        }
        else{
            ASSERT_SYS_OK(chrecv(24 + rank * 2, data, count));
        }
        return 0;
    }
    else{
        printf("TEN BALDA %d\n", MIMPI_World_rank());
        return 3;
    }
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