#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

void die(int val, char *str) {
	if (val < 0) {
		perror(str);
		exit(EXIT_FAILURE);
	}
}

void recv_all(int fd, void* dst, size_t len) {
	size_t nb_read = 0;
	int ret = 0;

	while (nb_read < len) {
		ret = read(fd, (char*)dst + nb_read, len - nb_read);
		die(ret, "Error on reading message");
		nb_read += ret;
	}
}

int recv_until(int fd, void* dst, size_t buf_len, char* pattern, size_t pattern_len) {
	size_t nb_read = 0;
	int ret = 0;
	bool done = false;

	while (!done && nb_read < buf_len) {
		printf("nb_read = %ld\n", nb_read);
		ret = read(fd, (char*)dst + nb_read, 1);
		die(ret, "Error on reading message");
		nb_read += ret;

		done = true;
		for (int i = 0; i < pattern_len && done; i++) {
			char p = pattern[i];
			char check = *(char*)(dst + nb_read + i - pattern_len);
			if (p != check) done = false;
		}
	}

	return nb_read;
}

void send_all(int fd, void* src, size_t len) {
	size_t nb_sent = 0;
	int ret = 0;

	while (nb_sent < len) {
		ret = write(fd, (char*)src + nb_sent, len - nb_sent);
		die(ret, "Error on sending message size");
		nb_sent += ret;
	}
}

int min(const int a, const int b) {
	return (a < b) ? a : b;
}
