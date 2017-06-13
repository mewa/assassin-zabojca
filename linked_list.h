#pragma once

struct node {
  struct node *next;
  int data;
};

void push_element(struct node **head, int data);
void erase_element(struct node **head, struct node *entry);
int pop_element(struct node **head);
