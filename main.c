#include <unistd.h>
#include <stdio.h>
#include "lamport.h"
#include <time.h>
#include <stdlib.h>
#include <mpi.h>
#include <pthread.h>

#define TAG 12

void wait_sec(int min, int max) {
  unsigned int time = (rand() % (max - min)) + min;
  sleep(time);
}

int main(int argc, char** argv) {
  int d = 1, i;
  int rank, size;
  unsigned long clk = 0;

  int thread_support_provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &thread_support_provided);
  if (thread_support_provided != MPI_THREAD_MULTIPLE) {
    fprintf(stderr, "doesn't support multithreading\n");
    MPI_Finalize();
    exit(-1);
  }

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  MPI_Status status;
  for (i = 0; i < 10; ++i) {
    const unsigned int to = (rank + 1) % size;
    lamport_send(&d, 1, MPI_INT, to, TAG, MPI_COMM_WORLD, &clk, MPI_Send);
    printf("[%d @ %ld]: sent to [%d]\n", rank, clk, (rank + 1) % size);
    const unsigned int from = (rank - 1 + size) % size;
    lamport_recv(&d, 1, MPI_INT, from, TAG, MPI_COMM_WORLD, &status, &clk, MPI_Recv);
    printf("[%d @ %ld]: received\n", rank, clk);
  }
  MPI_Finalize();
  return 0;
}
