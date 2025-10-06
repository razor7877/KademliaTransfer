#define _GNU_SOURCE
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>

#include "network.h"
#include "client.h"

/**
 * @brief This gets set when the main thread requests the P2P thread to stop its execution
 * 
 */
atomic_bool thread_request_stop = ATOMIC_VAR_INIT(false);

/**
 * @brief This indicates whether the P2P thread is currently running
 * 
 */
atomic_bool thread_running = ATOMIC_VAR_INIT(false);

/**
 * @brief The secondary P2P thread
 * 
 */
pthread_t p2p_thread;

void start_client() {
    printf("Starting P2P client\n");

    atomic_store(&thread_running, true);
    int res = pthread_create(&p2p_thread, NULL, init_client, NULL);
    printf("pthread_create result: %d\n", res);

    if (res != 0) {
        perror("pthread_create failed!");
        atomic_store(&thread_running, false);
    }
}

void* init_client(void* arg) {
    init_network();

    update_client();
}

void update_client() {
    // Run until the main thread tells us to stop
    while (atomic_load(&thread_running)) {
        update_network();
    }

    stop_network();
    atomic_store(&thread_request_stop, true);

    printf("Stopped P2P client (P2P thread)\n");
}

void stop_client() {
    printf("Stopping P2P client\n");
    atomic_store(&thread_running, false);

    pthread_join(p2p_thread, NULL);
    printf("Stopped P2P client (main thread)\n");
}

int download_file(struct FileMagnet* file) {
    return -1;
}

int upload_file(struct FileMagnet* file) {
    return -1;
}

void show_network_status() {
    printf("Showing network status\n");
}
