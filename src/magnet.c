#include "magnet.h"

#include <string.h>

struct FileMagnet* create_magnet(char* filename, size_t filename_len,
                                 char* contents, size_t contents_len) {
  pointer_not_null(filename, "Error in create_magnet the filename is null!\n");
  pointer_not_null(contents, "Error in create_magnet the contents is null!\n");
  if (filename_len < 1 || contents_len < 1) {
    fprintf(stderr,
            "Error in create_magnet, you can't create magnet from not a valid "
            "file!\n");
    exit(EXIT_FAILURE);
  }

  struct FileMagnet* new_magnet =
      (struct FileMagnet*)malloc(sizeof(struct FileMagnet));
  pointer_not_null(new_magnet,
                   "Error in create_magnet the new_magnet is unitialized!\n");

  new_magnet->display_name = (char*)malloc(sizeof(char) * filename_len + 1);
  pointer_not_null(new_magnet->display_name,
                   "Error in create_magnet display_name alloc failed!\n");
  memcpy(new_magnet->display_name, filename, filename_len);
  new_magnet->display_name[filename_len] = '\0';
  new_magnet->exact_length = contents_len;
  SHA256(contents, contents_len, new_magnet->file_hash);
  new_magnet->address_tracker = NULL;
  new_magnet->web_seed = NULL;
  new_magnet->acceptable_source = NULL;
  new_magnet->exact_source = NULL;
  new_magnet->keyword_topic = NULL;
  new_magnet->manifest_topic = NULL;
  new_magnet->select_only = NULL;
  new_magnet->peer_addresses = NULL;
  return new_magnet;
}

char* save_magnet_to_uri(struct FileMagnet* magnet) {
  pointer_not_null(
      magnet, "Error in save_magnet_to_uri, the provided magnet is null!\n");

  const char* base = "magnet:?xt=urn:sha256:";
  char file_size[32];
  sprintf(file_size, "%zu", magnet->exact_length);
  size_t uri_size = strlen(base) + strlen(magnet->display_name) +
                    strlen(file_size) + SHA256_DIGEST_LENGTH * 2 + 8;
  char* uri = (char*)malloc(sizeof(char) * uri_size);
  pointer_not_null(uri, "Error in save_magnet_to_uri, uri alloc failed!\n");
  uri[0] = '\0';

  char hash_hex[65];
  for (int i = 0; i < 32; i++)
    sprintf(hash_hex + i * 2, "%02x", magnet->file_hash[i]);
  hash_hex[64] = '\0';

  strncat(uri, base, uri_size - strlen(uri) - 1);
  strncat(uri, hash_hex, uri_size - strlen(uri) - 1);
  strncat(uri, "&dn=", uri_size - strlen(uri) - 1);
  strncat(uri, magnet->display_name, uri_size - strlen(uri) - 1);
  strncat(uri, "&xl=", uri_size - strlen(uri) - 1);
  strncat(uri, file_size, uri_size - strlen(uri) - 1);

  return uri;
}

struct FileMagnet* parse_magnet_from_uri(char* contents, size_t len) {
  return NULL;
}

void free_magnet(struct FileMagnet* magnet) { free(magnet); }
