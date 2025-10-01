
// Kademlia RPC functions implementation
#pragma once
#include <openssl/sha.h>
#include <stdbool.h>

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
  // TODO : [key;value] pair
};

struct RPCFind {
  struct RPCMessageHeader header;
  unsigned char key[SHA256_DIGEST_LENGTH];
};

struct RPCResponse {};