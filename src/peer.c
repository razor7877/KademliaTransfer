#include <memory.h>

#include "peer.h"
#include "rpc.h"

int serialize_rpc_peer(const struct Peer* peer, struct RPCPeer* serialized) {
    if (!peer || !serialized)
        return -1;
    
    memcpy(&serialized->peer_addr, &peer->peer_addr, sizeof(peer->peer_addr));
    memcpy(serialized->peer_id, peer->peer_id, sizeof(peer->peer_id));
    // Unused for now
    memset(serialized->peer_key, 0, sizeof(serialized->peer_key));

    return 0;
}

int deserialize_rpc_peer(const struct RPCPeer* peer, struct Peer* deserialized) {
    if (!peer || !deserialized)
        return -1;

    memcpy(&deserialized->peer_addr, &peer->peer_addr, sizeof(peer->peer_addr));
    memcpy(deserialized->peer_id, peer->peer_id, sizeof(peer->peer_id));
    // Unused for now
    deserialized->peer_pub_key = NULL;

    return 0;
}