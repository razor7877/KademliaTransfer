#pragma once

#include <stdio.h>
#include <openssl/sha.h>
#include <openssl/evp.h>

#define BUF_SIZE 16384
#define FILE_BLOCK_SIZE 4096

#define K_VALUE 2

typedef unsigned char HashID[SHA256_DIGEST_LENGTH];
typedef unsigned char PubKey[32];

void die(int val, char *str);
ssize_t recv_all(int fd, void* dst, size_t len);
ssize_t recv_until(int fd, void* dst, size_t len, const char* pattern, size_t pattern_len);
ssize_t send_all(int fd, const void* src, size_t len);
int min(const int a, const int b);
