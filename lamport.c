#include "lamport.h"
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

const int MSG_MAX_SIZE = 1024;
MPI_Comm comm = MPI_COMM_WORLD;

pthread_mutex_t clk_mutex = PTHREAD_MUTEX_INITIALIZER;

int lamport_send_to_all(void const* data, unsigned long len, MPI_Datatype dtype, int tag, int* clock, int size, int my_rank) {

    int pos = 0;
    void *send_buf = malloc(MSG_MAX_SIZE);
    MPI_Comm comm = MPI_COMM_WORLD;

    pthread_mutex_lock(&clk_mutex);
    MPI_Pack(clock, 1, MPI_INT, send_buf, MSG_MAX_SIZE, &pos, comm);
    int clock_when_sending = *clock;
    (*clock)++;
    pthread_mutex_unlock(&clk_mutex);
    MPI_Pack(data, len, dtype, send_buf, MSG_MAX_SIZE, &pos, comm);

    int i;
    for (i = 0; i < size; i++) {
        if (i != my_rank) {
            MPI_Send(send_buf, pos, MPI_PACKED, i, tag, comm);
        }
    }

    return clock_when_sending;
}



int lamport_send(void const* data, unsigned long len, MPI_Datatype dtype, int dest,
        int tag, int* clock) {

    int pos = 0;
    void *send_buf = malloc(MSG_MAX_SIZE);

    pthread_mutex_lock(&clk_mutex);
    MPI_Pack(clock, 1, MPI_INT, send_buf, MSG_MAX_SIZE, &pos, comm);
    int clock_when_sending = *clock;
    (*clock)++;
    pthread_mutex_unlock(&clk_mutex);
    MPI_Pack(data, len, dtype, send_buf, MSG_MAX_SIZE, &pos, comm);

    MPI_Send(send_buf, pos, MPI_PACKED, dest, tag, comm);

    return clock_when_sending;
}

int lamport_recv(void* data, unsigned long len, MPI_Datatype dtype, int source,
        int tag, MPI_Status* status, int* clock) {
    int msg_clock;
    return lamport_recv_clk(data, len, dtype, source, tag, status, clock, &msg_clock);
}

int lamport_recv_clk(void* data, unsigned long len, MPI_Datatype dtype, int source,
        int tag, MPI_Status* status, int* clock, int* msg_clock) {

    int pos = 0;
    void *recv_buf = malloc(MSG_MAX_SIZE);

    int ret = MPI_Recv(recv_buf, MSG_MAX_SIZE, MPI_PACKED, source, tag, comm, status);
    if (ret < 0)
        goto recv_err;

    MPI_Unpack(recv_buf, MSG_MAX_SIZE, &pos, msg_clock, 1, MPI_INT, comm);
    MPI_Unpack(recv_buf, MSG_MAX_SIZE, &pos, data, len, dtype, comm);

    pthread_mutex_lock(&clk_mutex);
    *clock = ulmax(*clock, *msg_clock) + 1;
    pthread_mutex_unlock(&clk_mutex);
recv_err:
    return ret;
}

int ulmax(int a, int b) {
    return a > b ? a : b;
}
