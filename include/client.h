#pragma once

#include "magnet.h"

extern struct CommandQueue commands;

/**
 * @file client.h
 * @brief Interface between frontend (CLI, GUI) and the P2P client code
 * 
 * This file defines the high-level functions that may called by the application frontend, to set up the P2P client and interact with it
 * 
 */

/**
 * @brief Initializes the state of the P2P client which runs on its own thread
 * 
 */
int start_client();

/**
 * @brief Entry point of the P2P client thread
 * 
 */
void* init_client(void* arg);

/**
 * @brief Update loop of the P2P client
 * 
 */
void update_client();

/**
 * @brief Stops and cleans up the resources of the P2P client
 * 
 */
void stop_client();

/**
 * @brief Downloads a file from the P2P network
 * 
 * @param file The file to be downloaded
 * @return int 0 if the file was downloaded successfully, a negative number otherwise
 */
int download_file(struct FileMagnet* file);

/**
 * @brief Uploads a file to the P2P network
 * 
 * @param file The file to be uploaded
 * @return int int 0 if the file was downloaded successfully, a negative number otherwise
 */
int upload_file(struct FileMagnet* file);

/**
 * @brief Prints out the current network status to the TTY
 * 
 */
void show_network_status();
