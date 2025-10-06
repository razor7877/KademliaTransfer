#include "magnet.h"

struct FileMagnet* create_magnet(char* filename, size_t filename_len, char* contents, size_t contents_len) {
    return NULL;
}

char* save_magnet_to_uri(struct FileMagnet* magnet) {
    return NULL;
}

struct FileMagnet* parse_magnet_from_uri(char* contents, size_t len) {
    return NULL;
}

void free_magnet(struct FileMagnet* magnet) {
    free(magnet);
}
