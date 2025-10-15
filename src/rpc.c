#include "rpc.h"

#include <hash/hashmap.h>
#include <memory.h>
#include <poll.h>

#include "bucket.h"
#include "http.h"
#include "log.h"
#include "magnet.h"
#include "network.h"
#include "storage.h"
#include "vector.h"

/**
 * @brief Kademlia neighbor buckets
 *
 */
static Buckets buckets = {0};

static void handle_update_peer(struct RPCPeer *serialize_peer)
{
  struct Peer peer;
  // Get the peer object back
  deserialize_rpc_peer(serialize_peer, &peer);
  // Update our buckets with the information from this (potentially new) peer
  update_bucket_peers(buckets, &peer);
}

static void handle_ping(struct pollfd *sock, struct RPCPing *data)
{
  log_msg(LOG_DEBUG, "Handling RPC ping");
  handle_update_peer(&data->header.peer);

  struct RPCResponse response = {
      .header = {.magic_number = RPC_MAGIC,
                 .call_type = PING_RESPONSE,
                 .packet_size = sizeof(struct RPCResponse),
                 .peer = {0}},
      .success = true};

  struct Peer own;
  if (create_own_peer(&own))
    response.success = false;
  else
  {
    struct RPCPeer serialized_own;
    serialize_rpc_peer(&own, &serialized_own);
    response.header.peer = serialized_own;
  }

  send_all(sock->fd, &response, sizeof(response));
}

static void handle_store(struct pollfd *sock, struct RPCStore *data)
{
  log_msg(LOG_DEBUG, "Handling RPC store");

  struct KeyValuePair kvp;
  deserialize_rpc_value(&data->key_value, &kvp);
  storage_put_value(&kvp);
  handle_update_peer(&data->header.peer);

  struct RPCResponse response = {
      .header = {.magic_number = RPC_MAGIC,
                 .call_type = STORE_RESPONSE,
                 .packet_size = sizeof(struct RPCResponse),
                 .peer = {0}},
      .success = true};

  struct Peer own;
  if (create_own_peer(&own))
    response.success = false;
  else
  {
    struct RPCPeer serialized_own;
    serialize_rpc_peer(&own, &serialized_own);
    response.header.peer = serialized_own;
  }

  send_all(sock->fd, &response, sizeof(response));
}

static void handle_find_node(struct pollfd *sock, struct RPCFind *data)
{
  log_msg(LOG_DEBUG, "Handling RPC find node");
  handle_update_peer(&data->header.peer);

  struct RPCFindNodeResponse response = {
      .header = {.magic_number = RPC_MAGIC,
                 .call_type = FIND_NODE_RESPONSE,
                 .packet_size = sizeof(struct RPCFindNodeResponse),
                 .peer = {0}},
      .success = true,
      .found_key = false,
      .num_closest = 0,
      .closest = {{{0}}}};

  struct Peer own;
  if (create_own_peer(&own))
    response.success = false;
  else
  {
    struct RPCPeer serialized_own;
    serialize_rpc_peer(&own, &serialized_own);
    response.header.peer = serialized_own;
  }

  int count = BUCKET_SIZE;
  struct Peer **closest = find_closest_peers(buckets, data->key, count);

  if (closest == NULL)
  {
    log_msg(LOG_ERROR, "handle_find_node find_closest_peers returned NULL");
    // Send error response
    send_all(sock->fd, &response, sizeof(response));
    return;
  }

  // Ugly, need to just return number found from find_closest_peers
  int found = 0;
  for (int i = 0; i < count; i++)
  {
    if (closest[i] == NULL)
      continue;

    found = i;
    serialize_rpc_peer(closest[i], &response.closest[i]);
  }

  response.num_closest = found;
  send_all(sock->fd, &response, sizeof(response));

  free(closest);
}

static void handle_find_value(struct pollfd *sock, struct RPCFind *data)
{
  log_msg(LOG_DEBUG, "Handling RPC find value");
  handle_update_peer(&data->header.peer);

  struct RPCFindValueResponse response = {
      .header = {.magic_number = RPC_MAGIC,
                 .call_type = FIND_VALUE_RESPONSE,
                 .packet_size = sizeof(struct RPCFindValueResponse),
                 .peer = {0}},
      .success = true,
      .found_key = false,
      .values = {{0}},
      .num_closest = 0,
      .closest = {{{0}}}};

  struct Peer own;
  if (create_own_peer(&own))
    response.success = false;
  else
  {
    struct RPCPeer serialized_own;
    serialize_rpc_peer(&own, &serialized_own);
    response.header.peer = serialized_own;
  }

  const struct KeyValuePair *kvp = storage_get_value(data->key);

  if (kvp)
  {
    log_msg(
        LOG_DEBUG,
        "We had the key value pair, returning value from our storage to peer");
    response.found_key = true;
    serialize_rpc_value(kvp, &response.values);

    send_all(sock->fd, &response, sizeof(response));

    return;
  }

  log_msg(LOG_DEBUG,
          "We don't have the key value pair, returning our neighbors closest "
          "to target");

  int count = BUCKET_SIZE;
  struct Peer **closest = find_closest_peers(buckets, data->key, count);

  if (closest == NULL)
  {
    log_msg(LOG_ERROR, "handle_find_node find_closest_peers returned NULL");
    // Send error response
    send_all(sock->fd, &response, sizeof(response));
    return;
  }

  // Ugly, need to just return number found from find_closest_peers
  int found = 0;
  for (int i = 0; i < count; i++)
  {
    if (closest[i] == NULL)
      continue;

    found = i;
    serialize_rpc_peer(closest[i], &response.closest[i]);
  }

  response.num_closest = found;

  send_all(sock->fd, &response, sizeof(response));

  free(closest);
}

static void handle_broadcast(struct pollfd *sock, struct RPCBroadcast *data)
{
  handle_update_peer(&data->header.peer);
}

void handle_rpc_request(struct pollfd *sock, char *contents, size_t length)
{
  size_t expected_size = 0;

  struct RPCMessageHeader *header = (struct RPCMessageHeader *)contents;

  switch (header->call_type)
  {
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
    log_msg(LOG_WARN,
            "Received a response packet without prior communication");
    break;

  default:
    log_msg(LOG_ERROR, "Got invalid RPC request!");
    return;
  }

  // log_msg(LOG_DEBUG, "Claimed size is: %d - Expected size is: %ld",
  // header->packet_size, expected_size);

  if (header->packet_size != expected_size)
  {
    log_msg(LOG_ERROR,
            "Claimed size doesn't match expected, discarding packet!");
    return;
  }

  switch (header->call_type)
  {
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

int handle_rpc_upload(struct FileMagnet *file)
{
  log_msg(LOG_DEBUG, "Start handling RPC upload");

  if (!file)
  {
    log_msg(LOG_ERROR, "handle_rpc_upload got NULL file");
    return -1;
  }

  struct KeyValuePair kv = {.key = {0}, .num_values = 1};
  memcpy(kv.key, file->file_hash, sizeof(HashID));
  // Create peer with our info in the KeyValuePair
  create_own_peer(&kv.values[0]);

  kv.values[0].peer_addr.sin_port = htons(SERVER_PORT);
  storage_put_value(&kv);

  int k = K_VALUE;
  struct Peer **closest = find_closest_peers(buckets, file->file_hash, k);

  if (!closest)
  {
    log_msg(LOG_WARN,
            "handle_rpc_upload: no peers available for STORE propagation");
    return -1;
  }

  struct RPCKeyValue serialized_kv = {0};
  serialize_rpc_value(&kv, &serialized_kv);

  struct RPCStore store_req = {
      .header = {.magic_number = RPC_MAGIC,
                 .packet_size = sizeof(struct RPCStore),
                 .call_type = STORE,
                 .peer = {0}}};
  struct Peer own;
  if (create_own_peer(&own))
  {
    struct RPCPeer serialized_own;
    serialize_rpc_peer(&own, &serialized_own);
    store_req.header.peer = serialized_own;
  }
  memcpy(&store_req.key_value, &serialized_kv, sizeof(struct RPCKeyValue));

  for (int i = 0; i < k; i++)
  {
    if (closest[i] == NULL)
    {
      log_msg(LOG_DEBUG, "Skipping NULL peer");
      continue;
    }

    log_msg(LOG_DEBUG, "Peer addr is %p", closest[i]);
    log_msg(LOG_DEBUG, "Sending store to closest peer %d with port %d", i,
            ntohs(closest[i]->peer_addr.sin_port));

    int sock = connect_to_peer(&closest[i]->peer_addr);
    if (sock < 0)
    {
      log_msg(LOG_DEBUG, "Skipping because no connection");
      continue;
    }

    send_all(sock, &store_req, sizeof(store_req));
    close(sock);
  }

  log_msg(LOG_DEBUG,
          "handle_rpc_upload finished propagating file key to peers");

  free(closest);

  return 0;
}

int handle_rpc_download(struct FileMagnet *file)
{
  log_msg(LOG_DEBUG, "Start handling RPC download");

  if (!file)
  {
    log_msg(LOG_ERROR, "handle_rpc_download got NULL file");
    return -1;
  }

  // First check local storage for the key-value pair
  const struct KeyValuePair *local_kv = storage_get_value(file->file_hash);
  if (local_kv)
  {
    log_msg(LOG_DEBUG, "Key found locally, downloading from local peers");
    for (int i = 0; i < local_kv->num_values; i++)
    {
      if (download_http_file(&local_kv->values[i], file) == 0)
        return 0;
    }

    log_msg(LOG_WARN, "All known peers failed to serve the file");
    return -1;
  }

  // Vector to store the current closest nodes we're contacting
  VectorPtr closest;
  vector_init(&closest);

  // Vector to store for each of the closest nodes whether we already contacted
  // them or not
  VectorPtr contacted;
  vector_init(&contacted);

  // Fill initial closest peers with our already known pears
  struct Peer **initial = find_closest_peers(buckets, file->file_hash, K_VALUE);
  if (initial)
  {
    for (int i = 0; i < K_VALUE && initial[i]; i++)
    {
      vector_push(&closest, initial[i]);
      bool *c = calloc(1, sizeof(bool));
      vector_push(&contacted, c);
    }
    free(initial);
  }

  bool value_found = false;

  // Iterative lookup loop to traverse the network
  while (!value_found)
  {
    // Whether at least one new peer was contacted during this loop, if not we
    // can assume we got as close as possible to our target but no one has it
    bool any_new_contact = false;

    for (size_t i = 0; i < closest.size; i++)
    {
      bool *c = (bool *)vector_get(&contacted, i);
      struct Peer *p = (struct Peer *)vector_get(&closest, i);

      if (!p || *c)
        continue;

      *c = true;
      any_new_contact = true;

      // Contact this peer to get their closest known peers to our target
      int sock = connect_to_peer(&p->peer_addr);
      struct pollfd pollfd_sock = {.fd = sock};
      if (sock < 0)
        continue;

      struct RPCFind find_req = {
          .header = {.magic_number = RPC_MAGIC,
                     .call_type = FIND_VALUE,
                     .packet_size = sizeof(struct RPCFind),
                     .peer = {0}}};
      struct Peer own;
      if (create_own_peer(&own))
      {
        struct RPCPeer serialized_own;
        serialize_rpc_peer(&own, &serialized_own);
        find_req.header.peer = serialized_own;
      }

      memcpy(find_req.key, file->file_hash, sizeof(HashID));
      log_msg(LOG_DEBUG, "Sending RPC FIND_VALUE");
      send_all(sock, &find_req, sizeof(find_req));

      char buf[MAX_RPC_PACKET_SIZE];
      size_t packet_size = 0;

      char peek_buf[4] = {0};
      ssize_t peeked = recv_all_peek(sock, peek_buf, sizeof(peek_buf));

      if (peeked <= 0)
      {
        if (peeked < 0)
          perror("recv_all_peek");

        close(sock);
        continue;
      }

      if (peeked != 4 || memcmp(peek_buf, RPC_MAGIC, 4) != 0)
      {
        log_msg(LOG_ERROR, "Incorrect RPC_MAGIC in FIND_VALUE answer");
        close(sock);
        continue;
      }

      // Send them the prepared FIND_VALUE RPC
      if (get_rpc_request(&pollfd_sock, buf, &packet_size) == 0)
      {
        log_msg(LOG_DEBUG, "Got RPC FIND_VALUE response");
        struct RPCMessageHeader *header = (struct RPCMessageHeader *)buf;

        // Validate response type
        if (header->call_type == FIND_VALUE_RESPONSE)
        {
          struct RPCFindValueResponse *response =
              (struct RPCFindValueResponse *)buf;

          // They have the key, get it and contact the owners of the file
          if (response->found_key)
          {
            log_msg(LOG_DEBUG, "Contacted peer knows the key");

            struct KeyValuePair kvp;
            deserialize_rpc_value(&response->values, &kvp);
            for (int j = 0; j < kvp.num_values; j++)
            {
              log_msg(LOG_DEBUG,
                      "Trying to download the file from peer %d in the KVP", j);
              if (download_http_file(&kvp.values[j], file) == 0)
              {
                value_found = true;
                break;
              }
            }
            if (value_found)
              break;
          }
          else
          {
            // They didn't have the key, get their closest peers and add to our
            // list
            for (int j = 0; j < response->num_closest; j++)
            {
              bool already_present = false;

              // Don't add a peer that we already have in our list
              for (size_t s = 0; s < closest.size; s++)
              {
                struct Peer *q = (struct Peer *)vector_get(&closest, s);
                if (q && memcmp(&q->peer_id, &response->closest[j].peer_id,
                                sizeof(HashID)) == 0)
                {
                  already_present = true;
                  break;
                }
              }

              if (already_present)
                continue;

              struct Peer *new_peer = malloc(sizeof(struct Peer));
              deserialize_rpc_peer(&response->closest[j], new_peer);
              vector_push(&closest, new_peer);

              bool *new_c = calloc(1, sizeof(bool));
              vector_push(&contacted, new_c);
            }
          }
        }
      }

      close(sock);
      if (value_found)
        break;
    }

    if (!any_new_contact)
      break;
  }

  // Cleanup
  vector_free(&closest, true);
  vector_free(&contacted, true);

  if (value_found)
    return 0;

  return -1;
}

void handle_rpc_refresh_bucket()
{
  // todo
}