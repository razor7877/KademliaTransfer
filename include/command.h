#pragma once

#include <pthread.h>
#include <stdbool.h>

/**
 * @file command.h
 * @brief Data structures and functions for implementation of a command queue
 *
 * This file contains structures and functions to create and manipulate a
 * command queue. It is used for communication between the frontend and clients
 * thread. The frontend thread pushes commands to the queue whenever it needs
 * the client to do something.
 *
 */

#define MAX_COMMANDS_PENDING 10

/**
 * @brief Describes the different commands that can be issued to the P2P client
 *
 */
enum CommandType { CMD_NONE, CMD_SHOW_STATUS, CMD_UPLOAD, CMD_DOWNLOAD };

/**
 * @brief Represents a single command to be issued to the P2P client
 *
 */
struct Command {
  /**
   * @brief The type of command to issue
   *
   */
  enum CommandType cmd_type;

  /**
   * @brief For CMD_UPLOAD & CMD_DOWNLOAD, the information about the file
   *
   */
  struct FileMagnet *file;

  /**
   * @brief Lock for acquiring the Command object
   *
   */
  pthread_mutex_t lock;

  /**
   * @brief Condition variable for the caller thread to wait for result
   *
   */
  pthread_cond_t cond;

  /**
   * @brief Whether the command was executed
   *
   */
  bool done;

  /**
   * @brief The result of the command
   *
   */
  int result;
};

/**
 * @brief A queue of commands issued to the P2P client
 *
 */
struct CommandQueue {
  struct Command *items[MAX_COMMANDS_PENDING];
  int head;
  int tail;
  int count;
  pthread_mutex_t lock;
};

bool command_init(struct Command *cmd);
void command_destroy(struct Command *cmd);

/**
 * @brief Initializes the command queue
 *
 * @param q The command queue to be initialized
 * @return int Returns 0 if the initialization was successful, less than 0
 * otherwise
 */
int queue_init(struct CommandQueue *q);

/**
 * @brief Pushes a new command to the queue
 *
 * @param q The queue to push the command in
 * @param cmd The command to be pushed to the queue
 * @return true The command was successfully pushed to the queue
 * @return false The command was not successfully pushed to the queue (full
 * queue)
 */
bool queue_push(struct CommandQueue *q, struct Command *cmd);

/**
 * @brief Pops a command from the queue if there is at least one
 *
 * @param q The queue to pop a command from
 * @param out_cmd A pointer to memory that will store data from the popped
 * command (if pop was successful)
 * @return true The command was successfully popped from the queue
 * @return false The command was not successfully popped from the queue (empty
 * queue)
 */
bool queue_pop(struct CommandQueue *q, struct Command **out_cmd);

/**
 * @brief Cleans up the state associated with a queue
 *
 * @param q The queue to destroy
 */
void queue_destroy(struct CommandQueue *q);
