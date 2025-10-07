#pragma once

// Kademlia RPC functions implementation
#include <openssl/sha.h>
#include <stdbool.h>
#include <arpa/inet.h>

#include "bucket.h"
#include "shared.h"

/**
 * @file rpc.h
 * @brief Remote Procedure Calls Protocol
 * 
 * This file defines all the main structures and functions used for the P2P peer discovery and communication.
 * It defines the structure of the different Kademlia packets, and how data such as peer information should be serialized.
 * 
 */

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define MAX_RPC_PACKET_SIZE \
    MAX(sizeof(struct RPCPing), \
    MAX(sizeof(struct RPCStore), \
    MAX(sizeof(struct RPCFind), \
    MAX(sizeof(struct RPCResponse), \
    MAX(sizeof(struct RPCFindValueResponse), \
        sizeof(struct RPCFindNodeResponse))))))

#define RPC_MAGIC "KDMT"

#pragma pack(push, 1)

enum RPCCallType {
  PING = 1,
  STORE = 2,
  FIND_NODE = 4,
  FIND_VALUE = 8,
  PING_RESPONSE = 16,
  STORE_RESPONSE = 32,
  FIND_NODE_RESPONSE = 64,
  FIND_VALUE_RESPONSE = 128
};

struct RPCPeer {
  HashID peer_id;
  struct sockaddr_in peer_addr;
  PubKey peer_key;
};

struct RPCKeyValue {
  HashID key;
  size_t num_values;
  struct RPCPeer values[K_VALUE];
};

struct RPCMessageHeader {
  char magic_number[4];
  int packet_size;
  enum RPCCallType call_type;
};

struct RPCPing {
  struct RPCMessageHeader header;
};

struct RPCStore {
  struct RPCMessageHeader header;
  struct RPCKeyValue key_value;
};

struct RPCFind {
  struct RPCMessageHeader header;
  HashID key;
};

struct RPCResponse {
  struct RPCMessageHeader header;
  uint8_t success;
};

struct RPCFindValueResponse {
  struct RPCMessageHeader header;
  uint8_t found_key;
  struct RPCKeyValue values;
};

struct RPCFindNodeResponse {
  struct RPCMessageHeader header;
  uint8_t found_key;
  size_t num_closest;
  struct RPCPeer closest[K_VALUE];
};

#pragma pack(pop)

/**
 * @brief Handles a RPC request
 * 
 * @param contents The RPC request packet
 * @param length The length of the RPC request
 */
void handle_rpc_request(struct pollfd* sock, char* contents, size_t length);
