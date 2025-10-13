#include "bucket.h"

#include <openssl/evp.h>
#include <string.h>

#include "list.h"
#include "log.h"
#include "shared.h"

/**
 * @brief Gets the bucket index depending on the distance from our node
 *
 * @param distance The distance from our node
 * @return int Returns the bucket to choose for this distance, or -1 if the node
 * is ourselves
 */
static int get_bucket_index(HashID distance) {
  for (int byte = 0; byte < sizeof(HashID); byte++) {
    unsigned char byte_value = distance[byte];
    for (int bit = 0; bit < 8; bit++) {
      if (byte_value & (0x80 >> bit)) {
        // log_msg(LOG_WARN, "byte is %d and bit is %d", byte, bit);
        return byte * 8 + bit;
      }
    }
  }

  return -1;
};

struct Peer** find_closest_peers(Buckets buckets, HashID target, int n) {
  // log_msg(LOG_DEBUG, "Searching for %d closest peers to ")
  struct Peer** result = calloc(n, sizeof(struct Peer*));
  pointer_not_null(result, "find_closest_peers malloc error");

  char target_str[sizeof(HashID) * 2 + 1] = {0};
  sha256_to_hex(target, target_str);

  log_msg(LOG_DEBUG, "Searching %d closest peers to target %s", n, target_str);

  int count = 0;
  HashID own_id = {0};

  if (get_own_id(own_id) != 0) {
    log_msg(LOG_ERROR, "find_closest_peers get_own_id error");
    return NULL;
  }

  HashID dist_to_target = {0};
  // Calculate distance between us and the target
  dist_hash(dist_to_target, own_id, target);

  char dist_str[sizeof(HashID) * 2 + 1] = {0};
  sha256_to_hex(dist_to_target, dist_str);

  log_msg(LOG_DEBUG, "Distance from us to target is %s", dist_str);

  int bucket_index = get_bucket_index(dist_to_target);

  if (bucket_index == -1) {
    log_msg(LOG_WARN, "find_closest_peers - Why are we looking for ourselves?");
    return NULL;
  }

  log_msg(LOG_DEBUG, "Bucket index for this distance is %d", bucket_index);

  // First get nodes from the bucket with nodes closest to the target
  count +=
      find_nearest(&buckets[bucket_index], target, result + count, n - count);

  // Extend our range to neighboring buckets until we searched everything or
  // found the number of requested peers
  int offset = 1;

  while (count < n && (bucket_index - offset >= 0 || bucket_index + offset < BUCKET_COUNT)) {
    if (bucket_index - offset >= 0)
      count += find_nearest(&buckets[bucket_index - offset], target,
                            result + count, n - count);

    if (bucket_index + offset < BUCKET_COUNT)
      count += find_nearest(&buckets[bucket_index + offset], target,
                            result + count, n - count);

    offset++;
  }

  if (count == 0) {
    free(result);
    return NULL;
  }

  return result;
}

// This shall be improved upon later on, we should ideally do ranking using last_seen timestamps
void update_bucket_peers(Buckets bucket, struct Peer* peer) {
  if (!peer) {
    log_msg(LOG_ERROR, "Error in update_bucket_peers peer is NULL!");
    return;
  }

  if (!bucket) {
    log_msg(LOG_ERROR, "Error in update_bucket_peers bucket is null");
    return;
  }

  HashID own_id;
  int ret = get_own_id(own_id);

  if (ret != 0) {
    log_msg(LOG_ERROR, "Error in update_bucket_peers get_own_id!");
    return;
  }

  HashID distance;
  dist_hash(distance, own_id, peer->peer_id);

  // Find the bucket in which we should store the peer
  int bucket_index = get_bucket_index(distance);

  if (bucket_index == -1) {
    log_msg(LOG_ERROR, "Error in update_bucket_peers bucket_index!");
    return;
  }

  // log_msg(LOG_DEBUG, "bucket_index is %d", bucket_index);

  if (bucket[bucket_index].size >= BUCKET_SIZE) {
    log_msg(LOG_DEBUG, "[update_bucket_peers]: The Bucket is full");
    return;
  }

  if (find_peer_by_id(&bucket[bucket_index], peer->peer_id)) {
    // log_msg(LOG_DEBUG, "[update_bucket_peers]: Peer already present in bucket");
    return;
  }

  log_msg(LOG_DEBUG, "got new peer in bucket with port %d", ntohs(peer->peer_addr.sin_port));
  add_front(&bucket[bucket_index], peer); 
}
