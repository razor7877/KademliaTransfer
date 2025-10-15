#pragma once
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Generic dynamic array (vector) of pointers
 */
typedef struct {
  /**
   * @brief Array of pointers holding any kind of data
   *
   */
  void **data;

  /**
   * @brief Current size of vector
   *
   */
  size_t size;

  /**
   * @brief Current capacity of vector
   *
   */
  size_t capacity;
} VectorPtr;

/**
 * @brief Comparator function for vector_sort with userdata
 *
 * @param a Pointer to first element
 * @param b Pointer to second element
 * @param userdata User-defined context pointer
 * @return true if a < b, false otherwise
 */
typedef bool (*VectorCmpFunc)(void *a, void *b, const void *userdata);

/**
 * @brief Initialize a vector
 *
 * @param vec Pointer to vector
 */
void vector_init(VectorPtr *vec);

/**
 * @brief Push a new element to the vector
 *
 * @param vec Pointer to vector
 * @param elem Pointer to element
 */
void vector_push(VectorPtr *vec, void *elem);

/**
 * @brief Free vector resources, optionally freeing contained elements
 *
 * @param vec Pointer to vector
 * @param free_elements True to free each element, false to leave them
 */
void vector_free(VectorPtr *vec, bool free_elements);

/**
 * @brief Get element at index
 *
 * @param vec Pointer to vector
 * @param index Index to fetch
 * @return void* Pointer to element
 */
void *vector_get(VectorPtr *vec, size_t index);

/**
 * @brief Set element at index
 *
 * @param vec Pointer to vector
 * @param index Index to set
 * @param elem Pointer to element
 */
void vector_set(VectorPtr *vec, size_t index, void *elem);

/**
 * @brief Sort a VectorPtr using a user-supplied comparison function with
 * userdata
 *
 * @param vec Pointer to the vector
 * @param cmp Comparison function returning true if a < b
 * @param userdata Pointer to user data for the comparator
 */
void vector_sort(VectorPtr *vec, VectorCmpFunc cmp, const void *userdata);
