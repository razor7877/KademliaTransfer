#pragma once

#include <netinet/in.h>
#include <poll.h>

/**
 * @file network.h
 * @brief Network transport functionality : Sending over TCP, TLS etc.
 *
 * This file defines an interface for managing the network layer. It abstracts
 * its behavior and exposes simple functions to start/update/stop. The network
 * layer accepts new connections and handles requests, dispatching them to the
 * corresponding handling layers (HTTP or RPC).
 *
 */

#define SERVER_PORT 8182
#define BROADCAST_PORT 8183

/**
 * @brief Gets the contents of the entire RPC request according to the request
 *
 * @param sock The struct of the socket that sent the RPC request
 * @param buf A pointer to the buffer where data can be read into
 * @param out_size A pointer to store the size of the packet that was read
 */
int get_rpc_request(const struct pollfd *sock, char *buf, size_t *out_size);

/**
 * @brief Initializes the network stack
 *
 */
void init_network();

/**
 * @brief Updates the network
 *
 */
void update_network();

/**
 * @brief Stops and cleans up the network stack
 *
 */
void stop_network();

/**
 * @brief Connects to a peer
 *
 * @param addr The peer to connect to
 * @return int The socket fd if connection was successful, a negative number
 * otherwise
 */
int connect_to_peer(const struct sockaddr_in *addr);
