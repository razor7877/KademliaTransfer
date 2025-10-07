#include "list.h"

#include <string.h>

#include "peer.h"

void add_front(struct DList* list, struct Peer* peer) {
  if (!list || !peer) return NULL;

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
  if (!list || !peer) return NULL;

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
  list->size -= 1;
  return head_peer;
}

struct Peer* remove_back(struct DList* list) {
  if (!list || !list->tail) return (NULL);

  struct DNode* tail = list->tail;
  if (list->tail->prev) list->tail->prev->next = NULL;
  list->tail = tail->prev;
  struct Peer* tail_peer = tail->peer;
  free(tail);
  list->size -= 1;
  return tail_peer;
}

/**
 * @brief Calculate the XOR distance between two HashID
 * @param result Buffer where the result is stored
 * @param id1 First HashID
 * @param id2 Second HashID
 */
static void dist_hash(HashID* result, const HashID* id1, const HashID* id2) {
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    *result[i] = *id1[i] ^ *id2[i];
  }
}

/**
 * @brief Compare two distances (produced by dist_hash)
 *  It returns:
 *      - -1 if dist1 < dist2
 *      - 1 if dist1 > dist2
 *      - 0 if dist1 = dist2
 * @param dist1 First distance to compare
 * @param dist2 Second distance to compare
 * @return int Comparaison result: -1,0,1
 */
static int compare_distance(const HashID* dist1, const HashID* dist2) {
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    if (*dist1[i] < *dist2[i]) return -1;
    if (*dist1[i] > *dist2[i]) return 1;
  }
  return 0;
}

struct Peer* find_nearest(const struct DList* list, const HashID* id) {
  if (!list || !list->head) return NULL;

  struct Peer* nearest = list->head->peer;
  HashID nearest_dist = {0};
  HashID current_dist = {0};
  dist_hash(&nearest_dist, &nearest->peer_id, id);
  struct DNode* current = list->head->next;

  while (current != NULL) {
    dist_hash(&current_dist, &current->peer->peer_id, id);
    if (compare_distance(&nearest_dist, &current_dist) > 0) {
      nearest = current->peer;
      memcpy(nearest_dist, current_dist, SHA256_DIGEST_LENGTH);
    }
    current = current->next;
  }
  return nearest;
}