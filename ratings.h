#pragma once

struct rating {
    int rating_num;
    int rating_sum;
};

void recv_rating(struct rating *arr, int *clock, int *r);
int send_rating(int company_no, int rate, int *clock, int size);
void print_rating(int rank, struct rating *arr, int len);
void init_ranking(struct rating *arr, int len);
