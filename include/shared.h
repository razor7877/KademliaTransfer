#include <openssl/evp.h>
#include <openssl/sha.h>
#include <stdio.h>

#define BUF_SIZE 16384
#define FILE_BLOCK_SIZE 4096

#define K_VALUE 2

typedef unsigned char HashID[SHA256_DIGEST_LENGTH];
typedef unsigned char PubKey[32];

void die(int val, char* str);
void recv_all(int fd, void* dst, size_t len);
void recv_until(int fd, void* dst, size_t len, char* pattern,
                size_t pattern_len);
void send_all(int fd, void* src, size_t len);
int min(const int a, const int b);
void pointer_not_null(void* ptr, const char* message);
