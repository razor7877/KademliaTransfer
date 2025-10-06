// TODO : Buckets implementation
// Functions for manipulating bucket contents (nearest neighbor find etc.)

#include "shared.h"
#include "list.h"

#define BUCKET_SIZE K_VALUE
#define BUCKET_COUNT 8

typedef struct DList Buckets[BUCKET_COUNT];
