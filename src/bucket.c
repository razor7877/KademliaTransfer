#include "bucket.h"
#include "log.h"

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

struct Peer* find_closest_peers(Buckets* buckets, HashID* target, int n) {
    struct Peer* result = malloc(n * sizeof(struct Peer));
    int count = 0;
    
    char ip[INET_ADDRSTRLEN] = {0};
    if (!get_primary_ip(ip, sizeof(ip))) {
        log_msg(LOG_ERROR, "find_closest_peers get_primary_ip error");
        return NULL;
    }

    log_msg(LOG_DEBUG, "Client primary IP is: %s", ip);

    return result;
}

void update_bucket_peers(struct Peer* peer) {
    log_msg(LOG_DEBUG, "update_bucket_peers");
}
