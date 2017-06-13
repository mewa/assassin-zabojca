#include "linked_list.h"
#include <stdlib.h>

void push_element(struct node **head, int data) {
    struct node* new_node = malloc(sizeof(struct node));
    new_node->data = data;
    new_node->next = *head;
    *head = new_node;
}

void erase_element(struct node **head, struct node *entry) {
  struct node **indirect = head;
  while ((*indirect) != entry) {
    indirect = &(*indirect)->next;
  }
  *indirect = entry->next;
  free(entry);
}

int pop_element(struct node **head) {
  if (head) {
    int ret = (*head)->data;
    erase_element(head, *head);
    return ret;
  }
  return -1;
}
    
