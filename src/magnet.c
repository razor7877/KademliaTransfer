#include "magnet.h"
#include "shared.h"

#include <regex.h>
#include <string.h>

struct FileMagnet* create_magnet(const char* filename, size_t filename_len) {
  if (!filename || filename_len < 1) return NULL;
  struct FileMagnet* new_magnet =
      (struct FileMagnet*)malloc(sizeof(struct FileMagnet));
  pointer_not_null(new_magnet,
                   "Error in create_magnet the new_magnet alloc failed!\n");

  int contents_len = sha256_file(filename, &new_magnet->file_hash);
  if (contents_len == -1) {
    free_magnet(new_magnet);
    return NULL;
  }

  new_magnet->display_name = (char*)malloc(sizeof(char) * filename_len + 1);
  pointer_not_null(new_magnet->display_name,
                   "Error in create_magnet display_name alloc failed!\n");
  memcpy(new_magnet->display_name, filename, filename_len);
  new_magnet->display_name[filename_len] = '\0';

  new_magnet->exact_length = contents_len;
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
  if (!magnet) return NULL;

  const char* base = "magnet:?xt=urn:sha256:";
  size_t uri_size = strlen(base) + SHA256_DIGEST_LENGTH * 2 + 1;
  char file_size[32] = "";

  if (magnet->exact_length > 0) {
    sprintf(file_size, "%zu", magnet->exact_length);
    uri_size += strlen(file_size) + 4;
  }
  if (magnet->display_name != NULL) {
    uri_size += strlen(magnet->display_name) + 4;
  }

  char* uri = (char*)malloc(sizeof(char) * uri_size);
  pointer_not_null(uri, "Error in save_magnet_to_uri, uri alloc failed!\n");
  uri[0] = '\0';

  char hash_hex[65];
  for (int i = 0; i < 32; i++)
    sprintf(hash_hex + i * 2, "%02x", magnet->file_hash[i]);
  hash_hex[64] = '\0';

  strncat(uri, base, uri_size - strlen(uri) - 1);
  strncat(uri, hash_hex, uri_size - strlen(uri) - 1);

  if (magnet->display_name != NULL) {
    strncat(uri, "&dn=", uri_size - strlen(uri) - 1);
    strncat(uri, magnet->display_name, uri_size - strlen(uri) - 1);
  }

  if (magnet->exact_length > 0) {
    strncat(uri, "&xl=", uri_size - strlen(uri) - 1);
    strncat(uri, file_size, uri_size - strlen(uri) - 1);
  }

  return uri;
}

struct FileMagnet* parse_magnet_from_uri(char* contents, size_t len) {
  if (len < 1 || !contents) return NULL;

  regex_t match;
  regmatch_t matches[3];
  int regex_return_value = 0;

  if (regcomp(&match, "^(magnet:\\?xt=urn:sha256:)([A-Fa-f0-9]{64})",
              REG_EXTENDED)) {
    fprintf(stderr, "Could not compile regex\n");
    exit(EXIT_FAILURE);
  }
  regex_return_value = regexec(&match, contents, 3, matches, 0);
  regfree(&match);
  if (regex_return_value == REG_NOMATCH || regex_return_value != 0) return NULL;

  struct FileMagnet* new_magnet =
      (struct FileMagnet*)malloc(sizeof(struct FileMagnet));
  pointer_not_null(
      new_magnet,
      "Error in parse_magnet_from_uri : new_magnet alloc failed!\n");
  char hash[65] = {0};
  memcpy(hash, contents + matches[2].rm_so,
         matches[2].rm_eo - matches[2].rm_so);
  for (int i = 0; i < 32; i++) {
    sscanf(hash + 2 * i, "%2hhx", &new_magnet->file_hash[i]);
  }

  if (regcomp(&match, "&dn=([^&]*)", REG_EXTENDED)) {
    fprintf(stderr, "Could not compile regex\n");
    exit(EXIT_FAILURE);
  }

  regex_return_value = regexec(&match, contents, 2, matches, 0);
  regfree(&match);
  if (regex_return_value == REG_NOMATCH || regex_return_value != 0)
    new_magnet->display_name = NULL;
  else {
    new_magnet->display_name =
        (char*)malloc(sizeof(char) * (matches[1].rm_eo - matches[1].rm_so) + 1);
    pointer_not_null(
        new_magnet->display_name,
        "Error in parse_magnet_from_uri : display_name alloc failed!\n");
    memcpy(new_magnet->display_name, contents + matches[1].rm_so,
           (matches[1].rm_eo - matches[1].rm_so));
    new_magnet->display_name[matches[1].rm_eo - matches[1].rm_so] = '\0';
  }

  if (regcomp(&match, "&xl=([^&]*)", REG_EXTENDED)) {
    fprintf(stderr, "Could not compile regex\n");
    exit(EXIT_FAILURE);
  }

  regex_return_value = regexec(&match, contents, 2, matches, 0);
  regfree(&match);
  if (regex_return_value == 0) {
    char file_size[32] = "";
    memcpy(file_size, contents + matches[1].rm_so,
           (matches[1].rm_eo - matches[1].rm_so));
    new_magnet->exact_length = atoi(file_size);
  }

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

void free_magnet(struct FileMagnet* magnet) {
  if (!magnet) return;

  if (magnet->display_name) free(magnet->display_name);
  free(magnet);
}
