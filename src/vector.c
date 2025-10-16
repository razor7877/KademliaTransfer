#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "shared.h"
#include "vector.h"

void vector_init(VectorPtr *vec) {
  if (!vec) {
    log_msg(LOG_ERROR, "vector_init called with NULL vec");
    return;
  }

  vec->data = NULL;
  vec->size = 0;
  vec->capacity = 0;
}

void vector_push(VectorPtr *vec, void *elem) {
  if (!vec) {
    log_msg(LOG_ERROR, "vector_push called with NULL vec");
    return;
  }

  if (vec->size == vec->capacity) {
    vec->capacity = vec->capacity ? vec->capacity * 2 : 8;
    vec->data = realloc(vec->data, vec->capacity * sizeof(void *));
    pointer_not_null(vec->data, "vector_push realloc failed");
  }

  vec->data[vec->size++] = elem;
}

void vector_free(VectorPtr *vec, bool free_elements) {
  if (!vec) {
    log_msg(LOG_ERROR, "vector_free called with NULL vec");
    return;
  }

  if (free_elements) {
    for (size_t i = 0; i < vec->size; i++) {
      free(vec->data[i]);
    }
  }

  free(vec->data);
  vec->data = NULL;
  vec->size = 0;
  vec->capacity = 0;
}

void *vector_get(VectorPtr *vec, size_t index) {
  if (!vec) {
    log_msg(LOG_ERROR, "vector_get called with NULL vec");
    return NULL;
  }

  if (index >= vec->size)
    return NULL;

  return vec->data[index];
}

void vector_set(VectorPtr *vec, size_t index, void *elem) {
  if (!vec) {
    log_msg(LOG_ERROR, "vector_set called with NULL vec");
    return;
  }

  if (index >= vec->size)
    return;

  vec->data[index] = elem;
}

void vector_sort(VectorPtr *vec, VectorCmpFunc cmp, const void *userdata) {
  if (!vec || !cmp || vec->size < 2)
    return;

  // Stable bubble sort
  for (size_t i = 0; i < vec->size - 1; i++) {
    for (size_t j = 0; j < vec->size - 1 - i; j++) {
      void *a = vec->data[j];
      void *b = vec->data[j + 1];

      if (!a || !b)
        continue;

      if (!cmp(a, b, userdata)) { // if a > b, swap
        void *tmp = vec->data[j];
        vec->data[j] = vec->data[j + 1];
        vec->data[j + 1] = tmp;
      }
    }
  }
}
