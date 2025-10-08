#include <poll.h>

#include "rpc.h"
#include "log.h"

/**
 * @brief Kademlia neighbor buckets
 * 
 */
static Buckets buckets = {0};

static void handle_ping(struct pollfd* sock, struct RPCPing* data) {
    log_msg(LOG_DEBUG, "Handling RPC ping\n");

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
    log_msg(LOG_DEBUG, "Handling RPC store\n");
}

static void handle_find_node(struct pollfd* sock, struct RPCFind* data) {
    log_msg(LOG_DEBUG, "Handling RPC find node\n");
}

static void handle_find_value(struct pollfd* sock, struct RPCFind* data) {
    log_msg(LOG_DEBUG, "Handling RPC find value\n");
}

void handle_rpc_request(struct pollfd* sock, char* contents, size_t length) {
    log_msg(LOG_DEBUG, "Handling RPC request in RPC layer\n");

    size_t expected_size = 0;

    struct RPCMessageHeader* header = (struct RPCMessageHeader*)contents;

	switch (header->call_type) {
		case PING: expected_size = sizeof(struct RPCPing); break;
		case STORE: expected_size = sizeof(struct RPCStore); break;
		case FIND_NODE: expected_size = sizeof(struct RPCFind); break;
		case FIND_VALUE: expected_size = sizeof(struct RPCFind); break;

		default:
			log_msg(LOG_ERROR, "Got invalid RPC request!\n");
            return;
	}

	log_msg(LOG_DEBUG, "Claimed size is: %d - Expected size is: %ld\n", header->packet_size, expected_size);

	if (header->packet_size != expected_size) {
		log_msg(LOG_ERROR, "Claimed size doesn't match expected, discarding packet!\n");
		return;
	}

    switch (header->call_type) {
		case PING: handle_ping(sock, (struct RPCPing*)contents); break;
		case STORE: handle_store(sock, (struct RPCStore*)contents); break;
		case FIND_NODE: handle_find_node(sock, (struct RPCFind*)contents); break;
		case FIND_VALUE: handle_find_value(sock, (struct RPCFind*)contents);; break;
	}
}

void handle_rpc_upload(struct FileMagnet* file) {
    log_msg(LOG_DEBUG, "Start handling RPC upload");


}

void handle_rpc_download(struct FileMagnet* file) {
    log_msg(LOG_DEBUG, "Start handling RPC download");
}
