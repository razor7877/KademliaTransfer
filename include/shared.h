#pragma once

#include <netinet/in.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <unistd.h>

#define BUF_SIZE 16384
#define FILE_BLOCK_SIZE 4096
#define SHA256_BLOCK_SIZE 4096

#define K_VALUE 4

#define UPLOAD_DIR "./upload"
#define DOWNLOAD_DIR "./download"

struct Peer;

/**
 * @file shared.h
 * @brief Shared Functions
 *
 * This file defines a number of useful functions and constants that may be
 * reused anywhere else in the codebase
 *
 */

/**
 * @brief Represents a single ID in the network, this may represent a user or
 * file id, or a distance between any thereof
 *
 */
typedef unsigned char HashID[SHA256_DIGEST_LENGTH];

/**
 * @brief Stores the public key for any given peer
 *
 */
typedef unsigned char PubKey[32];

/**
 * @brief Checks return value and exits program if there is an error. Only used
 * for critical features
 *
 * @param val The return value
 * @param str The message to display if there is an error
 */
void die(int val, const char *str);

/**
 * @brief Repeatedly reads from a file descriptor until all requested data is
 * read or there is an error
 *
 * @param fd The file descriptor to read from
 * @param dst A buffer pointing to the memory where the data should be stored
 * @param len The number of bytes to be read
 * @return size_t Returns how many bytes were actually read
 */
ssize_t recv_all(int fd, void *dst, size_t len);

/**
 * @brief Repeatedly peeks from a file descriptor until all requested data is
 * peeked or there is an error. This avoids consuming from the internal kernel
 * buffer
 *
 * @param fd The file descriptor to read from
 * @param dst A buffer pointing to the memory where the data should be stored
 * @param len The number of bytes to be read
 * @return size_t Returns how many bytes were actually read
 */
ssize_t recv_all_peek(int fd, void *dst, size_t len);

/**
 * @brief Repeatedly reads from a file descriptor until we reached max length or
 * the pattern is encountered or there is an error
 *
 * @param fd The file descriptor to read from
 * @param dst A buffer pointing to the memory where the data should be stored
 * @param len The number of bytes to be read
 * @param pattern The pattern upon which reading should be stopped
 * @param pattern_len The length of the pattern
 * @return size_t Returns how many bytes were actually read
 */
ssize_t recv_until(int fd, void *dst, size_t len, const char *pattern,
                   size_t pattern_len);

/**
 * @brief Repeatedly sends to a file descriptor until all request data is sent
 * or there is an error
 * @param fd The file descriptor to send to
 * @param src A buffer pointing to the memory where the data should be read from
 * @param len The number of bytes to be sent
 * @return size_t Returns how many bytes were actually sent
 */
ssize_t send_all(int fd, const void *src, size_t len);

/**
 * @brief Returns the minimum value
 *
 * @param a First value
 * @param b Second value
 * @return int Returns the minimum value between a and b
 */
int min(const int a, const int b);

/**
 * @brief Checks for NULL pointers when allocating memory, and exits the client
 * if allocation failed
 *
 * @param ptr The pointer to check
 * @param message The message to display in case of exit
 */
void pointer_not_null(void *ptr, const char *message);

/**
 * @brief Computes and store the SHA-256 hash of a given file
 *
 * @param filename A char buffer containing the name of the file to hash
 * @param id A pointer to memory where the hash should be stored
 * @return int Returns 0 if the hash was computed successfully, a negative
 * number otherwise
 */
int sha256_file(const char *filename, HashID id);

/**
 * @brief Computes and store the SHA-256 hash of a buffer contents
 *
 * @param in_buf A buffer containing the contents for which the SHA-256 sum
 * should be computed
 * @param buf_size The size of the data to be hashed
 * @param out_buf A pointer to memory where the hash should be stored
 * @return int Returns 0 if the hash was computed successfully, a negative
 * number otherwise
 */
int sha256_buf(const unsigned char *in_buf, size_t buf_size,
               unsigned char *out_buf);

/**
 * @brief Gets the client primary ip
 *
 * @param ip_buf A buffer to store the string representation of the client IP,
 * it should be at least INET_ADDRSTRLEN bytes long
 * @param buf_size The size of the buffer to store the string representation
 * @param out_addr May be NULL, if present, a copy of struct sockaddr_in
 * contents will be copied into it
 * @return int Returns 0 if the primary IP was obtained successfully, a negative
 * number otherwise
 */
int get_primary_ip(char *ip_buf, size_t buf_size, struct sockaddr_in *out_addr);

/**
 * @brief Gets the client own network ID
 *
 * @param out A pointer to memory where the network ID of the client should be
 * stored
 * @return int Returns 0 if the ID was obtained successfully, a negative number
 * otherwise
 */
int get_own_id(HashID out);

/**
 * @brief Prepares a Peer object with the information of the client
 *
 * @param out_peer A pointer to memory where the peer information should be
 * stored
 * @return int Returns 0 if the client information was obtained successfully, a
 * negative number otherwise
 */
int create_own_peer(struct Peer *out_peer);

/**
 * @brief Converts the SHA-256 hash to a hex string.
 *
 * @param hash Pointer to 32-byte hash
 * @param str_buf Buffer to hold hex string, must be at least 65 bytes
 */
void sha256_to_hex(const HashID hash, char *str_buf);

char *strcasestr_portable(const char *haystack, const char *needle);
