#pragma once

#include <mpi.h>
#include <pthread.h>

typedef int (*send_fun)(void const*, int, MPI_Datatype, int, int, MPI_Comm);

typedef int (*recv_fun)(void*, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status*);

int lamport_send(void const* data, unsigned long len, MPI_Datatype dtype, int dest,
		 int tag, MPI_Comm comm,
		 unsigned long* clock, send_fun f);

int lamport_recv_clk(void* data, unsigned long len, MPI_Datatype dtype, int source,
		 int tag, MPI_Comm comm, MPI_Status* status,
		 unsigned long* clock, unsigned long* msg_clock, recv_fun f);

int lamport_recv(void* data, unsigned long len, MPI_Datatype dtype, int source,
		 int tag, MPI_Comm comm, MPI_Status* status,
		 unsigned long* clock, recv_fun f);

int lamport_send_to_all(void const* data, unsigned long len, MPI_Datatype dtype,
    int tag, unsigned long* clock, int size, int my_rank);

unsigned long ulmax(unsigned long a, unsigned long b);
