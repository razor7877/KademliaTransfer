#pragma once

#include <openssl/sha.h>

#include "bucket.h"
#include "shared.h"
#define SHA256_BLOCK_SIZE 4096
/**
 * @file magnet.h
 * @brief Magnet URI implementation
 *
 * This file defines structures and functions for the manipulation of magnet
 * URIs. This follows the magnet URI standard. It allows creating new magnets
 * from files, as well as exporting/importing magnets to and from files.
 *
 */

struct Range {
  int begin;
  int end;
};

/**
 * @brief Represents the state for a file magnet using the Magnet URI Scheme
 * (more detail [here](https://en.wikipedia.org/wiki/Magnet_URI_scheme))
 *
 */
struct FileMagnet {
  /**
   * @brief The SHA-256 hash of the file
   *
   */
  HashID file_hash;

  /**
   * @brief Filename to display to the user
   *
   */
  char* display_name;

  /**
   * @brief File size in bytes
   *
   */
  size_t exact_length;

  /**
   * @brief Tracker URL
   *
   */
  char* address_tracker;

  /**
   * @brief Web seed address
   *
   */
  char* web_seed;

  /**
   * @brief Fallback web server download address if no available peers
   *
   */
  char* acceptable_source;

  /**
   * @brief Download source for the file, generally over HTTP
   *
   */
  char* exact_source;

  /**
   * @brief List of keyword topics
   *
   */
  char* keyword_topic;

  /**
   * @brief Link to the metafile that contains a list of magnets
   *
   */
  char* manifest_topic;

  /**
   * @brief The specific files that should be downloaded on the torrent
   *
   */
  struct Range* select_only;

  /**
   * @brief An array containing the addresses of peers to connect to
   *
   */
  struct Peer** peer_addresses;
};

/**
 * @brief Create a magnet object from a file
 *
 * @param filename A buffer pointing to the filename
 * @param filename_len The length of the filename
 * @return struct FileMagnet* Returns a pointer to a new magnet
 */
struct FileMagnet* create_magnet(const char* filename, size_t filename_len);

/**
 * @brief Serializes the magnet data to URI format
 *
 * @param magnet The magnet to be serialized
 * @return char* Returns a pointer to a buffer containing the URI contents
 */
char* save_magnet_to_uri(struct FileMagnet* magnet);

/**
 * @brief Parses the contents of a magnet from the URI representation
 *
 * @param contents A buffer pointing to the UTF-8 URI representation
 * @param len The length of the URI representation
 * @return FileMagnet* Returns a pointer to the parsed structure, caller is
 * responsible for freeing
 */
struct FileMagnet* parse_magnet_from_uri(char* contents, size_t len);

/**
 * @brief Frees the resources allocated for a magnet
 *
 * @param magnet The magnet for which the resources should be freed
 */
void free_magnet(struct FileMagnet* magnet);
