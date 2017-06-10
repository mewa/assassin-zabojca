#include <unistd.h>
#include <stdio.h>
#include "lamport.h"
#include <mpi.h>

#define TAG 12

int main(int argc, char** argv) {
  int d = 1, i;
  int rank, size;
  unsigned long clk = 0;
  MPI_Init(&argc, &argv);

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
