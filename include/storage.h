#pragma once

#include "shared.h"
#include "peer.h"
#include "rpc.h"

/**
 * @file storage.h
 * @brief Kademlia key-value storage
 * 
 * This file defines the interfaces for interacting with the key-value pair storage of the client.
 * It allows storing or querying existing values from the storage
 * 
 */

/**
 * @brief Represents a key-value pair store
 * 
 */
struct KeyValuePair {
  HashID key;
  size_t num_values;
  struct Peer values[K_VALUE];
};

/**
 * @brief Queries a key from the client storage
 * 
 * @param key The key to be queried for in the client storage
 * @return const struct KeyValuePair* Returns the key-value pair if it exists, NULL otherwise
 */
const struct KeyValuePair* storage_get_value(HashID* key);

/**
 * @brief Stores a key-value pair in the client storage
 * 
 * @param value The key-value pair to be stored into the client storage
 */
void storage_put_value(struct KeyValuePair* value);

struct RPCKeyValue* serialize_rpc_value(const struct KeyValuePair* value);
struct KeyValuePair* deserialize_rpc_value(const struct RPCKeyValue* value);
