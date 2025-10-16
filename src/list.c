#include "list.h"

#include <string.h>

#include "log.h"
#include "peer.h"

void add_front(struct DList *list, struct Peer *peer)
{
  if (!list || !peer)
    return;

  struct DNode *new_node = (struct DNode *)malloc(sizeof(struct DNode));
  pointer_not_null(new_node,
                   "Error in add_front the new_node node is unitialized!\n");

  new_node->peer = peer;
  new_node->next = list->head;
  new_node->prev = NULL;

  if (!list->head)
  {
    list->head = list->tail = new_node;
    list->size = 1;
  }
  else
  {
    list->head->prev = new_node;
    list->head = new_node;
    list->size += 1;
  }
}

void add_back(struct DList *list, struct Peer *peer)
{
  if (!list || !peer)
    return;

  struct DNode *new_node = (struct DNode *)malloc(sizeof(struct DNode));
  pointer_not_null(new_node,
                   "Error in add_back the new_node node is unitialized!\n");

  new_node->peer = peer;
  new_node->next = NULL;
  new_node->prev = list->tail;

  if (!list->head)
  {
    list->head = list->tail = new_node;
    list->size = 1;
  }
  else
  {
    list->tail->next = new_node;
    list->tail = new_node;
    list->size += 1;
  }
}

struct Peer *remove_front(struct DList *list)
{
  if (!list || !list->head)
    return (NULL);

  struct DNode *head = list->head;
  if (list->head->next)
    list->head->next->prev = NULL;
  list->head = head->next;
  struct Peer *head_peer = head->peer;
  free(head);
  list->size -= 1;
  return head_peer;
}

struct Peer *remove_back(struct DList *list)
{
  if (!list || !list->tail)
    return (NULL);

  struct DNode *tail = list->tail;
  if (list->tail->prev)
    list->tail->prev->next = NULL;
  list->tail = tail->prev;
  struct Peer *tail_peer = tail->peer;
  free(tail);
  list->size -= 1;
  return tail_peer;
}

struct Peer *remove_node(struct DList *list, struct Peer *node)
{
  if (!list || !node)
    return NULL;

  if (list->head->peer == node)
  {
    list->head = list->head->next;
    if (list->head)
      list->head->prev = NULL;
    list->size -= 1;
    return node;
  }

  struct DNode *prev = list->head;
  while (prev->next != NULL && prev->next->peer != node)
    prev = prev->next;

  if (prev->next->peer == node)
  {
    struct DNode *to_delete = prev->next;
    prev->next = to_delete->next;
    if (prev->next)
      prev->next->prev = prev;
    return node;
    list->size -= 1;
  }
  return NULL;
}

void dist_hash(HashID result, const HashID id1, const HashID id2)
{
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
  {
    result[i] = id1[i] ^ id2[i];
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
static int compare_distance(const HashID dist1, const HashID dist2)
{
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
  {
    if (dist1[i] < dist2[i])
      return -1;
    if (dist1[i] > dist2[i])
      return 1;
  }
  return 0;
}

int find_nearest(const struct DList *list, const HashID id,
                 struct Peer **out_peers, size_t max_neighbors)
{
  if (!list || !list->head || !out_peers || max_neighbors == 0)
    return 0;

  HashID neighbors_dist[max_neighbors];
  size_t n = 0;

  struct DNode *current = list->head;

  while (current != NULL)
  {
    HashID current_dist = {0};
    dist_hash(&current_dist, &current->peer->peer_id, id);

    if (n < max_neighbors)
    {
      memcpy(&neighbors_dist[n], &current_dist, sizeof(HashID));
      out_peers[n] = current->peer;
      n += 1;
    }
    else
    {
      for (size_t i = 0; i < n; i++)
      {
        if (compare_distance(&neighbors_dist[i], &current_dist) > 0)
        {
          log_msg(LOG_WARN, "Store out_peers peer with addr %p", current->peer);
          out_peers[i] = current->peer;
          memcpy(&neighbors_dist[i], &current_dist, sizeof(HashID));
        }
      }
    }
    current = current->next;
  }

  return n;
}

struct Peer *find_peer_by_id(const struct DList *list, const HashID id)
{
  if (!list || !list->head || !id)
    return NULL;

  struct DNode *current = list->head;
  while (current != NULL)
  {
    if (memcmp(&current->peer->peer_id, id, sizeof(HashID)) == 0)
      return current->peer;

    current = current->next;
  }

  return NULL;
}
