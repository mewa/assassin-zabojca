#include <unistd.h>
#include <stdio.h>
#include "lamport.h"
#include <time.h>
#include <stdlib.h>
#include <mpi.h>
#include <pthread.h>
#include "ratings.h"

#define COMPANIES_NUM 4

void wait_sec(int min, int max) {
  unsigned int time = (rand() % (max - min)) + min;
  sleep(time);
}

int main(int argc, char** argv) {

  int i;
  int rank, size;
  unsigned long clk = 0;
  struct rating rating_arr[COMPANIES_NUM];
  init_ranking(rating_arr, COMPANIES_NUM);

  int thread_support_provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &thread_support_provided);
  if (thread_support_provided != MPI_THREAD_MULTIPLE) {
    fprintf(stderr, "doesn't support multithreading\n");
    MPI_Finalize();
    exit(-1);
  }

  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  for (i = 0; i < COMPANIES_NUM; ++i) {
    send_rating(i, rand() % 10, &clk, size);
  }
  for (i = 0; i < size * COMPANIES_NUM; ++i) {
    recv_rating(rating_arr, &clk);
  }

  print_rating(rank, rating_arr, COMPANIES_NUM);

  MPI_Finalize();
  return 0;
}
