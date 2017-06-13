#pragma once

struct data {
    unsigned long clk;
    int rank;
    int company;
};

struct node {
    struct node *next;
    struct data data;
};


void push_element(struct node **head, struct data d);
void erase_element(struct node **head, struct node *entry);
struct data pop_element(struct node **head);
