#include "list.h"

void add_front(struct DList* list, struct Peer* peer) {
  pointer_not_null(list, "Error in add_front the list is unitialized!\n");
  pointer_not_null(peer, "Error in add_front the peer is unitialized!\n");

  struct DNode* new = (struct DNode*)malloc(sizeof(struct DNode));
  pointer_not_null(new, "Error in add_front the new node is unitialized!\n");

  new->peer = peer;
  new->next = list->head;
  new->prev = NULL;

  if (!list->head) {
    list->head = list->tail = new;
    list->size = 1;
  } else {
    list->head->prev = new;
    list->head = new;
    list->size += 1;
  }
}