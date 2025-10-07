#pragma once

#include "shared.h"
#include "list.h"

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
