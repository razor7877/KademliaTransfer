#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <openssl/evp.h>

#include "shared.h"

/**
 * @brief Represents a single peer in the network
 * 
 */
struct Peer {
    /**
     * @brief The 256 bit identifier for the node
     * 
     */
    HashID peer_id;

    /**
     * @brief The network information for communicating with the node
     * 
     */
    struct sockaddr_in peer_addr;

    /**
     * @brief When this peer was last seen
     * 
     */
    time_t last_seen;

    /**
     * @brief The public key of the node
     * 
     */
    EVP_PKEY* peer_pub_key;
};