#define _GNU_SOURCE
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>

#include "network.h"
#include "shared.h"
#include "http.h"
#include "rpc.h"

#define MAX_WAIT_CON 5
#define MAX_SOCK 128
#define SERVER_PORT 8182

static const char http_pattern[] = "\r\n\r\n";

static struct pollfd sock_array[MAX_SOCK] = {0};
static char buf[BUF_SIZE] = {0};
static int listen_fd = 0;

/**
 * @brief Gets the contents of the entire RPC request according to the request type before passing it to the RPC layer
 * 
 * @param sock The struct of the socket that sent the RPC request
 * @param buf A pointer to the buffer where data can be read into
 * @param buf_index Current index into the buffer
 */
static void get_rpc_request(struct pollfd* sock, char* buf) {
	printf("Handling P2P request\n");

	struct RPCMessageHeader header = {0};
	// We already read the 4 bytes magic, read the rest of the header
	int received = recv_all(sock->fd, &header, sizeof(header));

	printf("Packet size is: %d\n", header.packet_size);

	size_t expected_size = 0;

	switch (header.call_type) {
		case PING:
			printf("Got RPC ping\n");
			expected_size = sizeof(struct RPCPing);
			break;
		
		case STORE:
			printf("Got RPC store\n");
			expected_size = sizeof(struct RPCStore);
			break;

		case FIND_NODE:
			printf("Got RPC find node\n");
			expected_size = sizeof(struct RPCFind);
			break;

		case FIND_VALUE:
			printf("Got RPC find value\n");
			expected_size = sizeof(struct RPCFind);
			break;
		
		case PING_RESPONSE:
			printf("Got RPC ping response\n");
			expected_size = sizeof(struct RPCResponse);
			break;

		case STORE_RESPONSE:
			printf("Got RPC store response\n");
			expected_size = sizeof(struct RPCResponse);
			break;

		// Sizes here will actually depend on whether a key was found or not
		case FIND_NODE_RESPONSE:
			printf("Got RPC find node response\n");
			expected_size = sizeof(struct RPCFindNodeResponse);
			break;

		case FIND_VALUE_RESPONSE:
			printf("Got RPC find node response\n");
			expected_size = sizeof(struct RPCFindNodeResponse);
			break;

		default:
			printf("Got invalid RPC request!\n");
			break;
	}

	printf("Claimed size is: %d - Expected size is: %ld\n", header.packet_size, expected_size);

	if (header.packet_size != expected_size) {
		printf("Claimed size doesn't match expected, discarding packet!\n");
		return;
	}

	void* rpc_packet = malloc(expected_size);

	// Copy the already read header
	memcpy(rpc_packet, &header, sizeof(header));

	// Get the rest of the packet
	received = recv_all(sock->fd, rpc_packet + sizeof(header), expected_size - sizeof(header));

	if (received < 0) {
		printf("Error while trying to read entire RPC request. Skipping...");
		free(rpc_packet);
		return;
	}

	// Pass the RPC packet to the RPC layer
	handle_rpc_request(rpc_packet, expected_size);

	free(rpc_packet);
}

/**
 * @brief Gets the contents of the entire HTTP request before passing it to the HTTP layer
 * 
 * @param sock The struct of the socket that sent the HTTP request
 * @param buf A pointer to the buffer where data can be read into
 * @param buf_index Current index into the buffer
 */
static void get_http_request(struct pollfd* sock, char* buf) {
	printf("Handling HTTP request\n");
	
	ssize_t received = recv_until(sock->fd, buf, BUF_SIZE, http_pattern, strlen(http_pattern));

	if (received < 0) {
		printf("Error while trying to read entire HTTP request. Skipping...");
		return;
	}

	handle_http_request(sock, buf, received);
}

static void handle_incoming() {
	// Accept incoming connections
	if (sock_array[0].revents & POLLIN) {
		printf("Accepting connection\n");
		struct sockaddr_in client_addr = {0};
		socklen_t size = sizeof(client_addr);

		int new_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &size);
		if (new_fd >= 0) {
			int i = 0;
			while (sock_array[i].fd != -1) i++;

			if (i < MAX_SOCK) {
				sock_array[i].fd = new_fd;
				sock_array[i].events = POLLIN;
				sock_array[i].revents = 0;
				printf("Accepted connection with fd: %d\n", new_fd);
				printf("Stored at index: %d\n", i);
			}
			else {
				perror("Not enough space in buffer to allocate new connection");
				close(sock_array[i].fd);
			}
		}
		else
			perror("Error while trying to accept new connection");

		sock_array[0].revents = 0;
	}
}

static void handle_connected() {
	// Handle existing connections
	for (int i = 1; i < MAX_SOCK; i++) {
		// Close connection and free space
		if (sock_array[i].revents & POLLHUP) {
			close(sock_array[i].fd);
			sock_array[i].fd = -1;
			sock_array[i].events = 0;
			sock_array[i].revents = 0;
		}

		// Handle message from connection
		if (sock_array[i].revents & POLLIN) {
			char peek_buf[4] = {0};
			ssize_t peeked = recv_all_peek(sock_array[i].fd, peek_buf, sizeof(peek_buf));

			if (peeked <= 0) {
				if (peeked < 0)
					perror("recv_all_peek");
				else
					printf("Connection closed on fd %d\n", sock_array[i].fd);
				
				close(sock_array[i].fd);
				sock_array[i].fd = -1;
				sock_array[i].events = 0;
				sock_array[i].revents = 0;
				continue;
			}

			if (peeked == 4 && memcmp(peek_buf, "KDMT", 4) == 0)
				get_rpc_request(&sock_array[i], buf);
			else
				get_http_request(&sock_array[i], buf);

			sock_array[i].revents = 0;
		}
	}
}

void init_network() {
    printf("Initializing network stack\n");

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	die(listen_fd, "socket");

	int reuse = 1;
	int ret = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	die(ret, "setsockopt(SO_REUSEADDR) failed");

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	die(ret, "bind");

	ret = listen(listen_fd, MAX_WAIT_CON);
	die(ret, "listen");

	printf("Server is listening on port %d...\n", SERVER_PORT);

	// Initialize listen socket
	sock_array[0].fd = listen_fd;
	sock_array[0].events = POLLIN;
	sock_array[0].revents = 0;

	for (int i = 1; i < MAX_SOCK; i++)
		sock_array[i].fd = -1;
}

void update_network() {
    int active_fd = poll(sock_array, MAX_SOCK, 50);

    // Accept any new connections
    handle_incoming();

    // Handle requests from connected peers
    handle_connected();
}

void stop_network() {
    printf("Stopping network stack\n");
}
