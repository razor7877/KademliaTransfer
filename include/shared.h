#pragma once

#include <stdio.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <unistd.h>

#define BUF_SIZE 16384
#define FILE_BLOCK_SIZE 4096

#define K_VALUE 2

typedef unsigned char HashID[SHA256_DIGEST_LENGTH];
typedef unsigned char PubKey[32];

/**
 * @brief Checks return value and exits program if there is an error. Only used for critical features
 * 
 * @param val The return value
 * @param str The message to display if there is an error
 */
void die(int val, char *str);

/**
 * @brief Repeatedly reads from a file descriptor until all requested data is read or there is an error
 * 
 * @param fd The file descriptor to read from
 * @param dst A buffer pointing to the memory where the data should be stored
 * @param len The number of bytes to be read
 * @return ssize_t Returns how many bytes were actually read
 */
ssize_t recv_all(int fd, void* dst, size_t len);

/**
 * @brief Repeatedly peeks from a file descriptor until all requested data is peeked or there is an error.
 * This avoids consuming from the internal kernel buffer
 * 
 * @param fd The file descriptor to read from
 * @param dst A buffer pointing to the memory where the data should be stored
 * @param len The number of bytes to be read
 * @return ssize_t Returns how many bytes were actually read
 */
ssize_t recv_all_peek(int fd, void* dst, size_t len);

/**
 * @brief Repeatedly reads from a file descriptor until we reached max length or the pattern is encountered or there is an error
 * 
 * @param fd The file descriptor to read from
 * @param dst A buffer pointing to the memory where the data should be stored
 * @param len The number of bytes to be read
 * @param pattern The pattern upon which reading should be stopped
 * @param pattern_len The length of the pattern
 * @return ssize_t Returns how many bytes were actually read
 */
ssize_t recv_until(int fd, void* dst, size_t len, const char* pattern, size_t pattern_len);

/**
 * @brief Repeatedely sends to a file descriptor until all request data is sent or there is an error 
 * @param fd The file descriptor to send to
 * @param src A buffer pointing to the memory where the data should be read from
 * @param len The number of bytes to be sent
 * @return ssize_t Returns how many bytes were actually sent
 */
ssize_t send_all(int fd, const void* src, size_t len);

/**
 * @brief Returns the minimum value
 * 
 * @param a First value
 * @param b Second value
 * @return int Returns the minimum value between a and b
 */
int min(const int a, const int b);
