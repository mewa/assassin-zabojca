#pragma once

#include <mpi.h>

struct message {
  size_t timestamp;
  void* data;
};

typedef int (*send_fun)(void*, int, MPI_Datatype, int, int, MPI_Comm);

typedef int (*recv_fun)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status*);

int lamport_send(void* data, size_t len, MPI_Datatype dtype, int dest, int tag, MPI_Comm comm, long* clock, send_fun f);

int lamport_recv(void* data, size_t len, MPI_Datatype dtype, int source, int tag, MPI_Comm comm, MPI_Status* status, long* clock, recv_fun f);
