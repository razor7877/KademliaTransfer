#pragma once

#include "list.h"
#include "peer.h"
#include "shared.h"

/**
 * @file bucket.h
 * @brief Data structures and constants for the Kademlia k-buckets
 *
 *
 */

/**
 * @brief The number of node entries in a given k-bucket
 *
 */
#define BUCKET_SIZE K_VALUE

/**
 * @brief The number of buckets in the k-buckets structure
 *
 */
#define BUCKET_COUNT 8

typedef struct DList Buckets[BUCKET_COUNT];

/**
 * @brief Finds and returns the n closest peers to our own node
 *
 * @param buckets The buckets to search for peers in
 * @param target The target we are looking for
 * @param n The maximum number of close peers to search for
 * @return struct Peer** Returns an array of struct Peer*, entries are filled
 * out with NULL if less than n peers were found
 */
struct Peer **find_closest_peers(Buckets buckets, const HashID target, int n);

/**
 * @brief Updates the peers in our buckets, should be called whenever we
 * interact we a potentially new peer
 *
 * @param bucket The buckets to updade peers with
 * @param peer The peer that was interacted with
 */
void update_bucket_peers(Buckets bucket, struct Peer *peer);
