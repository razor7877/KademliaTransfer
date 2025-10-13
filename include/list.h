#pragma once

#include "shared.h"

#pragma once

/**
 * @file list.h
 * @brief Data structures and functions for manipulating doubly-linked lists
 *
 * Each bucket in the Kademlia k-buckets structure consists of a doubly-linked
 * list to allow quick insertion and removal from the head and tail
 *
 */

struct Peer;

/**
 * @brief Represents a single node of a doubly linked list
 *
 */
struct DNode {
  /**
   * @brief The value stored for the node
   *
   */
  struct Peer* peer;

  /**
   * @brief The next node in the list, or NULL if there are none
   *
   */
  struct DNode* next;

  /**
   * @brief The previous node in the list, or NULL if there are none
   *
   */
  struct DNode* prev;
};

/**
 * @brief Implementation of a doubly linked list
 *
 */
struct DList {
  /**
   * @brief The current number of nodes in the list
   *
   */
  size_t size;

  /**
   * @brief The head of the list
   *
   */
  struct DNode* head;

  /**
   * @brief The tail of the list
   *
   */
  struct DNode* tail;
};

/**
 * @brief Adds a node to the head of the linked list
 *
 * @param list The list to which to add the node
 * @param peer The peer to add to the list
 */
void add_front(struct DList* list, struct Peer* peer);

/**
 * @brief Adds a node to the tail of the linked list
 *
 * @param list The list to which to add the node
 * @param peer The peer to add to the list
 */
void add_back(struct DList* list, struct Peer* peer);

/**
 * @brief Removes the node at the front of the list
 *
 * @param list The list from which to remove a node
 * @return Peer* The pointer to the popped node, or NULL if there were none
 */
struct Peer* remove_front(struct DList* list);

/**
 * @brief Removes the node at the back of the list
 *
 * @param list The list from which to remove a node
 * @return Peer* The pointer to the popped node, or NULL if there were none
 */
struct Peer* remove_back(struct DList* list);

/**
 * @brief Calculate the XOR distance between two HashID
 * @param result Buffer where the result is stored
 * @param id1 First HashID
 * @param id2 Second HashID
 */
void dist_hash(HashID* result, const HashID* id1, const HashID* id2);

/**
 * @brief Finds the nearest node to a hash in the linked list
 *
 * @param list The list in which to search
 * @param id The id that should be matched as closely as possible
 * @param out_peers 
 * @param max_neighbors 
 * @return int 
 */
int find_nearest(const struct DList* list, const HashID* id,
                           struct Peer** out_peers,
                           size_t max_neighbors);
