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
    char val[4];
    char* initalized = getenv("ENTERED");
    if (initalized != NULL && atoi(initalized) == 0) {
        sprintf(val, "%d", 1);
        setenv("ENTERED", val, 1);
    }
    else{
        sprintf(val, "%d", -1);
        setenv("ENTERED", val, 1);
    }
}

void MIMPI_Finalize() {
    char* initalized = getenv("ENTERED");
    if (initalized != NULL && atoi(initalized) == 1) {
        int rank = MIMPI_World_rank();
        for (int i = 0; i < MIMPI_World_size(); i++){
            if (i != rank) {
                MIMPI_Send(NULL, 0, i, -1);
            }
        }
        int *data = malloc(sizeof(int) * MIMPI_World_size());
        int from = 2 * MIMPI_World_size() * (MIMPI_World_size() - 1) + 20;
        ASSERT_SYS_OK(chrecv(from, data, sizeof(int) * MIMPI_World_size()));
        data[rank] = 0;
        ASSERT_SYS_OK(chsend(from + 1, data, sizeof(int) * MIMPI_World_size()));
        char val[4];
        sprintf(val, "%d", -1);
        setenv("ENTERED", val, -1);
        channels_finalize();
    }
}

int MIMPI_World_size() {
    char* initalized = getenv("ENTERED");
    if (initalized != NULL && atoi(initalized) == 1) {
        char *size = getenv("WORLD_SIZE");
        if (size != NULL) {
            return atoi(size);
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
    if (initalized != NULL && atoi(initalized) == 1) {
        char* rank = getenv("MY_RANK");
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
    if (destination == MIMPI_World_rank()) {
        return 1;
    } else if (destination >= MIMPI_World_size()) {
        return 2;
    } else if (initalized != NULL && atoi(initalized) == 1) {
        int dir = MIMPI_World_rank() * (MIMPI_World_size() - 1) * 2 + 21 + destination * 2;
        if (destination > MIMPI_World_rank()) {
            dir -= 2;
        }
        if (tag == -1){
            int x = -1;
            ASSERT_SYS_OK(chsend(dir, &x, sizeof(int)));
            return 0;
        }
        else{
            int *in = malloc(sizeof(int) * MIMPI_World_size());
            int from = 2 * MIMPI_World_size() * (MIMPI_World_size() - 1) + 20;
            ASSERT_SYS_OK(chrecv(from, in, sizeof(int) * MIMPI_World_size()));
            int val = in[destination];
            ASSERT_SYS_OK(chsend(from + 1, in, sizeof(int) * MIMPI_World_size()));
            if (val == 0) {
                return 3;
            }
            void* msg = malloc(504);
            memcpy(msg, &tag, sizeof(int));
            memcpy(msg + sizeof(int), data, sizeof(*data));
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
    if (source == MIMPI_World_rank()){
        return 1;
    }
    else if (source >= MIMPI_World_size()){
        return 2;
    }
    else if (initalized != NULL && atoi(initalized) == 1) {
        int dir = source * (MIMPI_World_size() - 1) * 2 + 20 + MIMPI_World_rank() * 2;
        if (source < MIMPI_World_rank()) {
            dir -= 2;
        }
        int newtag;
        void* buff = malloc(sizeof(int));
        ASSERT_SYS_OK(chrecv(dir, buff, sizeof(int)));

        memcpy(&newtag, buff, sizeof(int));
        if (newtag == -1){
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