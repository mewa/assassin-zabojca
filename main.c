#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <mpi.h>
#include <pthread.h>
#include "ratings.h"
#include "lamport.h"
#include "tags.h"

#define COMPANIES_NUM 4
#define ASSASSINS_NUM 2

int rank, size;
unsigned long clk = 1;
int assassin_req_clk = 0;
int assassin_company_num = 2;

void wait_sec(int min, int max) {
  unsigned int time = (rand() % (max - min)) + min;
  sleep(time);
}

void* get_assasin(void *arg) {
  //TODO mutex
  assassin_req_clk = clk;
  printf("%d: send req for assassin with clock %d\n", rank, assassin_req_clk);
  int send_to_all = lamport_send_to_all(&assassin_company_num, 1, MPI_INT, ASSASIN_TAG_REQ, &clk, size, rank);
  if (send_to_all) {
    fprintf(stderr, "send to all\n");
  }
  int ack = 0;
  while (ack < size - ASSASSINS_NUM) {
    int company_num_ack;
    MPI_Status status;
    int recv_err = lamport_recv(&company_num_ack, 1, MPI_INT, MPI_ANY_SOURCE, ASSASIN_TAG_ACK,
        MPI_COMM_WORLD, &status, &clk, MPI_Recv);
    if (recv_err) {
      fprintf(stderr, "recv err\n");
    }
    printf("%d: recv ack from %d with company no: %d\n", rank, status.MPI_SOURCE, company_num_ack);
    if (company_num_ack == assassin_company_num) {
      printf("%d: ack++\n", rank);
      ack++;
    }
  }
  printf("%d: get assassin\n", rank);
  wait_sec(4, 10);
  printf("%d: free assassin\n", rank);
  //TODO mutex
  assassin_req_clk = 0;
  return NULL;
}

void* accept_assassin_req(void *arg) {
  int company_num_req = 0;
  unsigned long req_clk;
  MPI_Status status;

  while (1) {
    lamport_recv_clk(&company_num_req, 1, MPI_INT, MPI_ANY_SOURCE, ASSASIN_TAG_REQ,
        MPI_COMM_WORLD, &status, &clk, &req_clk, MPI_Recv);
    printf("%d: recv req for assassin from %d with req_clk %lu\n", rank, status.MPI_SOURCE, req_clk);
    if (req_clk < assassin_req_clk || !assassin_req_clk ||
        (req_clk == assassin_req_clk && status.MPI_SOURCE < rank)) {
      printf("%d: send ack to %d\n", rank, status.MPI_SOURCE);
      lamport_send(&company_num_req, 1, MPI_INT, status.MPI_SOURCE, ASSASIN_TAG_ACK,
          MPI_COMM_WORLD, &clk, MPI_Send);
    } else {
      //TODO add to queue
    }
    //TODO empty queue
  }
}

int main(int argc, char** argv) {

  int i;
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

  srand(time(0) + rank);
  pthread_t assassin_req_thread, assassin_ack_thread;


  for (i = 0; i < COMPANIES_NUM; ++i) {
    send_rating(i, rand() % 10, &clk, size);
  }
  for (i = 0; i < size * COMPANIES_NUM; ++i) {
    recv_rating(rating_arr, &clk);
  }

  print_rating(rank, rating_arr, COMPANIES_NUM);

  pthread_create(&assassin_req_thread, NULL, get_assasin, NULL);
  pthread_create(&assassin_ack_thread, NULL, accept_assassin_req, NULL);

  pthread_join(assassin_req_thread, NULL);
  pthread_join(assassin_ack_thread, NULL);

  MPI_Finalize();
  return 0;
}
