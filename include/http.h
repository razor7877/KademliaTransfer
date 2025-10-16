#pragma once

#include <poll.h>
#include <stddef.h>

#include "magnet.h"
#include "shared.h"

/**
 * @file http.h
 * @brief Implementation of HTTP client server interactions
 *
 * This file defines functions for handling incoming HTTP request passed on by
 * the network layer, as well as downloading files from other peers
 *
 */

#define HTTP_HEADER_SIZE 8192
#define CHUNK_SIZE 4096

/**
 * @brief Handles a HTTP request
 *
 * @param sock The socket that sent the HTTP request
 * @param contents The contents of the HTTP request
 * @param length The length of the HTTP packet
 */
void handle_http_request(const struct pollfd *sock, const char *contents, size_t length);

/**
 * @brief Downloads a file by hash from the HTTP server served by peer
 *
 * @param peer The peer from which to download the file
 * @param file The hash of the file to be downloaded
 * @return int 0 if it was successful, negative number otherwise
 */
int download_http_file(const struct Peer *peer, const struct FileMagnet *file);
