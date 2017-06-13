#pragma once

struct rating {
    int rating_num;
    int rating_sum;
};

int recv_rating(struct rating *arr, unsigned long *clock);
int send_rating(int company_no, int rate, unsigned long *clock, int size);
void print_rating(int rank, struct rating *arr, int len);
void init_ranking(struct rating *arr, int len);
