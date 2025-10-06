#pragma once

#include "magnet.h"

// Interface between frontend (CLI, GUI) and the P2P network code

/**
 * @brief Initializes the state of the P2P client which runs on its own thread
 * 
 */
void start_client();

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
