#include <memory.h>

#include <hash/hashmap.h>

#include "bucket.h"
#include "http.h"
#include "log.h"
#include "magnet.h"
#include "network.h"
#include "rpc.h"
#include "storage.h"
#include "vector.h"

/**
 * @brief Kademlia neighbor buckets
 *
 */
static Buckets buckets = {0};

static void handle_ping(const struct pollfd *sock, struct RPCPing *data) {
  log_msg(LOG_DEBUG, "Handling RPC ping");

  struct RPCResponse response = {
      .header = {.magic_number = RPC_MAGIC,
                 .call_type = PING_RESPONSE,
                 .packet_size = sizeof(struct RPCResponse)},
      .success = true};

  send_all(sock->fd, &response, sizeof(response));
}

static void handle_store(const struct pollfd *sock,
                         const struct RPCStore *data) {
  log_msg(LOG_DEBUG, "Handling RPC store");

  struct KeyValuePair kvp;
  deserialize_rpc_value(&data->key_value, &kvp);
  storage_put_value(&kvp);

  struct RPCResponse response = {
      .header = {.magic_number = RPC_MAGIC,
                 .call_type = STORE_RESPONSE,
                 .packet_size = sizeof(struct RPCResponse)},
      .success = true};

  send_all(sock->fd, &response, sizeof(response));
}

static void handle_find_node(const struct pollfd *sock,
                             const struct RPCFind *data) {
  log_msg(LOG_DEBUG, "Handling RPC find node");

  struct RPCFindNodeResponse response = {
      .header = {.magic_number = RPC_MAGIC,
                 .call_type = FIND_NODE_RESPONSE,
                 .packet_size = sizeof(struct RPCFindNodeResponse)},
      .success = true,
      .found_key = false,
      .num_closest = 0,
      .closest = {{{0}}}};

  int count = BUCKET_SIZE;
  struct Peer **closest = find_closest_peers(buckets, data->key, count);

  if (closest == NULL) {
    log_msg(LOG_ERROR, "handle_find_node find_closest_peers returned NULL");
    // Send error response
    send_all(sock->fd, &response, sizeof(response));
    return;
  }

  // Ugly, need to just return number found from find_closest_peers
  int found = 0;
  for (int i = 0; i < count; i++) {
    if (closest[i] == NULL)
      continue;

    found++;
    serialize_rpc_peer(closest[i], &response.closest[i]);
  }

  response.num_closest = found;

  send_all(sock->fd, &response, sizeof(response));

  free(closest);
}

static void handle_find_value(const struct pollfd *sock, struct RPCFind *data) {
  log_msg(LOG_DEBUG, "Handling RPC find value");

  struct RPCFindValueResponse response = {
      .header = {.magic_number = RPC_MAGIC,
                 .call_type = FIND_VALUE_RESPONSE,
                 .packet_size = sizeof(struct RPCFindValueResponse)},
      .success = true,
      .found_key = false,
      .values = {{0}},
      .num_closest = 0,
      .closest = {{{0}}}};

  const struct KeyValuePair *kvp = storage_get_value(data->key);

  if (kvp) {
    log_msg(
        LOG_DEBUG,
        "We had the key value pair, returning value from our storage to peer");
    response.found_key = true;
    serialize_rpc_value(kvp, &response.values);

    send_all(sock->fd, &response, sizeof(response));

    return;
  }

  log_msg(LOG_DEBUG, "We don't have the key value pair, returning our "
                     "neighbors closest to target");

  int count = BUCKET_SIZE;
  struct Peer **closest = find_closest_peers(buckets, data->key, count);

  if (closest == NULL) {
    log_msg(LOG_ERROR, "handle_find_node find_closest_peers returned NULL");
    // Send error response
    send_all(sock->fd, &response, sizeof(response));
    return;
  }

  // Ugly, need to just return number found from find_closest_peers
  int found = 0;
  for (int i = 0; i < count; i++) {
    if (closest[i] == NULL)
      continue;

    found++;
    serialize_rpc_peer(closest[i], &response.closest[i]);
  }

  response.num_closest = found;

  send_all(sock->fd, &response, sizeof(response));

  free(closest);
}

static void handle_broadcast(const struct pollfd *sock,
                             const struct RPCBroadcast *data) {
  struct Peer peer;
  // Get the peer object back
  deserialize_rpc_peer(&data->peer, &peer);
  // Update our buckets with the information from this (potentially new) peer
  update_bucket_peers(buckets, &peer);
}

static bool peer_distance_cmp(void *a, void *b, const void *userdata) {
  struct Peer *p1 = a;
  struct Peer *p2 = b;

  unsigned char *target = (unsigned char *)userdata;
  HashID d1, d2;
  dist_hash(d1, p1->peer_id, target);
  dist_hash(d2, p2->peer_id, target);

  return memcmp(d1, d2, sizeof(HashID)) < 0;
}

/**
 * @brief Free an array of Peer pointers
 *
 * @param peers Array of Peer pointers
 * @param count Number of peers in the array
 */
static void free_peer_array(struct Peer **peers, size_t count) {
  if (!peers)
    return;
  for (size_t i = 0; i < count; i++) {
    free(peers[i]);
    peers[i] = NULL;
  }
}

/**
 * @brief Iterative network traversal to search for the closest peers to a
 * target key
 *
 * @param target_key The key to find the closest peers to
 * @param out_peers Points to a vector that will store the most suitable peers
 * @param max_peers How many peers should be returned at most
 * @param find_value FIND_VALUE or FIND_NODE RPC requests
 * @return int Returns 0 if the search was successful, a negative number
 * otherwise
 */
static int iterative_find_peers(const HashID target_key,
                                struct Peer **out_peers, size_t max_peers,
                                bool find_value) {
  if (!out_peers || max_peers == 0) {
    log_msg(LOG_ERROR, "iterative_find_peers: invalid out_peers buffer");
    return -1;
  }

  VectorPtr pending;
  vector_init(&pending);
  VectorPtr contacted;
  vector_init(&contacted);

  // Find the closest potential peers among those we already know of
  struct Peer **initial = find_closest_peers(buckets, target_key, K_VALUE);
  if (initial) {
    for (int i = 0; i < K_VALUE && initial[i]; i++) {
      // Always make a copy to avoid issues with freeing everything later on
      struct Peer *copy = malloc(sizeof(struct Peer));
      pointer_not_null(copy, "iterative_find_peers malloc error");

      memcpy(copy, initial[i], sizeof(struct Peer));
      vector_push(&pending, copy);

      // Initially not contacted
      bool *c = calloc(1, sizeof(bool));
      vector_push(&contacted, c);
    }
    free(initial);
  }

  bool done = false;
  bool value_found = false;

  // Iterative lookup loop to traverse the network
  while (!done && (!find_value || !value_found)) {
    bool any_new_contact = false;

    for (size_t i = 0; i < pending.size; i++) {
      bool *c = (bool *)vector_get(&contacted, i);
      struct Peer *p = (struct Peer *)vector_get(&pending, i);

      // No peer information or already contacted
      if (!p || *c)
        continue;

      *c = true;
      any_new_contact = true;

      // Try contacting this peer to get their closest known peers to our target
      int sock = connect_to_peer(&p->peer_addr);
      if (sock < 0)
        continue;

      struct RPCFind req = {
          .header = {.magic_number = RPC_MAGIC,
                     .call_type = find_value ? FIND_VALUE : FIND_NODE,
                     .packet_size = sizeof(struct RPCFind)}};

      memcpy(req.key, target_key, sizeof(HashID));
      send_all(sock, &req, sizeof(req));

      // Make sure we receive a valid RPC packet back
      char peek_buf[4] = {0};
      if (recv_all_peek(sock, peek_buf, sizeof(peek_buf)) <= 0 ||
          memcmp(peek_buf, RPC_MAGIC, 4) != 0) {
        close(sock);
        continue;
      }

      // Get the entire response contents
      char buf[MAX_RPC_PACKET_SIZE];
      size_t packet_size = 0;
      if (get_rpc_request(&(struct pollfd){.fd = sock}, buf, &packet_size) !=
          0) {
        close(sock);
        continue;
      }

      struct RPCMessageHeader *header = (struct RPCMessageHeader *)buf;

      // Handle FIND_VALUE response (for downloads)
      if (find_value && header->call_type == FIND_VALUE_RESPONSE) {
        struct RPCFindValueResponse *resp = (struct RPCFindValueResponse *)buf;

        // Peer gave us the value we were looking for
        if (resp->found_key) {
          log_msg(LOG_DEBUG, "iterative_find_peers: Found value during lookup");
          struct KeyValuePair kvp;
          deserialize_rpc_value(&resp->values, &kvp);

          for (int j = 0; j < kvp.num_values && j < max_peers; j++) {
            out_peers[j] = malloc(sizeof(struct Peer));
            memcpy(out_peers[j], &kvp.values[j], sizeof(struct Peer));
          }

          // Free all remaining peers in pending to avoid leaks
          // This is also super ugly, theres gotta be a better way
          for (size_t i = 0; i < pending.size; i++) {
            struct Peer *p = vector_get(&pending, i);
            // Skip peers we just moved to out_peers
            bool skip = false;
            for (int j = 0; j < kvp.num_values && j < max_peers; j++) {
              if (p == out_peers[j]) {
                skip = true;
                break;
              }
            }
            if (!skip)
              free(p);
          }

          value_found = true;
          break;
        } else {
          // They didn't have the key-value pair, get their closest neighbors
          // instead
          for (int j = 0; j < resp->num_closest; j++) {
            struct Peer *new_peer = malloc(sizeof(struct Peer));
            deserialize_rpc_peer(&resp->closest[j], new_peer);
            // Update our own neighbor lists
            update_bucket_peers(buckets, new_peer);

            bool exists = false;
            // Check that we didn't already store this peer in our list
            for (size_t s = 0; s < pending.size; s++) {
              struct Peer *q = (struct Peer *)vector_get(&pending, s);
              if (memcmp(&q->peer_id, &new_peer->peer_id, sizeof(HashID)) ==
                  0) {
                exists = true;
                break;
              }
            }
            // Add it to our list of peers to contact
            if (!exists) {
              vector_push(&pending, new_peer);
              bool *new_c = calloc(1, sizeof(bool));
              vector_push(&contacted, new_c);
            } else {
              free(new_peer);
            }
          }
        }
      }

      // Handle FIND_NODE response (for uploads)
      if (!find_value && header->call_type == FIND_NODE_RESPONSE) {
        struct RPCFindNodeResponse *resp = (struct RPCFindNodeResponse *)buf;
        for (int j = 0; j < resp->num_closest; j++) {
          struct Peer *new_peer = malloc(sizeof(struct Peer));
          deserialize_rpc_peer(&resp->closest[j], new_peer);
          // Update our own neighbor lists
          update_bucket_peers(buckets, new_peer);

          bool exists = false;
          for (size_t s = 0; s < pending.size; s++) {
            struct Peer *q = (struct Peer *)vector_get(&pending, s);
            if (memcmp(&q->peer_id, &new_peer->peer_id, sizeof(HashID)) == 0) {
              exists = true;
              break;
            }
          }
          if (!exists) {
            vector_push(&pending, new_peer);
            bool *new_c = calloc(1, sizeof(bool));
            vector_push(&contacted, new_c);
          } else {
            free(new_peer);
          }
        }
      }

      close(sock);

      if (value_found)
        break;
    }

    if (!any_new_contact || pending.size >= max_peers)
      done = true;
  }

  // In case of FIND_NODE, we sort all the peers we contacted and return the
  // closest ones encountered
  if (!find_value) {
    log_msg(LOG_DEBUG, "Before sort peers:");

    for (int i = 0; i < pending.size; i++) {
      struct Peer *p = vector_get(&pending, i);
      char buf[65] = {0};
      sha256_to_hex(p->peer_id, buf);

      log_msg(LOG_DEBUG, "pending[%d]->peer_id = %s", i, buf);
    }

    vector_sort(&pending, peer_distance_cmp, target_key);

    log_msg(LOG_DEBUG, "After sort peers:");
    for (int i = 0; i < pending.size; i++) {
      struct Peer *p = vector_get(&pending, i);
      char buf[65] = {0};
      sha256_to_hex(p->peer_id, buf);

      log_msg(LOG_DEBUG, "pending[%d]->peer_id = %s", i, buf);
    }

    // Fill out_peers with the K closest ones
    size_t count = (pending.size < max_peers) ? pending.size : max_peers;
    for (size_t i = 0; i < count; i++) {
      out_peers[i] = (struct Peer *)vector_get(&pending, i);
    }

    for (size_t i = count; i < pending.size; i++) {
      struct Peer *p = vector_get(&pending, i);
      free(p);
    }
  }

  // Caller becomes responsible for freeing the contents
  vector_free(&pending, false);
  vector_free(&contacted, true);

  return (find_value && !value_found) ? -1 : 0;
}

void handle_rpc_request(const struct pollfd *sock, char *contents,
                        size_t length) {
  size_t expected_size = 0;

  struct RPCMessageHeader *header = (struct RPCMessageHeader *)contents;

  switch (header->call_type) {
  case PING:
    expected_size = sizeof(struct RPCPing);
    break;
  case STORE:
    expected_size = sizeof(struct RPCStore);
    break;
  case FIND_NODE:
    expected_size = sizeof(struct RPCFind);
    break;
  case FIND_VALUE:
    expected_size = sizeof(struct RPCFind);
    break;
  case BROADCAST:
    expected_size = sizeof(struct RPCBroadcast);
    break;

  case PING_RESPONSE:
  case STORE_RESPONSE:
  case FIND_NODE_RESPONSE:
  case FIND_VALUE_RESPONSE:
    log_msg(LOG_WARN, "Received a response packet without prior communication");
    break;

  default:
    log_msg(LOG_ERROR, "Got invalid RPC request!");
    return;
  }

  // log_msg(LOG_DEBUG, "Claimed size is: %d - Expected size is: %ld",
  // header->packet_size, expected_size);

  if (header->packet_size != expected_size) {
    log_msg(LOG_ERROR,
            "Claimed size doesn't match expected, discarding packet!");
    return;
  }

  switch (header->call_type) {
  case PING:
    handle_ping(sock, (struct RPCPing *)contents);
    break;
  case STORE:
    handle_store(sock, (struct RPCStore *)contents);
    break;
  case FIND_NODE:
    handle_find_node(sock, (struct RPCFind *)contents);
    break;
  case FIND_VALUE:
    handle_find_value(sock, (struct RPCFind *)contents);
    break;
  case BROADCAST:
    handle_broadcast(sock, (struct RPCBroadcast *)contents);
    break;

  default:
    break;
  }
}

int handle_rpc_upload(struct FileMagnet *file) {
  log_msg(LOG_DEBUG, "Start handling RPC upload");

  if (!file) {
    log_msg(LOG_ERROR, "handle_rpc_upload got NULL file");
    return -1;
  }

  struct KeyValuePair kv = {.key = {0}, .num_values = 1};
  memcpy(kv.key, file->file_hash, sizeof(HashID));
  // Create peer with our info in the KeyValuePair
  create_own_peer(&kv.values[0]);

  kv.values[0].peer_addr.sin_port = htons(SERVER_PORT);
  storage_put_value(&kv);

  struct Peer *out_peers[K_VALUE] = {0};
  int found = iterative_find_peers(file->file_hash, out_peers, K_VALUE, false);
  if (found != 0) {
    log_msg(LOG_WARN,
            "handle_rpc_upload: no peers available for STORE propagation");
    return -1;
  }

  char full_path[512] = {0};
  snprintf(full_path, sizeof(full_path), "%s/%s", UPLOAD_DIR,
           file->display_name);

  FILE *file_handle = fopen(full_path, "r");
  if (file_handle == NULL) {
    log_msg(LOG_ERROR,
            "File does not exist at path '%s'! The uploaded file should have "
            "been copied there beforehand.\n",
            full_path);
    return -1;
  }

  // Get size, allocate memory and read contents into the buffer
  fseek(file_handle, 0, SEEK_END);
  long file_size = ftell(file_handle);
  fseek(file_handle, 0, SEEK_SET);

  void *file_contents = malloc(file_size);

  fread(file_contents, file_size, 1, file_handle);
  fclose(file_handle);

  // Prepare the STORE request
  struct RPCStore store_req = {
      .header = {.magic_number = RPC_MAGIC,
                 .packet_size = sizeof(struct RPCStore),
                 .call_type = STORE}};

  // Replicate at most K - 1 times since we already filled a slot with our info
  for (int i = 0; i < K_VALUE - 1; i++) {
    if (out_peers[i] == NULL)
      continue;

    log_msg(LOG_DEBUG, "Replicating file to closest peer %d with port %d", i,
            ntohs(out_peers[i]->peer_addr.sin_port));

    // We also do replication on these nodes
    int res = upload_http_file(out_peers[i], file, file_contents, file_size);

    if (res != 0) {
      log_msg(LOG_ERROR, "Couldn't replicate file on remote peer");
      continue;
    }

    // We successfully replicated to this peer, add them to the key-value pair
    memcpy(&kv.values[kv.num_values], out_peers[i], sizeof(struct Peer));
    kv.num_values++;
  }

  struct RPCKeyValue serialized_kv = {0};
  serialize_rpc_value(&kv, &serialized_kv);

  memcpy(&store_req.key_value, &serialized_kv, sizeof(struct RPCKeyValue));

  for (int i = 0; i < K_VALUE; i++) {
    if (out_peers[i] == NULL) {
      log_msg(LOG_DEBUG, "Skipping NULL peer");
      continue;
    }

    log_msg(LOG_DEBUG, "Sending store to closest peer %d with port %d", i,
            ntohs(out_peers[i]->peer_addr.sin_port));

    int sock = connect_to_peer(&out_peers[i]->peer_addr);
    if (sock < 0) {
      log_msg(LOG_DEBUG, "Skipping because no connection");
      continue;
    }

    send_all(sock, &store_req, sizeof(store_req));

    close(sock);
  }

  log_msg(LOG_DEBUG,
          "handle_rpc_upload finished propagating file key to peers");

  free_peer_array(out_peers, K_VALUE);
  free(file_contents);

  return 0;
}

int handle_rpc_download(struct FileMagnet *file) {
  log_msg(LOG_DEBUG, "Start handling RPC download");

  if (!file) {
    log_msg(LOG_ERROR, "handle_rpc_download got NULL file");
    return -1;
  }

  HashID own_id;

  if (get_own_id(own_id) != 0) {
    log_msg(LOG_ERROR, "handle_rpc_download get_own_id error");
    return -1;
  }

  // First check local storage for the key-value pair
  const struct KeyValuePair *local_kv = storage_get_value(file->file_hash);
  if (local_kv) {
    log_msg(LOG_DEBUG, "Key found locally, downloading from local peers");
    for (int i = 0; i < local_kv->num_values; i++) {
      if (compare_hashes(own_id, local_kv->values[i].peer_id) == 0) {
        log_msg(LOG_INFO, "We are already one of the peers owning this file, "
                          "no need to redownload");
        return 0;
      }
      if (download_http_file(&local_kv->values[i], file) == 0)
        return 0;
    }

    log_msg(LOG_WARN, "All known peers failed to serve the file");
    return -1;
  }

  struct Peer *out_peers[K_VALUE] = {0};
  int value_found =
      iterative_find_peers(file->file_hash, out_peers, K_VALUE, true);

  if (value_found < 0) {
    log_msg(LOG_WARN, "File not found on the network");
    return -1;
  } else {
    for (int i = 0; i < K_VALUE; i++) {
      log_msg(LOG_DEBUG, "Trying to download the file from peer %d in the KVP",
              i);
      if (out_peers[i] && download_http_file(out_peers[i], file) == 0) {
        free_peer_array(out_peers, K_VALUE);
        return 0;
      }
    }
  }

  log_msg(LOG_WARN,
          "None of the owning peers were able to provide us the file");

  free_peer_array(out_peers, K_VALUE);
  return -1;
}
