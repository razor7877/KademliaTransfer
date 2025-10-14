#include <poll.h>
#include <memory.h>

#include <hash/hashmap.h>

#include "rpc.h"
#include "log.h"
#include "storage.h"
#include "bucket.h"
#include "magnet.h"
#include "network.h"

/**
 * @brief Kademlia neighbor buckets
 * 
 */
static Buckets buckets = {0};

static void handle_ping(struct pollfd* sock, struct RPCPing* data) {
    log_msg(LOG_DEBUG, "Handling RPC ping");

    struct RPCResponse response = {
        .header = {
            .magic_number = RPC_MAGIC,
            .call_type = PING_RESPONSE,
            .packet_size = sizeof(struct RPCResponse)
        },
        .success = true
    };

    send_all(sock->fd, &response, sizeof(response));
}

static void handle_store(struct pollfd* sock, struct RPCStore* data) {
    log_msg(LOG_DEBUG, "Handling RPC store");

    struct KeyValuePair kvp;
    deserialize_rpc_value(&data->key_value, &kvp);
    storage_put_value(&kvp);

    struct RPCResponse response = {
        .header = {
            .magic_number = RPC_MAGIC,
            .call_type = STORE_RESPONSE,
            .packet_size = sizeof(struct RPCResponse)
        },
        .success = true
    };

    send_all(sock->fd, &response, sizeof(response));
}

static void handle_find_node(struct pollfd* sock, struct RPCFind* data) {
    log_msg(LOG_DEBUG, "Handling RPC find node");

    struct RPCFindNodeResponse response = {
        .header = {
            .magic_number = RPC_MAGIC,
            .call_type = FIND_NODE_RESPONSE,
            .packet_size = sizeof(struct RPCFindNodeResponse)
        },
        .success = false,
        .found_key = false,
        .num_closest = 0,
        .closest = {0}
    };
    
    int count = BUCKET_SIZE;
    struct Peer** closest = find_closest_peers(buckets, data->key, count);

    if (closest == NULL) {
        log_msg(LOG_ERROR, "handle_find_node find_closest_peers returned NULL");
        // Send error response
        send_all(sock->fd, &response, sizeof(response));
        return;
    }

    send_all(sock->fd, &response, sizeof(response));

    free(closest);
}

static void handle_find_value(struct pollfd* sock, struct RPCFind* data) {
    log_msg(LOG_DEBUG, "Handling RPC find value");

}

static void handle_broadcast(struct pollfd* sock, struct RPCBroadcast* data) {
    struct Peer peer;
    // Get the peer object back
    deserialize_rpc_peer(&data->peer, &peer);
    // Update our buckets with the information from this (potentially new) peer
    update_bucket_peers(buckets, &peer);
}

void handle_rpc_request(struct pollfd* sock, char* contents, size_t length) {
    size_t expected_size = 0;

    struct RPCMessageHeader* header = (struct RPCMessageHeader*)contents;

	switch (header->call_type) {
		case PING: expected_size = sizeof(struct RPCPing); break;
		case STORE: expected_size = sizeof(struct RPCStore); break;
		case FIND_NODE: expected_size = sizeof(struct RPCFind); break;
		case FIND_VALUE: expected_size = sizeof(struct RPCFind); break;
        case BROADCAST: expected_size = sizeof(struct RPCBroadcast); break;

		default:
			log_msg(LOG_ERROR, "Got invalid RPC request!");
            return;
	}

	// log_msg(LOG_DEBUG, "Claimed size is: %d - Expected size is: %ld", header->packet_size, expected_size);

	if (header->packet_size != expected_size) {
		log_msg(LOG_ERROR, "Claimed size doesn't match expected, discarding packet!");
		return;
	}

    switch (header->call_type) {
		case PING: handle_ping(sock, (struct RPCPing*)contents); break;
		case STORE: handle_store(sock, (struct RPCStore*)contents); break;
		case FIND_NODE: handle_find_node(sock, (struct RPCFind*)contents); break;
		case FIND_VALUE: handle_find_value(sock, (struct RPCFind*)contents); break;
        case BROADCAST: handle_broadcast(sock, (struct RPCBroadcast*)contents); break;
	}
}

void handle_rpc_upload(struct FileMagnet* file) {
    log_msg(LOG_DEBUG, "Start handling RPC upload");

    if (!file) {
        log_msg(LOG_ERROR, "handle_rpc_upload got NULL file");
        return;
    }

    HashID file_key;
    memcpy(file_key, file->file_hash, sizeof(HashID));

    struct KeyValuePair kv = {
        .key = {0},
        .num_values = 1
    };
    memcpy(kv.key, file_key, sizeof(HashID));
    // Create peer with our info in the KeyValuePair
    create_own_peer(&kv.values[0]);

    storage_put_value(&kv);

    int k = K_VALUE;
    struct Peer** closest = find_closest_peers(buckets, file_key, k);

    if (!closest) {
        log_msg(LOG_WARN, "handle_rpc_upload: no peers available for STORE propagation");
        return;
    }

    struct RPCKeyValue serialized_kv;
    serialize_rpc_value(&kv, &serialized_kv);

    struct RPCStore store_req = {
            .header = {
                .magic_number = RPC_MAGIC,
                .packet_size = sizeof(struct RPCStore),
                .call_type = STORE
            }
        };
        
    memcpy(&store_req.key_value, &serialized_kv, sizeof(struct RPCKeyValue));

    for (int i = 0; i < k; i++) {
        if (closest[i] == NULL) {
            log_msg(LOG_DEBUG, "Skipping NULL peer");
            continue;
        }

        log_msg(LOG_DEBUG, "Sending store to closest peer %d", i);

        int sock = connect_to_peer(&closest[i]->peer_addr);
        if (sock < 0) {
            log_msg(LOG_DEBUG, "Skipping because no connection");
            continue;
        }

        send_all(sock, &store_req, sizeof(store_req));
        close(sock);
    }

    log_msg(LOG_DEBUG, "handle_rpc_upload finished propagating file key to peers");
}

void handle_rpc_download(struct FileMagnet* file) {
    log_msg(LOG_DEBUG, "Start handling RPC download");

    if (!file) {
        log_msg(LOG_ERROR, "handle_rpc_download got NULL file");
        return;
    }
}
