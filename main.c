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
int completed = 0;
int selected_company = -1;
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

void send_ack(int msg_clk, int company, int id) {
    int tab[2] = {msg_clk, company};
    lamport_send(tab, 2, MPI_INT, id, COMPANY_TAG_ACK, &clk);
}

void recv_ack(MPI_Status status, int *tab) {
    struct data d = {.clk = tab[0], .rank = status.MPI_SOURCE, .company = tab[1]};
    if(d.clk == clock_at_req[d.company]) {
        ack_num[d.company]++;
        print("receive ack for company %d\n", d.company);
        if (ack_num[d.company] >= size - NEAR_COMPANY - ASSASSINS_NUM && selected_company < 0) {
            selected_company = d.company;
            print("almost get company %d, freeing other companies\n", d.company);
        }
        if (ack_num[d.company] >= size - ASSASSINS_NUM) {
            completed = 1;
        }
    }
}

void wait_sec(int min, int max) {
    unsigned int time = (rand() % (max - min)) + min;
    sleep(time);
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

void accept_companies_req(MPI_Status status, int msg_clk, int company) {
    struct data d = {.clk = msg_clk, .rank = status.MPI_SOURCE, .company = company};
    print("recv req for company %d from %d with msg_clk %d\n", d.company, d.rank, d.clk);
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

int send_company_req(int company) {
    return lamport_send_to_all(&company, 1, MPI_INT, COMPANY_TAG_REQ, &clk, size, rank);
}

void clear() {
    int i;
    for (i = 0; i < COMPANIES_NUM; i++) {
        clock_at_req[i] = 0;
        ack_num[i] = 0;
        send_all_remain_ack(i);
    }
    selected_company = -1;
    completed = 0;
}

int main(int argc, char** argv) {

    init_ranking(rating_arr, COMPANIES_NUM);
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    srand(time(0) + rank);
    MPI_Status status;
    int data[2];
    int msg_clk;
    init_ranking(rating_arr, COMPANIES_NUM);

    int i = 0;
    for (;; i++) {
        int j;
        for (j = 0; j < COMPANIES_NUM; j++) {
            clock_at_req[j] = send_company_req(j);
            print("want %d company with clk %d\n", j, clock_at_req[j]);
        }

        while (!completed) {
            lamport_recv_clk(data, 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, &status, &clk, &msg_clk);
            switch (status.MPI_TAG) {
                case COMPANY_TAG_REQ:
                    accept_companies_req(status, msg_clk, data[0]);
                    break;
                case COMPANY_TAG_ACK:
                    recv_ack(status, data);
                    break;
                case RATING_TAG:
                    recv_rating(rating_arr, &clk, data);
                    break;
            }
            if (selected_company > 0) {
                free_other_companies();
            }
        }

        print("[assassin start] from company no %d\n", selected_company);
        wait_sec(3, 5);
        print("[assassin stop] from company no %d\n", selected_company);
        send_rating(selected_company, rand() % 10, &clk, size);
        clear();
        if (i % 3 == 0 && rank == 0) {
            print_rating(rank, rating_arr, COMPANIES_NUM);
        }
    }

    MPI_Finalize();
    return 0;
}
