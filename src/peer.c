#include <memory.h>

#include "peer.h"

struct RPCPeer* serialize_rpc_peer(const struct Peer* peer) {
    struct RPCPeer* serialized = malloc(sizeof(struct RPCPeer));
    
    memcpy(&serialized->peer_addr, &peer->peer_addr, sizeof(peer->peer_addr));
    memcpy(serialized->peer_id, peer->peer_id, sizeof(peer->peer_id));
    // Unused for now
    memset(serialized->peer_key, 0, sizeof(serialized->peer_key));

    return serialized;
}

struct Peer* deserialize_rpc_peer(const struct RPCPeer* peer) {
    struct Peer* deserialized = malloc(sizeof(struct Peer));

    memcpy(&deserialized->peer_addr, &peer->peer_addr, sizeof(peer->peer_addr));
    memcpy(deserialized->peer_id, peer->peer_id, sizeof(peer->peer_id));
    // Unused for now
    deserialized->peer_pub_key = NULL;

    return deserialized;
}