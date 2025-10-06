#pragma once

#include <stddef.h>
#include <poll.h>

#include "shared.h"
#include "bucket.h"

// Implementation of HTTP client server interactions

/**
 * @brief Handles a HTTP request
 * 
 * @param contents The contents of the HTTP request
 * @param length The length of the HTTP packet
 */
void handle_http_request(struct pollfd* sock, char* contents, size_t length);

/**
 * @brief Downloads a file by hash from the HTTP server served by peer
 * 
 * @param peer The peer from which to download the file
 * @param file The hash of the file to be downloaded
 * @return int 0 if it was successful, negative number otherwise
 */
int download_http_file(struct Peer* peer, HashID* file);
