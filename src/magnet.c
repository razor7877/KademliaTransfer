#include "magnet.h"

#include <string.h>

struct FileMagnet* create_magnet(char* filename, size_t filename_len,
                                 char* contents, size_t contents_len) {
  struct FileMagnet* new_magnet =
      (struct FileMagnet*)malloc(sizeof(struct FileMagnet));
  pointer_not_null(new_magnet,
                   "Error in create_magnet the new_magnet is unitialized!\n");

  new_magnet->display_name = (char*)malloc(sizeof(char) * filename_len);
  pointer_not_null(new_magnet->display_name,
                   "Error in create_magnet display_name alloc failed!\n");
  memcpy(new_magnet->display_name, filename, filename_len);
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

char* save_magnet_to_uri(struct FileMagnet* magnet) { return NULL; }

struct FileMagnet* parse_magnet_from_uri(char* contents, size_t len) {
  return NULL;
}

void free_magnet(struct FileMagnet* magnet) { free(magnet); }
