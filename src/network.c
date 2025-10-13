#define _GNU_SOURCE

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>

#include "client.h"
#include "command.h"
#include "http.h"
#include "log.h"
#include "rpc.h"
#include "schedule.h"
#include "shared.h"
#include "network.h"

#define MAX_WAIT_CON 5
#define MAX_SOCK 128
#define SERVER_PORT 8182
#define BROADCAST_PORT 8183

static const char http_pattern[] = "\r\n\r\n";

static struct pollfd sock_array[MAX_SOCK] = {0};
static char buf[BUF_SIZE] = {0};
static int listen_fd = 0;
static int broad_fd = 0;
static void broadcast_discovery_request(void);
static struct Schedule tasks[] = {
    { "broadcast_discovery", 0, 1.0, broadcast_discovery_request },
    { NULL, 0, 0, NULL }
};

/**
 * @brief Gets the contents of the entire RPC request according to the request
 * type before passing it to the RPC layer
 *
 * @param sock The struct of the socket that sent the RPC request
 * @param buf A pointer to the buffer where data can be read into
 * @param buf_index Current index into the buffer
 */
static void get_rpc_request(struct pollfd* sock, char* buf) {
    // log_msg(LOG_INFO, "Handling P2P request");

    int sock_type = 0;
    socklen_t optlen = sizeof(sock_type);
    if (getsockopt(sock->fd, SOL_SOCKET, SO_TYPE, &sock_type, &optlen) < 0) {
        perror("getsockopt");
        return;
    }

    ssize_t received = 0;

    if (sock_type == SOCK_STREAM) {
        // TCP: read header first
        received = recv_all(sock->fd, buf, sizeof(struct RPCMessageHeader));
        if (received < sizeof(struct RPCMessageHeader)) {
            log_msg(LOG_ERROR, "Failed to read RPC header");
            return;
        }
    }
    else if (sock_type == SOCK_DGRAM) {
        // UDP: single recvfrom will give entire datagram
        struct sockaddr_in from_addr = {0};
        socklen_t from_len = sizeof(from_addr);
        received = recvfrom(sock->fd, buf, MAX_RPC_PACKET_SIZE, 0,
                            (struct sockaddr*)&from_addr, &from_len);
        if (received <= 0) {
            perror("recvfrom");
            return;
        }
    }
    else {
        log_msg(LOG_ERROR, "Unknown socket type");
        return;
    }

    // Interpret header
    struct RPCMessageHeader* header = (struct RPCMessageHeader*)buf;
    // log_msg(LOG_INFO, "Packet size is: %d", header->packet_size);

    if (header->packet_size > MAX_RPC_PACKET_SIZE) {
        log_msg(LOG_ERROR, "Packet too large! Discarding.");
        return;
    }

    if (sock_type == SOCK_STREAM) {
        // TCP: read rest of packet
        received = recv_all(sock->fd, buf + sizeof(struct RPCMessageHeader),
                            header->packet_size - sizeof(struct RPCMessageHeader));
        if (received < 0) {
            log_msg(LOG_ERROR, "Error reading full RPC request");
            return;
        }
    }
    else {
        // UDP: we already got the full packet in one recvfrom
        if (received != header->packet_size) {
            log_msg(LOG_WARN, "UDP packet size mismatch: got %zd, expected %u", received, header->packet_size);
            return;
        }
    }

    // Pass packet to RPC layer
    handle_rpc_request(sock, buf, header->packet_size);
}

/**
 * @brief Gets the contents of the entire HTTP request before passing it to the
 * HTTP layer
 *
 * @param sock The struct of the socket that sent the HTTP request
 * @param buf A pointer to the buffer where data can be read into
 * @param buf_index Current index into the buffer
 */
static void get_http_request(struct pollfd* sock, char* buf) {
  log_msg(LOG_INFO, "Handling HTTP request");

  ssize_t received =
      recv_until(sock->fd, buf, BUF_SIZE, http_pattern, strlen(http_pattern));

  if (received < 0) {
    log_msg(LOG_ERROR,
            "Error while trying to read entire HTTP request. Skipping...");
    return;
  }

  handle_http_request(sock, buf, received);
}

/**
 * @brief Called in the network update loop. Sends a broadcast discovery request
 *
 */
static void broadcast_discovery_request(void) {
  struct sockaddr_in server_addr = {0};
  socklen_t size = sizeof(server_addr);
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(BROADCAST_PORT);
  server_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

  struct RPCBroadcast request = {
    .header = {
      .magic_number = RPC_MAGIC,
      .packet_size = sizeof(struct RPCBroadcast),
      .call_type = BROADCAST,
    },
    .peer = {0}
  };

  struct Peer peer;
  create_own_peer(&peer);

  peer.peer_addr.sin_port = htons(SERVER_PORT);

  struct RPCPeer* serialized_peer = serialize_rpc_peer(&peer);
  request.peer = *serialized_peer;
  
  // log_msg(LOG_WARN, "Sending broadcast with addr with port %d", ntohs(peer.peer_addr.sin_port));

  free(serialized_peer);

  // Broadcast a RPC ping packet to everyone with our info
  ssize_t sent = sendto(sock_array[1].fd, &request, sizeof(request), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

  if (sent < 0)
    perror("sendto");
}

/**
 * @brief Called in the network update loop. Accepts incoming connections
 *
 */
static void handle_incoming() {
  // Accept incoming connections
  if (sock_array[0].revents & POLLIN) {
    log_msg(LOG_INFO, "Accepting connection");
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
        log_msg(LOG_DEBUG, "Accepted connection with fd: %d", new_fd);
        log_msg(LOG_DEBUG, "Stored at index: %d", i);
      } else {
        perror("Not enough space in buffer to allocate new connection");
        close(sock_array[i].fd);
      }
    } else
      perror("Error while trying to accept new connection");

    sock_array[0].revents = 0;
  }
  
  if (sock_array[1].revents && POLLIN) {
    // log_msg(LOG_DEBUG, "Receiving data on broadcast port");

    struct sockaddr_in client_addr = {0};
    socklen_t size = sizeof(client_addr);

    // Peek first 4 bytes to check magic
    uint8_t peek_magic[4] = {0};
    ssize_t recvd = recvfrom(sock_array[1].fd, peek_magic, sizeof(peek_magic), MSG_PEEK, (struct sockaddr*)&client_addr, &size);
    
    char my_ip[INET_ADDRSTRLEN] = {0};
    struct sockaddr_in my_addr;
    
    if (get_primary_ip(my_ip, sizeof(my_ip), &my_addr) == 0) {
        if (client_addr.sin_addr.s_addr == my_addr.sin_addr.s_addr) {
            // This is our own broadcast, ignore
            // log_msg(LOG_DEBUG, "Ignoring our own broadcast");
            char discard[MAX_RPC_PACKET_SIZE];
            // Make sure to consume data from the internal buffer
            recvfrom(sock_array[1].fd, discard, sizeof(discard), 0, NULL, NULL);
            sock_array[1].revents = 0;
            return;
        }
    }

    if (recvd < 4) {
        log_msg(LOG_WARN, "Incomplete UDP magic from %s:%d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        sock_array[1].revents = 0;
        return;
    }

    if (memcmp(peek_magic, RPC_MAGIC, 4) != 0) {
        log_msg(LOG_WARN, "Invalid RPC magic from %s:%d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        // Consume/discard
        char discard[1024];
        recvfrom(sock_array[1].fd, discard, sizeof(discard), 0, NULL, NULL);
        sock_array[1].revents = 0;
        return;
    }

    // Use get_rpc_request() to read the full packet into buf and process it
    struct pollfd udp_sock = sock_array[1];
    get_rpc_request(&udp_sock, buf);

    sock_array[1].revents = 0;
  }
}

/**
 * @brief Called in the network update loop. Handles requests from connected
 * peers
 *
 */
static void handle_connected() {
  // Handle existing connections
  for (int i = 2; i < MAX_SOCK; i++) {
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
          log_msg(LOG_DEBUG, "Connection closed on fd %d", sock_array[i].fd);

        close(sock_array[i].fd);
        sock_array[i].fd = -1;
        sock_array[i].events = 0;
        sock_array[i].revents = 0;
        continue;
      }

      // Dispatch depending on magic number
      if (peeked == 4 && memcmp(peek_buf, RPC_MAGIC, 4) == 0)
        get_rpc_request(&sock_array[i], buf);
      else
        get_http_request(&sock_array[i], buf);

      sock_array[i].revents = 0;
    }
  }
}

static void handle_pending() {
  struct Command cmd = {0};

  bool had_commands = commands.count > 0;

  while (commands.count > 0) {
    queue_pop(&commands, &cmd);

    log_msg(LOG_DEBUG, "Handling command from P2P client");

    switch (cmd.cmd_type) {
      case CMD_SHOW_STATUS:
        log_msg(LOG_DEBUG, "Show status");
        break;

      case CMD_UPLOAD:
        if (cmd.file == NULL) {
          log_msg(LOG_WARN, "Got upload command with empty file");
          continue;
        }

        handle_rpc_upload(cmd.file);

        break;

      case CMD_DOWNLOAD:
        if (cmd.file == NULL) {
          log_msg(LOG_WARN, "Got download command with empty file");
          continue;
        }

        handle_rpc_download(cmd.file);

        break;

      default:
        log_msg(LOG_DEBUG, "Unknown command");
        break;
    }
  }

  if (had_commands)
  log_msg(LOG_DEBUG, "Finished handling commands");
}

/**
 * @brief Called in the network update loop. Handles scheduled tasks
 * peers
 *
 */
static void handle_tasks() {
  time_t now = time(NULL);
  for (int i = 0; tasks[i].func != NULL; i++) {
    if (now >= tasks[i].next_run) {
      tasks[i].func();
      tasks[i].next_run = now + tasks[i].interval_secs;
    }
  }
}

void init_network() {
    log_msg(LOG_DEBUG, "Initializing network stack");

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    die(listen_fd, "socket");

    int reuse = 1;
    int ret = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    die(ret, "setsockopt(SO_REUSEADDR) failed");

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    ret = bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    die(ret, "bind");

    ret = listen(listen_fd, MAX_WAIT_CON);
    die(ret, "listen");

    log_msg(LOG_DEBUG, "Server is listening on port %d...", SERVER_PORT);

    // Initialize listen socket
    sock_array[0].fd = listen_fd;
    sock_array[0].events = POLLIN;
    sock_array[0].revents = 0;

    // Broadcast server init
    broad_fd = socket(AF_INET, SOCK_DGRAM, 0);
    die(broad_fd, "broadcast socket");

    struct sockaddr_in broadcast_server_addr;
    broadcast_server_addr.sin_family = AF_INET;
    broadcast_server_addr.sin_port = htons(BROADCAST_PORT);
    broadcast_server_addr.sin_addr.s_addr = INADDR_ANY;
    int broad_ret = setsockopt(broad_fd, SOL_SOCKET, SO_BROADCAST, &reuse, sizeof(reuse));
    die(broad_ret, "setsockopt(SO_BROADCAST) failed");

    broad_ret = bind(broad_fd, (struct sockaddr*)&broadcast_server_addr, sizeof(broadcast_server_addr));
    die(broad_ret, "bind broadcast listen");

    log_msg(LOG_DEBUG, "Server Broadcast is listening on port %d...", BROADCAST_PORT);

    // Initialize listen socket
    sock_array[1].fd = broad_fd;
    sock_array[1].events = POLLIN;
    sock_array[1].revents = 0;

    for (int i = 2; i < MAX_SOCK; i++)
        sock_array[i].fd = -1;
}

void update_network() {
  int active_fd = poll(sock_array, MAX_SOCK, 50);

  // Handle any pending commands from the frontend
  handle_pending();

  // Accept any new connections
  handle_incoming();

  // Handle requests from connected peers
  handle_connected();

  // Check periodic task
  handle_tasks();
}

void stop_network() {
    log_msg(LOG_INFO, "Stopping network stack");
}

int connect_to_peer(const struct sockaddr_in* addr) {
    if (!addr) {
        log_msg(LOG_ERROR, "connect_to_peer: NULL address");
        return -1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        log_msg(LOG_ERROR, "connect_to_peer: socket() failed: %s", strerror(errno));
        return -1;
    }

    // Set timeouts so we dont entirely block the client if the peer doesn't respond
    struct timeval timeout = {
        .tv_sec = 3,
        .tv_usec = 0
    };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    char ip_str[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &addr->sin_addr, ip_str, sizeof(ip_str));

    log_msg(LOG_DEBUG, "Connecting to peer %s:%d", ip_str, ntohs(addr->sin_port));

    if (connect(sock, (const struct sockaddr*)addr, sizeof(*addr)) < 0) {
        log_msg(LOG_WARN, "connect_to_peer: connect() failed to %s:%d (%s)",
                ip_str, ntohs(addr->sin_port), strerror(errno));
        close(sock);
        return -1;
    }

    log_msg(LOG_DEBUG, "Connected successfully to %s:%d", ip_str, ntohs(addr->sin_port));
    return sock;
}
