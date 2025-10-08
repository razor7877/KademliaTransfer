#define _GNU_SOURCE
#include "network.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "client.h"
#include "command.h"
#include "http.h"
#include "log.h"
#include "rpc.h"
#include "schedule.h"
#include "shared.h"

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
    {"broadcast_discovery", 0, 30, broadcast_discovery_request},
    {NULL, 0, 0, NULL}};

/**
 * @brief Gets the contents of the entire RPC request according to the request
 * type before passing it to the RPC layer
 *
 * @param sock The struct of the socket that sent the RPC request
 * @param buf A pointer to the buffer where data can be read into
 * @param buf_index Current index into the buffer
 */
static void get_rpc_request(struct pollfd* sock, char* buf) {
  log_msg(LOG_INFO, "Handling P2P request");

  // We already read the 4 bytes magic, read the rest of the header
  int received = recv_all(sock->fd, buf, sizeof(struct RPCMessageHeader));

  // Interpret the data as RPC header
  struct RPCMessageHeader* header = (struct RPCMessageHeader*)buf;

  log_msg(LOG_INFO, "Packet size is: %d", header->packet_size);

  if (header->packet_size > MAX_RPC_PACKET_SIZE) {
    log_msg(LOG_ERROR, "Packet too large! Discarding.");
    return;
  }

  // Get the rest of the packet
  received = recv_all(sock->fd, buf + sizeof(struct RPCMessageHeader),
                      header->packet_size - sizeof(struct RPCMessageHeader));

  if (received < 0) {
    log_msg(LOG_ERROR,
            "Error while trying to read entire RPC request. Skipping...");
    return;
  }

  // Pass the RPC packet to the RPC layer
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

static void broadcast_discovery_request(void) {
  log_msg(LOG_INFO, "Running new node discovery");
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
  } else if (sock_array[1].revents && POLLIN) {
    struct sockaddr_in client_addr = {0};
    socklen_t size = sizeof(client_addr);

    char peek_buf[4] = {0};
    ssize_t peeked = recvfrom(sock_array[1].fd, peek_buf, sizeof(peek_buf), 0,
                              (struct sockaddr*)&client_addr, &size);
    if (peeked < 0)
      perror("recvfrom broadcast peek");
    else {
      log_msg(LOG_DEBUG, "UDP broadcast from %s:%d - %s",
              inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port),
              peek_buf);
    }
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
      ssize_t peeked =
          recv_all_peek(sock_array[i].fd, peek_buf, sizeof(peek_buf));

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

        log_msg(LOG_DEBUG, "Upload file");
        break;

      case CMD_DOWNLOAD:
        if (cmd.file == NULL) {
          log_msg(LOG_WARN, "Got download command with empty file");
          continue;
        }

        handle_rpc_download(cmd.file);

        log_msg(LOG_DEBUG, "Download file");
        break;

      default:
        log_msg(LOG_DEBUG, "Unknown command");
        break;
    }
  }

  if (had_commands) log_msg(LOG_DEBUG, "Finished handling commands");
}

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
  int ret =
      setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
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
  int broad_ret =
      setsockopt(broad_fd, SOL_SOCKET, SO_BROADCAST, &reuse, sizeof(reuse));
  die(broad_ret, "setsockopt(SO_BROADCAST) failed");

  broad_ret = bind(broad_fd, (struct sockaddr*)&broadcast_server_addr,
                   sizeof(broadcast_server_addr));
  die(broad_ret, "bind broadcast listen");

  log_msg(LOG_DEBUG, "Server Broadcast is listening on port %d...",
          BROADCAST_PORT);

  // Initialize listen socket
  sock_array[1].fd = broad_fd;
  sock_array[1].events = POLLIN;
  sock_array[1].revents = 0;

  for (int i = 2; i < MAX_SOCK; i++) sock_array[i].fd = -1;
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

void stop_network() { log_msg(LOG_INFO, "Stopping network stack"); }
