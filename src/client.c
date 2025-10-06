#define _GNU_SOURCE
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>

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
    int res = pthread_create(&p2p_thread, NULL, update_client, NULL);
    printf("pthread_create result: %d\n", res);

    if (res != 0) {
        perror("pthread_create failed!");
        atomic_store(&thread_running, false);
    }
}

void* update_client(void* arg) {
    pid_t tid = gettid();

    while (atomic_load(&thread_running)) {
        //printf("Updating P2P client from secondary thread - TID: %lu\n", tid);
    }

    atomic_store(&thread_request_stop, true);

    printf("Stopped P2P client (P2P thread)\n");

    return NULL;
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
