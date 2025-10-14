#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <openssl/evp.h>

#include "shared.h"

struct RPCPeer;

/**
 * @file peer.h
 * @brief Data structures and functions for manipulating peers
 * 
 * 
 */

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

/**
 * @brief Serializes a struct Peer to a struct RPCPeer
 * 
 * @param peer The peer to serialize
 * @param serialized A pointer to store the serialized peer
 * @return int 0 if the peer was serialized correctly, a negative number otherwise
 */
int serialize_rpc_peer(const struct Peer* peer, struct RPCPeer* serialized);

/**
 * @brief Deserializes a struct RPCPeer to a struct Peer
 * 
 * @param peer The peer to deserialize
 * @param deserialized A pointer to memory that will store the deserialized peer
 * @return int 0 if the peer was deserialized correctly, a negative number otherwise
 */
int deserialize_rpc_peer(const struct RPCPeer* peer, struct Peer* deserialized);
