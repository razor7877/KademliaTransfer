#include "http.h"

#include <ctype.h>
#include <errno.h>
#include <poll.h>
#include <string.h>
#include <sys/stat.h>

#include "log.h"
#include "network.h"
#include "shared.h"

#define UPLOAD_DIR "./upload"

static const char *not_found = "HTTP/1.1 404 Not Found\r\n"
                               "Content-Type: text/plain\r\n"
                               "Content-Length: 13\r\n"
                               "\r\n"
                               "404 Not Found";

static const char *method_not_allowed = "HTTP/1.1 405 Method Not Allowed\r\n"
                                        "Content-Type: text/plain\r\n"
                                        "Content-Length: 23\r\n"
                                        "Allow: GET\r\n"
                                        "\r\n"
                                        "405 Method Not Allowed";

/**
 * @brief Sends a file back over HTTP
 *
 * @param sock The peer socket to which to send the file
 * @param filename The name of the file to send
 */
static void send_http_file(struct pollfd *sock, const char *filename) {
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

void handle_http_request(struct pollfd *sock, char *contents, size_t length) {
  log_msg(LOG_INFO, "Handling HTTP request\n");

  if (memcmp(contents, "GET ", 4) != 0) {
    send_all(sock->fd, method_not_allowed, strlen(method_not_allowed));
    return;
  }

  char path[256] = {0};
  sscanf(contents, "GET %255s", path);

  // Strip leading '/'
  char *file_path = path + 1;
  if (strlen(file_path) == 0) {
    send_all(sock->fd, not_found, strlen(not_found));
    return;
  }

  log_msg(LOG_INFO, "Request file path: %s\n", file_path);

  send_http_file(sock, file_path);
}

static char *strcasestr_portable(const char *haystack, const char *needle) {
  if (!*needle)
    return (char *)haystack;

  for (; *haystack; haystack++) {
    const char *h = haystack;
    const char *n = needle;
    while (*h && *n &&
           tolower((unsigned char)*h) == tolower((unsigned char)*n)) {
      h++;
      n++;
    }
    if (!*n)
      return (char *)haystack;
  }
  return NULL;
}

int download_http_file(const struct Peer *peer, struct FileMagnet *file) {
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

  int bytes_send = send_all(peer_fd, request, strlen(request));
  if (bytes_send < 0) {
    log_msg(LOG_ERROR, "Error in download http_file 0 bytes send!");
    close(peer_fd);
    return -1;
  }

  int header_bytes_read =
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

  sscanf(content_length_buf, "Content-Length: %zu", &content_length);
  log_msg(LOG_DEBUG, "Downloading HTTP file with size %zu", content_length);

  // Create the download folder if not already present
  const char *download_dir = "./download";
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
  ssize_t received;

  while ((received = recv(peer_fd, buffer, sizeof(buffer), 0)) > 0)
    fwrite(buffer, 1, received, new_file);

  if (received < 0)
    log_msg(LOG_ERROR, "Error while receiving file: %s", strerror(errno));

  fclose(new_file);
  close(peer_fd);

  return 0;
}
