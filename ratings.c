#include "ratings.h"
#include "lamport.h"
#include "tags.h"
#include <stdio.h>

#define COMP_NUM 0
#define RATE 1

void init_ranking(struct rating *arr, int len) {
  int i;
  for (i = 0; i < len; ++i) {
    arr[i].rating_num = 1;
    arr[i].rating_sum = ((i + 5) % 10) + 1;
  }
}

int recv_rating(struct rating *arr, unsigned long *clock) {
  MPI_Status status;
  int r[2];
  int ret = lamport_recv(&r, 2, MPI_INT, MPI_ANY_SOURCE, RATING_TAG, MPI_COMM_WORLD, &status, clock, MPI_Recv);
  arr[r[COMP_NUM]].rating_num++;
  arr[r[COMP_NUM]].rating_sum += r[RATE];
  return ret;
}

int send_rating(int company_no, int rate, unsigned long *clock, int size) {
  int i;
  int r[2];
  r[COMP_NUM] = company_no;
  r[RATE] = rate;
  for (i = 0; i < size; i++) {
    int ret = lamport_send(&r, 2, MPI_INT, i, RATING_TAG, MPI_COMM_WORLD, clock, MPI_Send);
    if (ret != 0) {
      fprintf(stderr, "rating send error\n");
      return ret;
    }
  }
  return 0;
}

void print_rating(int rank, struct rating *arr, int len) {
  int i;
  for (i = 0; i < len; ++i) {
    printf("%d: company no: %d, rating sum: %d, rating num: %d\n",
        rank, i, arr[i].rating_sum, arr[i].rating_num);
  }
}
