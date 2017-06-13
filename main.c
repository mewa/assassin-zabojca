#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <mpi.h>
#include <pthread.h>
#include "ratings.h"
#include "lamport.h"
#include "tags.h"
#include "linked_list.h"

#define COMPANIES_NUM 4
#define ASSASSINS_NUM 2

pthread_mutex_t assassin_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t company_mut = PTHREAD_MUTEX_INITIALIZER;

int rank, size;
unsigned long clk = 1;
int selected_company = -1;
int assassin_req_clk = 0;
int assassin_company_num = 2;
int company_req_clk[COMPANIES_NUM] = {};
struct node *assassin_req_list = NULL;
struct node *company_req_list = NULL;

void wait_sec(int min, int max) {
  unsigned int time = (rand() % (max - min)) + min;
  sleep(time);
}

/*
void* get_company(void *arg) {
  int i;
  for (i = 0; i < COMPANIES_NUM; i++) {
    if (rand() % 2) {
      pthread_mutex_lock(&company_mut);
      company_req_clk[i] = clk;
      send_company_req(i);
      pthread_mutex_unlock(&company_mut);
    }
  }
  while (!selected_company) {
    recv_company_ack();
  }
  abort_other_companies();
  get_assasin_from_company();
  return NULL;
}
*/

void* accept_companies_req(void *arg) {
  int company;
  unsigned long req_clk;
  MPI_Status status;

  while (1) {
    lamport_recv_clk(&company, 1, MPI_INT, MPI_ANY_SOURCE, COMPANY_TAG_REQ, &status, &clk, &req_clk);
    printf("%d: recv req for company %d from %d with req_clk %lu\n", rank, company, status.MPI_SOURCE, req_clk);
    if (req_clk < company_req_clk[company] || !company_req_clk[company] ||
        (req_clk == company_req_clk[company] && status.MPI_SOURCE < rank)) {
      pthread_mutex_lock(&company_mut);
      printf("%d: send company %d ack to %d\n", rank, company, status.MPI_SOURCE);
      lamport_send(&req_clk, 1, MPI_INT, status.MPI_SOURCE, COMPANY_TAG_ACK, &clk);
      pthread_mutex_unlock(&company_mut);
    } else {
      struct data d = {.clk = req_clk, .rank = status.MPI_SOURCE, .data = company};
      pthread_mutex_lock(&company_mut);
      push_element(&assassin_req_list, d);
      pthread_mutex_unlock(&company_mut);
    }
  }
  return NULL;
}


void* get_assasin(void *arg) {
  pthread_mutex_lock(&assassin_mut);
  assassin_req_clk = clk;
  printf("%d: send req for assassin with clock %d\n", rank, assassin_req_clk);
  lamport_send_to_all(&assassin_company_num, 1, MPI_INT, ASSASIN_TAG_REQ, &clk, size, rank);
  pthread_mutex_unlock(&assassin_mut);
  int ack = 0;
  while (ack < size - ASSASSINS_NUM) {
    int msg;
    MPI_Status status;
    lamport_recv(&msg, 1, MPI_INT, MPI_ANY_SOURCE, ASSASIN_TAG_ACK, &status, &clk);
    if (msg == assassin_req_clk) {
      printf("%d: recv ack from %d, ack++\n", rank, status.MPI_SOURCE);
      ack++;
    }
  }
  printf("%d: get assassin\n", rank);
  wait_sec(4, 10);
  printf("%d: free assassin\n", rank);
  pthread_mutex_lock(&assassin_mut);
  while (assassin_req_list) {
    struct data d = pop_element(&assassin_req_list);
    lamport_send(&d.clk, 1, MPI_INT, d.rank, ASSASIN_TAG_ACK, &clk);
    printf("%d: send ack after free assassin to %d\n", rank, d.rank);
  }
  assassin_req_clk = 0;
  pthread_mutex_unlock(&assassin_mut);
  return NULL;
}

void* accept_assassin_req(void *arg) {
  int msg;
  unsigned long req_clk;
  MPI_Status status;

  while (1) {
    lamport_recv_clk(&msg, 1, MPI_INT, MPI_ANY_SOURCE, ASSASIN_TAG_REQ, &status, &clk, &req_clk);
    printf("%d: recv req for assassin from %d with req_clk %lu\n", rank, status.MPI_SOURCE, req_clk);
    if (req_clk < assassin_req_clk || !assassin_req_clk ||
        (req_clk == assassin_req_clk && status.MPI_SOURCE < rank)) {
      pthread_mutex_lock(&assassin_mut);
      printf("%d: send ack to %d\n", rank, status.MPI_SOURCE);
      lamport_send(&req_clk, 1, MPI_INT, status.MPI_SOURCE, ASSASIN_TAG_ACK, &clk);
      pthread_mutex_unlock(&assassin_mut);
    } else {
      struct data d = {.clk = req_clk, .rank = status.MPI_SOURCE};
      pthread_mutex_lock(&assassin_mut);
      push_element(&assassin_req_list, d);
      pthread_mutex_unlock(&assassin_mut);
    }
  }
}

int main(int argc, char** argv) {

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

  pthread_create(&assassin_req_thread, NULL, get_assasin, NULL);
  pthread_create(&assassin_ack_thread, NULL, accept_assassin_req, NULL);

  pthread_join(assassin_req_thread, NULL);
  pthread_join(assassin_ack_thread, NULL);

  MPI_Finalize();
  return 0;
}
