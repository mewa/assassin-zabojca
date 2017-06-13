#pragma once

#include <mpi.h>
#include <pthread.h>

typedef int (*send_fun)(void const*, int, MPI_Datatype, int, int, MPI_Comm);

typedef int (*recv_fun)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status*);

int lamport_send(void const* data, unsigned long len, MPI_Datatype dtype, int dest,
        int tag, int* clock);

int lamport_recv_clk(void* data, unsigned long len, MPI_Datatype dtype, int source,
        int tag, MPI_Status* status, int* clock, int* msg_clock);

int lamport_recv(void* data, unsigned long len, MPI_Datatype dtype, int source,
        int tag, MPI_Status* status, int* clock);

int lamport_send_to_all(void const* data, unsigned long len, MPI_Datatype dtype,
        int tag, int* clock, int size, int my_rank);

int ulmax(int a, int b);
