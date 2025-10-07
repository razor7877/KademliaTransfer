#define _GNU_SOURCE
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>

#include "network.h"
#include "client.h"
#include "command.h"
#include "log.h"

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

/**
 * @brief The queue for issuing commands from frontend to P2P client
 * 
 */
struct CommandQueue commands = {0};

int start_client() {
    log_msg(LOG_INFO, "Starting P2P client");

    // Init command queue
    if (queue_init(&commands) != 0) {
        perror("queue_init failed");
        return -1;
    }

    // Start P2P client thread
    atomic_store(&thread_running, true);
    int res = pthread_create(&p2p_thread, NULL, init_client, NULL);
    log_msg(LOG_INFO, "pthread_create result: %d", res);

    if (res != 0) {
        perror("pthread_create failed");
        atomic_store(&thread_running, false);
        return -1;
    }

    return 0;
}

void* init_client(void* arg) {
    // We are on the secondary thread, set a specific log color for this one
    log_set_thread_color(LOG_COLOR_MAGENTA);

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

    log_msg(LOG_INFO, "Stopped P2P client (P2P thread)");
}

void stop_client() {
    log_msg(LOG_INFO, "Stopping P2P client");
    atomic_store(&thread_running, false);

    pthread_join(p2p_thread, NULL);
    log_msg(LOG_INFO, "Stopped P2P client (main thread)");

    queue_destroy(&commands);
}

int download_file(struct FileMagnet* file) {
    struct Command c = {
        .cmd_type = CMD_DOWNLOAD,
        .file = file
    };

    queue_push(&commands, &c);

    return -1;
}

int upload_file(struct FileMagnet* file) {
    struct Command c = {
        .cmd_type = CMD_UPLOAD,
        .file = file
    };

    queue_push(&commands, &c);

    return -1;
}

void show_network_status() {
    log_msg(LOG_INFO, "Showing network status");

    struct Command c = {
        .cmd_type = CMD_SHOW_STATUS,
        .file = NULL
    };

    queue_push(&commands, &c);
}
