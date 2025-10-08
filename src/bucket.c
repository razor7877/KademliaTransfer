#include <openssl/evp.h>
#include <string.h>

#include "bucket.h"
#include "log.h"
#include "shared.h"

struct Peer* find_closest_peers(Buckets* buckets, HashID* target, int n) {
    struct Peer* result = malloc(n * sizeof(struct Peer));
    pointer_not_null(result, "find_closest_peers malloc error");
    
    int count = 0;
    HashID own_id = {0};

    if (get_own_id(&own_id) != 0) {
        log_msg(LOG_ERROR, "find_closest_peers get_own_id error");
        return NULL;
    }

    return result;
}

void update_bucket_peers(struct Peer* peer) {
    log_msg(LOG_DEBUG, "update_bucket_peers");
}
