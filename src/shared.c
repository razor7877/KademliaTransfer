#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <openssl/sha.h>

#include "log.h"
#include "shared.h"

void die(int val, char * str) {
	if (val < 0) {
		perror(str);
		exit(EXIT_FAILURE);
	}
}

ssize_t recv_all(int fd, void * dst, size_t len) {
	size_t nb_read = 0;
	ssize_t ret = 0;

	while (nb_read < len) {
		ret = read(fd, (char * ) dst + nb_read, len - nb_read);

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

ssize_t recv_all_peek(int fd, void * dst, size_t len) {
	size_t nb_read = 0;
	ssize_t ret = 0;

	while (nb_read < len) {
		ret = recv(fd, (char * ) dst + nb_read, len - nb_read, MSG_PEEK);

		if (ret < 0) {
			// Retry
			if (errno == EINTR)
				continue;

			perror("recv(MSG_PEEK)");
			return -1;
		}

		// EOF
		if (ret == 0)
			break;

		nb_read += ret;
	}

	return nb_read;
}

ssize_t recv_until(int fd, void * dst, size_t buf_len, const char * pattern, size_t pattern_len) {
	size_t nb_read = 0;
	ssize_t ret = 0;

	while (nb_read < buf_len) {
		ret = read(fd, (char * ) dst + nb_read, 1);

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
				if ( * ((char * ) dst + nb_read - pattern_len + i) != pattern[i]) {
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

ssize_t send_all(int fd, const void * src, size_t len) {
	size_t nb_sent = 0;
	ssize_t ret = 0;

	while (nb_sent < len) {
		ret = write(fd, (char * ) src + nb_sent, len - nb_sent);
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

void pointer_not_null(void * ptr, const char * message) {
	if (!ptr) {
		log_msg(LOG_ERROR, message);
		exit(EXIT_FAILURE);
	}
}

int sha256_file(const char* filename, HashID* id) {
  FILE* file = fopen(filename, "rb");
  if (!file) return -1;
  int bytes_read = 0;
  int file_size = 0;
  unsigned char block[SHA256_BLOCK_SIZE];

  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (!ctx) {
    fclose(file);
    return -1;
  }

  const EVP_MD* md = EVP_sha256();

  if (EVP_DigestInit_ex(ctx, md, NULL) != 1) {
    EVP_MD_CTX_free(ctx);
    fclose(file);
    return -1;
  }

  while ((bytes_read = fread(block, 1, sizeof(block), file)) > 0) {
    if (EVP_DigestUpdate(ctx, block, bytes_read) != 1) {
      EVP_MD_CTX_free(ctx);
      fclose(file);
      return -1;
    }
    file_size += bytes_read;
  }
  if (ferror(file)) {
    EVP_MD_CTX_free(ctx);
    fclose(file);
    return -1;
  }

  unsigned int hash_size = SHA256_DIGEST_LENGTH;

  if (EVP_DigestFinal_ex(ctx, (unsigned char*)id, &hash_size) != 1) {
    EVP_MD_CTX_free(ctx);
    fclose(file);
    return -1;
  }

  EVP_MD_CTX_free(ctx);
  fclose(file);
  return file_size;
}

int sha256_buf(const unsigned char* in_buf, size_t buf_size, unsigned char* out_buf) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) return -1;

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), NULL) != 1) {
        EVP_MD_CTX_free(ctx);
        return -1;
    }

    if (EVP_DigestUpdate(ctx, in_buf, buf_size) != 1) {
        EVP_MD_CTX_free(ctx);
        return -1;
    }

    unsigned int out_len = 0;

    if (EVP_DigestFinal_ex(ctx, out_buf, &out_len) != 1) {
        EVP_MD_CTX_free(ctx);
        return -1;
    }

	return 0;
}

static int get_primary_ip(char* ip_buf, size_t buf_size) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        log_msg(LOG_ERROR, "get_primary_ip socket error");
        return -1;
    }

    struct sockaddr_in serv = {0};

    serv.sin_family = AF_INET;
    // DNS Port
    serv.sin_port = htons(53);

    // OpenDNS
    inet_pton(AF_INET, "208.67.222.222", &serv.sin_addr);

    if (connect(sock, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
        log_msg(LOG_ERROR, "get_primary_ip connect error");
        close(sock);
        return -1;
    }

    struct sockaddr_in name = {0};
    socklen_t name_len = sizeof(name);

    if (getsockname(sock, (struct sockaddr*)&name, &name_len) < 0) {
        log_msg(LOG_ERROR, "get_primary_ip getsockname error");
        close(sock);
        return -1;
    }

    const char* result = inet_ntop(AF_INET, &name.sin_addr, ip_buf, buf_size);
    close(sock);

    return (result != NULL) ? 0 : -1;
}

int get_own_id(HashID* out) {
    char ip[INET_ADDRSTRLEN] = {0};
    if (get_primary_ip(ip, sizeof(ip)) != 0) {
        log_msg(LOG_ERROR, "get_own_id get_primary_ip error");
        return -1;
    }

    log_msg(LOG_DEBUG, "Client primary IP is: %s", ip);

    unsigned char ip_hash[32] = {0};

    sha256_buf(ip, strlen(ip), ip_hash);
    log_msg(LOG_DEBUG, "IP length is: %d", strlen(ip));

	char hash_str[sizeof(HashID) * 2 + 1] = {0};
	sha256_to_hex((HashID*)ip_hash, hash_str);
    log_msg(LOG_DEBUG, "Hash: %s", hash_str);

	return 0;
}

void sha256_to_hex(const HashID* hash, char* str_buf) {
    for (int i = 0; i < sizeof(HashID); i++) {
        sprintf(str_buf + (i * 2), "%02x", *hash[i]);
    }
    str_buf[sizeof(HashID) * 2] = '\0'; // null-terminate
}
