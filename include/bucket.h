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

struct Peer** find_closest_peers(Buckets buckets, HashID target, int n);

void update_bucket_peers(Buckets bucket, struct Peer* peer);
