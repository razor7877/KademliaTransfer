#include "http.h"

#include <ctype.h>
#include <errno.h>
#include <poll.h>
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <string.h>
#include <sys/stat.h>

#include "log.h"
#include "network.h"
#include "peer.h"
#include "shared.h"

static const char *not_found = "HTTP/1.1 404 Not Found\r\n"
                               "Content-Type: text/plain\r\n"
                               "Content-Length: 13\r\n"
                               "\r\n"
                               "404 Not Found";

static const char *method_not_allowed = "HTTP/1.1 405 Method Not Allowed\r\n"
                                        "Content-Type: text/plain\r\n"
                                        "Content-Length: 23\r\n"
                                        "Allow: GET, PUT\r\n"
                                        "\r\n"
                                        "405 Method Not Allowed";

static const char *file_created = "HTTP/1.1 201 Created\r\n"
                                  "Content-Type: text/plain\r\n"
                                  "Content-Length: 24\r\n"
                                  "Connection: close\r\n"
                                  "\r\n"
                                  "File uploaded successfully";

static const char *internal_server_error =
    "HTTP/1.1 500 Internal Server Error\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 21\r\n"
    "Connection: close\r\n"
    "\r\n"
    "Internal Server Error";

static const char *bad_request = "HTTP/1.1 400 Bad Request\r\n"
                                 "Content-Type: text/plain\r\n"
                                 "Content-Length: 12\r\n"
                                 "Connection: close\r\n"
                                 "\r\n"
                                 "Bad Request";

/**
 * @brief Sends a file back over HTTP
 *
 * @param sock The peer socket to which to send the file
 * @param filename The name of the file to send
 */
static void send_http_file(const struct pollfd *sock, const char *filename) {
  char full_path[512] = {0};
  snprintf(full_path, sizeof(full_path), "%s/%s", UPLOAD_DIR, filename);

  FILE *file = fopen(full_path, "rb");

  // If file not present on server
  if (file == NULL) {
    send_all(sock->fd, not_found, strlen(not_found));
    return;
  }

  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);

  char header[512] = {0};
  snprintf(header, sizeof(header),
           "HTTP/1.1 200 OK\r\n"
           "Content-Type: application/octet-stream\r\n"
           "Content-Length: %ld\r\n"
           "Connection: close\r\n"
           "\r\n",
           size);

  send_all(sock->fd, header, strlen(header));

  char buffer[BUF_SIZE] = {0};
  size_t bytes_read;
  while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
    send_all(sock->fd, buffer, bytes_read);
  }

  fclose(file);
}

static void receive_http_file(const struct pollfd *sock, const char *filename,
                              const char *headers, size_t header_len) {
  char full_path[512] = {0};
  snprintf(full_path, sizeof(full_path), "%s/%s", UPLOAD_DIR, filename);

  // Make sure upload directory exists
  struct stat st = {0};
  if (stat(UPLOAD_DIR, &st) == -1) {
    if (mkdir(UPLOAD_DIR, 0755) == -1) {
      log_msg(LOG_ERROR, "Failed to create upload directory: %s",
              strerror(errno));
      send_all(sock->fd, internal_server_error, strlen(internal_server_error));
      return;
    }
  }

  const char *cl_hdr = strcasestr_portable(headers, "Content-Length:");
  size_t content_length = 0;

  if (!cl_hdr) {
    log_msg(LOG_ERROR, "Missing Content-Length in PUT request");
    send_all(sock->fd, bad_request, strlen(bad_request));
    return;
  }

  if (sscanf(cl_hdr, "Content-Length: %zu", &content_length) != 1) {
    log_msg(LOG_ERROR, "Invalid Content-Length in PUT request");
    send_all(sock->fd, bad_request, strlen(bad_request));
    return;
  }

  log_msg(LOG_INFO, "Receiving file '%s' (%zu bytes)", filename,
          content_length);

  FILE *file = fopen(full_path, "wb");
  if (!file) {
    log_msg(LOG_ERROR, "Cannot create file %s: %s", full_path, strerror(errno));
    send_all(sock->fd, internal_server_error, strlen(internal_server_error));
    return;
  }

  const char *body_start = strstr(headers, "\r\n\r\n");
  if (body_start) {
    body_start += 4;
    size_t body_bytes_in_buf = 0;
    if (headers + header_len > body_start) {
      body_bytes_in_buf = (size_t)(headers + header_len - body_start);
    }
    if (body_bytes_in_buf > 0) {
      size_t to_write = body_bytes_in_buf < content_length ? body_bytes_in_buf
                                                           : content_length;
      if (fwrite(body_start, 1, to_write, file) != to_write) {
        log_msg(LOG_ERROR, "Error writing initial body bytes to file");
        fclose(file);
        send_all(sock->fd, internal_server_error,
                 strlen(internal_server_error));
        return;
      }
      content_length -= to_write;
    }
  } else {
    log_msg(LOG_WARN, "No header terminator found in headers buffer");
  }

  char buffer[CHUNK_SIZE];
  while (content_length > 0) {
    size_t to_recv =
        content_length < sizeof(buffer) ? content_length : sizeof(buffer);
    ssize_t n = recv_all(sock->fd, buffer, to_recv);
    if (n < 0) {
      if (errno == EINTR)
        continue;
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        continue;
      log_msg(LOG_ERROR, "recv error: %s", strerror(errno));
      send_all(sock->fd, internal_server_error, strlen(internal_server_error));
      fclose(file);
      return;
    }
    if (n == 0) {
      log_msg(LOG_WARN,
              "Connection closed while receiving file (remaining=%zu)",
              content_length);
      break;
    }
    if ((size_t)n > 0) {
      if (fwrite(buffer, 1, n, file) != (size_t)n) {
        log_msg(LOG_ERROR, "Error writing to file during upload");
        send_all(sock->fd, internal_server_error,
                 strlen(internal_server_error));
        fclose(file);
        return;
      }
      content_length -= (size_t)n;
    }
  }

  fclose(file);
  log_msg(LOG_INFO, "File '%s' uploaded successfully", full_path);

  // Send back success response
  send_all(sock->fd, file_created, strlen(file_created));
}

void handle_http_request(const struct pollfd *sock, const char *contents,
                         size_t length) {
  log_msg(LOG_INFO, "Handling HTTP request\n");

  char path[256] = {0};
  if (memcmp(contents, "GET ", 4) == 0) {
    sscanf(contents, "GET %255s", path);

    // Strip leading '/'
    char *file_path = path + 1;
    if (strlen(file_path) == 0) {
      send_all(sock->fd, not_found, strlen(not_found));
      return;
    }

    log_msg(LOG_INFO, "GET Request file path: %s\n", file_path);

    send_http_file(sock, file_path);
  } else if (memcmp(contents, "PUT ", 4) == 0) {
    sscanf(contents, "PUT %255s", path);

    // Strip leading '/'
    char *file_path = path + 1;
    if (strlen(file_path) == 0) {
      send_all(sock->fd, not_found, strlen(not_found));
      return;
    }

    log_msg(LOG_INFO, "PUT Request file path: %s\n", file_path);

    receive_http_file(sock, file_path, contents, length);
  } else {
    send_all(sock->fd, method_not_allowed, strlen(method_not_allowed));
  }
}

int download_http_file(const struct Peer *peer, const struct FileMagnet *file) {
  if (!peer) {
    log_msg(LOG_ERROR, "Error in download_http_file peer is null!");
    return -1;
  }

  char request[1024] = {0};
  char hash_buf[sizeof(HashID) * 2 + 1] = {0};

  sha256_to_hex(file->file_hash, hash_buf);
  char ip_str[INET_ADDRSTRLEN] = {0};
  inet_ntop(AF_INET, &(peer->peer_addr.sin_addr), ip_str, INET_ADDRSTRLEN);
  int port = ntohs(peer->peer_addr.sin_port);

  // Build the GET request
  snprintf(request, 1024,
           "GET /%s HTTP/1.1\r\n"
           "Host: %s:%d\r\n"
           "Connection: close\r\n\r\n",
           file->display_name, ip_str, port);

  int peer_fd = connect_to_peer(&peer->peer_addr);
  if (peer_fd == -1)
    return -1;

  char http_header[HTTP_HEADER_SIZE] = {0};

  ssize_t bytes_send = send_all(peer_fd, request, strlen(request));
  if (bytes_send < 0) {
    log_msg(LOG_ERROR, "Error in download http_file 0 bytes send!");
    close(peer_fd);
    return -1;
  }

  ssize_t header_bytes_read =
      recv_until(peer_fd, http_header, sizeof(http_header), "\r\n\r\n", 4);
  if (header_bytes_read <= 0) {
    log_msg(LOG_ERROR, "Error in download http_file response is empty");
    close(peer_fd);
    return -1;
  }

  if (strstr(http_header, "404 Not Found") != NULL) {
    log_msg(LOG_WARN, "Peer responded with 404 Not Found for file %s",
            file->display_name);
    close(peer_fd);
    return -1;
  }

  http_header[header_bytes_read] = '\0';
  char *content_length_buf =
      strcasestr_portable(http_header, "Content-Length:");
  size_t content_length = 0;
  if (!content_length_buf) {
    log_msg(LOG_ERROR, "Error in download_http_file content_length not found");
    close(peer_fd);
    return -1;
  }

  if (sscanf(content_length_buf, "Content-Length: %zu", &content_length) != 1) {
    log_msg(LOG_ERROR, "Error in download_http_file content length not found");
  }
  log_msg(LOG_DEBUG, "Downloading HTTP file with size %zu", content_length);

  // Create the download folder if not already present
  const char *download_dir = DOWNLOAD_DIR;
  if (access(download_dir, F_OK) == -1) {
    if (mkdir(download_dir, 0755) == -1) {
      log_msg(LOG_ERROR, "Error creating download directory %s: %s",
              download_dir, strerror(errno));
      close(peer_fd);
      return -1;
    }
  }

  // Create path to file in download folder
  char file_path[512] = {0};
  snprintf(file_path, sizeof(file_path), "%s/%s", download_dir,
           file->display_name);

  FILE *new_file = fopen(file_path, "wb");
  if (!new_file) {
    log_msg(LOG_ERROR,
            "Error in download_http_file cannot create the file!: %s",
            strerror(errno));
    close(peer_fd);
    return -1;
  }

  char buffer[CHUNK_SIZE];
  while (content_length > 0) {
    const size_t to_recv =
        content_length < sizeof(buffer) ? content_length : sizeof(buffer);
    const ssize_t n = recv_all(peer_fd, buffer, to_recv);

    if (n < 0) {
      if (errno == EINTR)
        continue;
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        continue;
      log_msg(LOG_ERROR, "recv error: %s", strerror(errno));
      fclose(new_file);
      close(peer_fd);
      return -1;
    }

    if (n == 0)
      break;

    fwrite(buffer, 1, n, new_file);
    content_length -= n;
  }

  fclose(new_file);
  close(peer_fd);
  log_msg(LOG_INFO, "Downloaded file saved to: %s", file_path);

  return 0;
}

int upload_http_file(const struct Peer *peer, const struct FileMagnet *file,
                     const char *contents, size_t length) {
  if (!peer || !contents || length == 0) {
    log_msg(LOG_ERROR, "upload_http_file: invalid arguments");
    return -1;
  }

  // Connect to the peer
  int sock = connect_to_peer(&peer->peer_addr);
  if (sock < 0) {
    log_msg(LOG_ERROR, "upload_http_file: cannot connect to peer");
    return -1;
  }

  // Build PUT request header
  char request_header[1024];
  snprintf(request_header, sizeof(request_header),
           "PUT /%s HTTP/1.1\r\n"
           "Host: %s:%d\r\n"
           "Content-Type: application/octet-stream\r\n"
           "Content-Length: %zu\r\n"
           "Connection: close\r\n\r\n",
           file->display_name, // use peer->display_name as filename
           inet_ntoa(peer->peer_addr.sin_addr), // peer IP
           ntohs(peer->peer_addr.sin_port), length);

  // Send header
  if (send_all(sock, request_header, strlen(request_header)) < 0) {
    log_msg(LOG_ERROR, "upload_http_file: failed to send header");
    close(sock);
    return -1;
  }

  // Send file contents
  if (send_all(sock, contents, length) < 0) {
    log_msg(LOG_ERROR, "upload_http_file: failed to send file data");
    close(sock);
    return -1;
  }

  char resp_header[HTTP_HEADER_SIZE];
  ssize_t header_bytes =
      recv_until(sock, resp_header, sizeof(resp_header), "\r\n\r\n", 4);

  if (header_bytes <= 0) {
    log_msg(LOG_ERROR, "upload_http_file: failed to read response headers");
    close(sock);
    return -1;
  }

  resp_header[header_bytes] = '\0';
  log_msg(LOG_DEBUG, "upload_http_file: response headers:\n%s", resp_header);

  int status_code = 0;
  if (sscanf(resp_header, "HTTP/%*d.%*d %d", &status_code) == 1) {
    if (status_code >= 200 && status_code < 300) {
      log_msg(LOG_INFO, "upload_http_file: success status %d", status_code);
    } else {
      log_msg(LOG_WARN, "upload_http_file: non-2xx response %d", status_code);
    }
  } else {
    log_msg(LOG_WARN, "upload_http_file: cannot parse response status");
  }

  char *cl = strcasestr_portable(resp_header, "Content-Length:");
  if (cl) {
    size_t resp_len = 0;
    if (sscanf(cl, "Content-Length: %zu", &resp_len) == 1) {
      size_t remaining = resp_len;
      char rbuf[CHUNK_SIZE];
      while (remaining > 0) {
        size_t to_recv = remaining < sizeof(rbuf) ? remaining : sizeof(rbuf);
        ssize_t rn = recv(sock, rbuf, to_recv, 0);
        if (rn <= 0)
          break;
        remaining -= rn;
      }
    }
  }

  close(sock);
  return 0;
}