#include "list.h"

void add_front(struct DList* list, struct Peer* peer) {
  pointer_not_null(list, "Error in add_front the list is unitialized!\n");
  pointer_not_null(peer, "Error in add_front the peer is unitialized!\n");

  struct DNode* new_node = (struct DNode*)malloc(sizeof(struct DNode));
  pointer_not_null(new_node,
                   "Error in add_front the new_node node is unitialized!\n");

  new_node->peer = peer;
  new_node->next = list->head;
  new_node->prev = NULL;

  if (!list->head) {
    list->head = list->tail = new_node;
    list->size = 1;
  } else {
    list->head->prev = new_node;
    list->head = new_node;
    list->size += 1;
  }
}

void add_back(struct DList* list, struct Peer* peer) {
  pointer_not_null(list, "Error in add_back the list is unitialized!\n");
  pointer_not_null(peer, "Error in add_back the peer is unitialized!\n");

  struct DNode* new_node = (struct DNode*)malloc(sizeof(struct DNode));
  pointer_not_null(new_node,
                   "Error in add_back the new_node node is unitialized!\n");

  new_node->peer = peer;
  new_node->next = NULL;
  new_node->prev = list->tail;

  if (!list->head) {
    list->head = list->tail = new_node;
    list->size = 1;
  } else {
    list->tail->next = new_node;
    list->tail = new_node;
    list->size += 1;
  }
}

struct Peer* remove_front(struct DList* list) {
  if (!list || !list->head) return (NULL);

  struct DNode* head = list->head;
  if (list->head->next) list->head->next->prev = NULL;
  list->head = head->next;
  struct Peer* head_peer = head->peer;
  free(head);
  return head_peer;
}