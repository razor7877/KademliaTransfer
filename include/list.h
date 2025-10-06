#include "shared.h"

#pragma once

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
struct Peer* remove_back(struct DList* list, struct Peer* peer);

/**
 * @brief Finds the nearest node to a hash in the linked list
 *
 * @param list The list in which to search
 * @param id The id that should be matched as closely as possible
 * @return Peer* A pointer to the nearest node in the list, or NULL if there are
 * none
 */
struct Peer* find_nearest(const struct DList* list, const HashID* id);
