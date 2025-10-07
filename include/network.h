#pragma once

/**
 * @file network.h
 * @brief Network transport functionality : Sending over TCP, TLS etc.
 * 
 * This file defines an interface for managing the network layer. It abstracts its behavior and exposes simple functions to start/update/stop.
 * The network layer accepts new connections and handles requests, dispatching them to the corresponding handling layers (HTTP or RPC). 
 * 
 */

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
