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
#include <stdarg.h>

#define COMPANIES_NUM 1
#define ASSASSINS_NUM 1
#define NEAR_COMPANY 2

pthread_mutex_t company_mut = PTHREAD_MUTEX_INITIALIZER;

int rank, size;
int clk = 1;
int selected_company = -4;
int clock_at_req[COMPANIES_NUM] = {};
int ack_num[COMPANIES_NUM] = {};
struct node *req_for_company[COMPANIES_NUM] = {};
struct rating rating_arr[COMPANIES_NUM];

void print(char* fmt, ...) {
  char buf[1024] = {};
  int index = 0;
  va_list args;
  index += sprintf(buf + index, "[%d | R%d] ", clk, rank);
  int i;
  index += sprintf(buf + index, " [ ");
  for (i = 0; i < COMPANIES_NUM; i++) {
      index += sprintf(buf + index, "%d:%d ", i, ack_num[i]);
  }
  index += sprintf(buf + index, "] ");
  va_start(args, fmt);
  index += vsprintf(buf + index, fmt, args);
  va_end(args);
  printf("%s", buf);
}

void send_ack(int req_clk, int company, int id) {
    int tab[2] = {req_clk, company};
    lamport_send(tab, 2, MPI_INT, id, COMPANY_TAG_ACK, &clk);
}

struct data recv_ack() {
    MPI_Status status;
    int tab[2]={};
    lamport_recv(tab, 2, MPI_INT, MPI_ANY_SOURCE, COMPANY_TAG_ACK, &status, &clk);
    struct data d = {.clk = tab[0], .rank = status.MPI_SOURCE, .company = tab[1]};
    if(d.clk == clock_at_req[d.company]) {
        ack_num[d.company]++;
        print("receive ack for company %d\n", d.company);
    }
    return d;
}

void wait_sec(int min, int max) {
    unsigned int time = (rand() % (max - min)) + min;
    sleep(time);
}

int send_company_req(int company) {
    return lamport_send_to_all(&company, 1, MPI_INT, COMPANY_TAG_REQ, &clk, size, rank);
}

void recv_company_ack() {
    struct data d = recv_ack();
    if (ack_num[d.company] >= size - NEAR_COMPANY - ASSASSINS_NUM) {
        selected_company = d.company;
        print("almost get company %d, freeing other companies\n", d.company);
    }
}

void send_all_remain_ack(int company) {
    pthread_mutex_lock(&company_mut);
    while (req_for_company[company]) {
        struct data d = pop_element(&req_for_company[company]);
        send_ack(d.clk, d.company, d.rank);
        print("send ack after free company %d to %d\n", company, d.rank);
    }
    pthread_mutex_unlock(&company_mut);
}

void free_other_companies() {
    int i;
    for (i = 0; i < COMPANIES_NUM; i++) {
        if (i != selected_company) {
            send_all_remain_ack(i);
            clock_at_req[i] = 0;
        }
    }
}

void get_last_ack() {
    while (1) {
        recv_ack();
        if (ack_num[selected_company] == size - ASSASSINS_NUM) {
            return;
        }
    }
}

void clear() {
    int i;
    for (i = 0; i < COMPANIES_NUM; i++) {
        clock_at_req[i] = 0;
        ack_num[i] = 0;
        send_all_remain_ack(i);
    }
    selected_company = -1;
}

void* get_company(void *arg) {
    while (1) {
        int i;
        for (i = 0; i < COMPANIES_NUM; i++) {
            clock_at_req[i] = send_company_req(i);
            print("want %d company with clk %d\n", i, clock_at_req[i]);
        }
        while (selected_company < 0) {
            recv_company_ack();
        }
        free_other_companies();
        get_last_ack();
        print("[assassin start] from company no %d\n", selected_company);
        wait_sec(1, 2);
        print("[assassin stop] from company no %d\n", selected_company);
        send_rating(selected_company, rand() % 10, &clk, size);
        clear();
    }
    return NULL;
}

void* accept_companies_req(void *arg) {
    int company;
    int req_clk;
    MPI_Status status;

    while (1) {
        lamport_recv_clk(&company, 1, MPI_INT, MPI_ANY_SOURCE, COMPANY_TAG_REQ, &status, &clk, &req_clk);
        struct data d = {.clk = req_clk, .rank = status.MPI_SOURCE, .company = company};
        print("recv req for company %d from %d with req_clk %d\n", d.company, d.rank, d.clk);
        if (d.clk < clock_at_req[d.company] || !clock_at_req[d.company] ||
                (d.clk == clock_at_req[d.company] && d.rank < rank) ||
                (selected_company >= 0 && d.company != selected_company)) {
            send_ack(d.clk, d.company, d.rank);
            print("send ack to %d for company %d\n", d.rank, d.company);
        } else {
            pthread_mutex_lock(&company_mut);
            push_element(&req_for_company[d.company], d);
            pthread_mutex_unlock(&company_mut);
        }
    }
    return NULL;
}


void* recv_rat(void *arg) {
    int i = 0;
    for(;;i++) {
        recv_rating(rating_arr, &clk);
        if (i%3 == 0 && rank == 1) {
            print_rating(rank, rating_arr, COMPANIES_NUM);
        }
    }
}

int main(int argc, char** argv) {

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
    pthread_t company_req_thread, company_ack_thread, rating;

    pthread_create(&company_ack_thread, NULL, accept_companies_req, NULL);
    pthread_create(&company_req_thread, NULL, get_company, NULL);
    pthread_create(&rating, NULL, recv_rat, NULL);

    pthread_join(company_ack_thread, NULL);
    pthread_join(company_req_thread, NULL);
    pthread_join(rating, NULL);

    MPI_Finalize();
    return 0;
}
