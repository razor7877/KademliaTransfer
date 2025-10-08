#include <stdbool.h>
#include <string.h>

#include <hash/hashmap.h>

#include "storage.h"
#include "log.h"

static bool storage_ready = false;

static struct hashmap* storage_map = NULL;

/**
 * @brief Defines a comparator for two hashmap items
 * 
 * @param a The first item
 * @param b The second item
 * @param udata User supplied data (unused)
 * @return int 0 if they are the same items, any other value otherwise
 */
static int storage_compare(const void* a, const void* b, void* udata) {
    const struct KeyValuePair* ua = a;
    const struct KeyValuePair* ub = b;

    return memcmp(ua->key, ub->key, sizeof(ua->key));
}

/**
 * @brief Defines an iterator for the hashmap items
 * 
 * @param item The item being iterated over
 * @param udata User supplied data (unused)
 * @return true Always returned
 * @return false Never returned
 */
static bool storage_iter(const void* item, const void* udata) {
    const struct KeyValuePair* pair = item;

    log_msg(LOG_DEBUG, "Print out storage item data...");

    return true;
}

/**
 * @brief Defines how to calculate the hash of an item in the hashmap
 * 
 * @param item The item to be hashed
 * @param seed0 The first hashmap seed
 * @param seed1 The second hashmap seed
 * @return uint64_t Returns a uint64_t hash of the item
 */
static uint64_t storage_hash(const void* item, uint64_t seed0, uint64_t seed1) {
    const struct KeyValuePair* pair = item;
    return hashmap_sip(pair->key, sizeof(pair->key), seed0, seed1);
}

/**
 * @brief Initializes the storage for use
 * 
 */
static void storage_init() {
    log_msg(LOG_DEBUG, "Initializing client storage");

    storage_map = hashmap_new(sizeof(struct KeyValuePair), 0, 0, 0,
                              storage_hash, storage_compare, NULL, NULL);
    
    storage_ready = true;
}

const struct KeyValuePair* storage_get_value(HashID* key) {
    if (!storage_ready) storage_init();

    log_msg(LOG_DEBUG, "storage_get_value");

    struct KeyValuePair find = {0};
    memcpy(find.key, key, sizeof(find.key));

    return hashmap_get(storage_map, &find);
}

void storage_put_value(struct KeyValuePair* value) {
    if (!storage_ready) storage_init();

    log_msg(LOG_DEBUG, "storage_put_value");

    if (hashmap_get(storage_map, value)) {
        log_msg(LOG_WARN, "Got store on existing key-value pair");
        return;
    }

    hashmap_set(storage_map, value);
}

struct RPCKeyValue* serialize_rpc_value(const struct KeyValuePair* value) {
    struct RPCKeyValue* serialized = malloc(sizeof(struct RPCKeyValue));

    pointer_not_null(serialized, "malloc error in deserialize_rpc_value");

    memcpy(serialized->key, value->key, sizeof(value->key));
    serialized->num_values = value->num_values;

    for (int i = 0; i < value->num_values; i++) {
        struct RPCPeer* p = serialize_rpc_peer(&value->values[i]);
        // Copy it to the struct
        serialized->values[i] = *p;

        free(p);
    }

    return serialized;
}

struct KeyValuePair* deserialize_rpc_value(const struct RPCKeyValue* value) {
    struct KeyValuePair* deserialized = malloc(sizeof(struct KeyValuePair));

    pointer_not_null(deserialized, "malloc error in deserialize_rpc_value");

    memcpy(deserialized->key, value->key, sizeof(value->key));
    deserialized->num_values = value->num_values;

    for (int i = 0; i < value->num_values; i++) {
        struct Peer* p = deserialize_rpc_peer(&value->values[i]);
        // Copy it to the struct
        deserialized->values[i] = *p;

        free(p);
    }

    return deserialized;
}
