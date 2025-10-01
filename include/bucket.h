// TODO : Buckets implementation
// Functions for manipulating bucket contents (nearest neighbor find etc.)

#include <arpa/inet.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <openssl/evp.h>

/**
 * @brief Represents a single peer in the network
 * 
 */
struct Peer {
    /**
     * @brief The 256 identifier for the node
     * 
     */
    unsigned char peer_id[SHA256_DIGEST_LENGTH];

    /**
     * @brief The network information for communicating with the node
     * 
     */
    struct sockaddr_in peer_addr;

    /**
     * @brief The public key of the node
     * 
     */
    EVP_PKEY* peer_pub_key;
};