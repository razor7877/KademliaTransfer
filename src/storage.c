#include <stdbool.h>
#include <string.h>

#include <hash/hashmap.h>

#include "storage.h"
#include "log.h"

static bool storage_ready = false;

static struct hashmap* storage_map = NULL;

static int storage_compare(const void* a, const void* b, void* udata) {
    const struct KeyValuePair* ua = a;
    const struct KeyValuePair* ub = b;

    return memcmp(ua->key, ub->key, sizeof(ua->key));
}

static bool storage_iter(const void* item, const void* udata) {
    const struct KeyValuePair* pair = item;

    log_msg(LOG_DEBUG, "Print out storage item data...");
}

static uint64_t storage_hash(const void* item, uint64_t seed0, uint64_t seed1) {
    const struct KeyValuePair* pair = item;
    return hashmap_sip(pair->key, sizeof(pair->key), seed0, seed1);
}

static void init_storage() {
    storage_map = hashmap_new(sizeof(struct KeyValuePair), 0, 0, 0,
                              storage_hash, storage_compare, NULL, NULL);
}

const struct KeyValuePair* storage_get_value(HashID* key) {
    log_msg(LOG_DEBUG, "storage_get_value");

    struct KeyValuePair find = {0};
    memcpy(find.key, key, sizeof(find.key));

    return hashmap_get(storage_map, &find);
}

void storage_put_value(struct KeyValuePair* value) {
    log_msg(LOG_DEBUG, "storage_put_value");

    if (hashmap_get(storage_map, value)) {
        log_msg(LOG_WARN, "Got store on existing key-value pair");
        return;
    }

    hashmap_set(storage_map, value);
}
