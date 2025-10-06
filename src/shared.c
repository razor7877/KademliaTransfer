#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

void die(int val, char *str) {
	if (val < 0) {
		perror(str);
		exit(EXIT_FAILURE);
	}
}

ssize_t recv_all(int fd, void* dst, size_t len) {
	size_t nb_read = 0;
	ssize_t ret = 0;

	while (nb_read < len) {
        ret = read(fd, (char *)dst + nb_read, len - nb_read);

        if (ret < 0) {
			// Retry
            if (errno == EINTR)
				continue;

            perror("read");
            return -1;
        }

		// EOF
        if (ret == 0)
			break;

        nb_read += ret;
    }
	
	return ret;
}

ssize_t recv_until(int fd, void* dst, size_t buf_len, const char* pattern, size_t pattern_len) {
	size_t nb_read = 0;
	ssize_t ret = 0;

	while (nb_read < buf_len) {
		ret = read(fd, (char *)dst + nb_read, 1);

        if (ret < 0) {
            // Retry
			if (errno == EINTR)
				continue;

			perror("read");
            return -1;
        }

		// EOF
        if (ret == 0)
			break;

        nb_read += ret;

        if (nb_read >= pattern_len) {
            bool match = true;

            for (size_t i = 0; i < pattern_len; i++) {
                if (*((char *)dst + nb_read - pattern_len + i) != pattern[i]) {
                    match = false;
                    break;
                }
            }

            if (match)
				break;
        }
	}

	return nb_read;
}

ssize_t send_all(int fd, const void* src, size_t len) {
	size_t nb_sent = 0;
	ssize_t ret = 0;

	while (nb_sent < len) {
		ret = write(fd, (char*)src + nb_sent, len - nb_sent);
		if (ret < 0) {
            if (errno == EINTR)
				continue;

            perror("write");
            return -1;
        }
		nb_sent += ret;
	}

	return nb_sent;
}

int min(const int a, const int b) {
	return (a < b) ? a : b;
}
