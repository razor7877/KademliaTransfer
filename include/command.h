#pragma once

#include <stdbool.h>
#include <pthread.h>

/**
 * @file command.h
 * @brief Data structures and functions for implementation of a command queue
 * 
 * This file contains sturctures and functions to create and manipulate a command queue.
 * It is used for communication between the frontend and clients thread.
 * The frontend thread pushes commands to the queue whenever it needs the client to do something.
 * 
 */

/**
 * @brief Describes the different commands that can be issued to the P2P client
 * 
 */
enum CommandType {
    CMD_NONE,
    CMD_SHOW_STATUS,
    CMD_UPLOAD,
    CMD_DOWNLOAD
};

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
    struct FileMagnet* file;
};

#define MAX_COMMANDS_PENDING 10

/**
 * @brief A queue of commands issued to the P2P client
 * 
 */
struct CommandQueue {
    struct Command items[MAX_COMMANDS_PENDING];
    int head;
    int tail;
    int count;
    pthread_mutex_t lock;
};

int queue_init(struct CommandQueue* q);
bool queue_push(struct CommandQueue* q, const struct Command* cmd);
bool queue_pop(struct CommandQueue* q, struct Command* out_cmd);
void queue_destroy(struct CommandQueue* q);
