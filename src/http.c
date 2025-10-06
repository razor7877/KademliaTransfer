#include <poll.h>
#include <string.h>

#include "http.h"
#include "shared.h"

static const char* not_found =
    "HTTP/1.1 404 Not Found\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 13\r\n"
    "\r\n"
    "404 Not Found";

static const char* method_not_allowed =
    "HTTP/1.1 405 Method Not Allowed\r\n"
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
static void send_http_file(struct pollfd* sock, const char* filename) {
    FILE* file = fopen(filename, "rb");

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
        "\r\n", size);

    send_all(sock->fd, header, strlen(header));

    char buffer[BUF_SIZE] = {0};
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send_all(sock->fd, buffer, bytes_read);
    }

    fclose(file);

}

void handle_http_request(struct pollfd* sock, char* contents, size_t length) {
    printf("Handling HTTP request\n");

    //printf("Contents:\n%s", contents);

    if (memcmp(contents, "GET ", 4) != 0) {
        send_all(sock->fd, method_not_allowed, strlen(method_not_allowed));
        return;
    }

    char path[256] = {0};
    sscanf(contents, "GET %255s", path);

    // Strip leading '/'
    char* file_path = path + 1;
    if (strlen(file_path) == 0) {
        send_all(sock->fd, not_found, strlen(not_found));
        return;
    }
    
    printf("Request file path: %s\n", file_path);

    send_http_file(sock, file_path);
}

int download_http_file(struct Peer* peer, HashID* file) {
    return -1;
}