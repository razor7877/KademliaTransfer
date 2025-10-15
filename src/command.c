#include <stdio.h>
#include <string.h>

#include "command.h"

bool command_init(struct Command *cmd) {
  if (!cmd)
    return false;

  memset(cmd, 0, sizeof(*cmd));
  if (pthread_mutex_init(&cmd->lock, NULL) != 0) {
    perror("pthread_mutex_init");
    return false;
  }

  if (pthread_cond_init(&cmd->cond, NULL) != 0) {
    perror("pthread_cond_init");
    pthread_mutex_destroy(&cmd->lock);
    return false;
  }

  cmd->done = false;
  cmd->result = 0;

  return true;
}

void command_destroy(struct Command *cmd) {
  if (!cmd)
    return;

  pthread_mutex_destroy(&cmd->lock);
  pthread_cond_destroy(&cmd->cond);
}

int queue_init(struct CommandQueue *q) {
  if (!q)
    return -1;

  q->head = 0;
  q->tail = 0;
  q->count = 0;

  return pthread_mutex_init(&q->lock, NULL);
}

bool queue_push(struct CommandQueue *q, struct Command *cmd) {
  if (!q || !cmd)
    return false;

  pthread_mutex_lock(&q->lock);

  if (q->count == MAX_COMMANDS_PENDING) {
    pthread_mutex_unlock(&q->lock);
    return false;
  }

  q->items[q->tail] = cmd;
  q->tail = (q->tail + 1) % MAX_COMMANDS_PENDING;
  q->count++;

  pthread_mutex_unlock(&q->lock);

  return true;
}

bool queue_pop(struct CommandQueue *q, struct Command **out_cmd) {
  if (!q || !out_cmd)
    return false;

  pthread_mutex_lock(&q->lock);

  if (q->count == 0) {
    pthread_mutex_unlock(&q->lock);
    return false;
  }

  *out_cmd = q->items[q->head];
  q->head = (q->head + 1) % MAX_COMMANDS_PENDING;
  q->count--;

  pthread_mutex_unlock(&q->lock);
  return true;
}

void queue_destroy(struct CommandQueue *q) {
  if (!q)
    return;

  pthread_mutex_destroy(&q->lock);
  q->head = 0;
  q->tail = 0;
  q->count = 0;
}