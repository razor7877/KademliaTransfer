
// Kademlia RPC functions implementation
#pragma once
#include <openssl/sha.h>
#include <stdbool.h>
#include <arpa/inet.h>

#include "bucket.h"
#include "shared.h"

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
  RPCPeer values[K_VALUE];
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
  RPCKeyValue key_value;
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
  RPCKeyValue values;
};

struct RPCFindNodeResponse {
  struct RPCMessageHeader header;
  uint8_t found_key;
  size_t num_closest;
  RPCPeer closest[K_VALUE];
};

#pragma pack(pop)

/**
 * @brief Handles a RPC request
 * 
 * @param contents The RPC request packet
 * @param length The length of the RPC request
 */
void handle_rpc_request(char* contents, size_t length);
