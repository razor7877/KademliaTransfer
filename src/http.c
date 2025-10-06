#include <poll.h>
#include <string.h>

#include "http.h"
#include "shared.h"

void handle_http_request(struct pollfd* sock, char* contents, size_t length) {
    printf("Handling HTTP request\n");

    const char body[] =
		"<!DOCTYPE html>\r\n"
		"<html>\r\n"
		"<head><title>Hello</title></head>\r\n"
		"<body>Hello, world!</body>\r\n"
		"</html>\r\n";

	char header[256];
	snprintf(header, sizeof(header),
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html; charset=UTF-8\r\n"
		"Content-Length: %zu\r\n"
		"\r\n",
		strlen(body));

    // Ideally, it should be the network layer that sends the response back
	send_all(sock->fd, header, strlen(header));
	send_all(sock->fd, body, strlen(body));
}

int download_http_file(struct Peer* peer, HashID* file) {
    return -1;
}