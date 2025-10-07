#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "client.h"
#include "log.h"

#define INPUT_SIZE 256
#define FILENAME_SIZE 512

void cli_download_file() {
    char magnet_link[FILENAME_SIZE] = {0};

    printf("Enter filename of magnet to download: ");

    if (fgets(magnet_link, sizeof(magnet_link), stdin) == NULL)
        return;

    magnet_link[strcspn(magnet_link, "\n")] = '\00';

    // Load magnet file contents from disk
    FILE* file = fopen(magnet_link, "r");

    if (file == NULL) {
        log_msg(LOG_ERROR, "File does not exist! Please enter valid filename.\n");
        return;
    }

    // Get size, allocate memory and read contents into the buffer
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    void* contents = malloc(size);

    fread(contents, size, 1, file);
    fclose(file);

    struct FileMagnet* magnet = parse_magnet_from_uri(contents, size);

    free(contents);

    if (magnet == NULL) {
        log_msg(LOG_ERROR, "Error while parsing magnet link contents! Please use a valid magnet file.\n");
        return;
    }

    int res = download_file(magnet);
    free_magnet(magnet);

    if (res == 0) {
        log_msg(LOG_INFO, "File successfully downloaded!\n");
    }
    else {
        log_msg(LOG_ERROR, "Error while trying to download the file! Error code: %d\n", res);
    }
}

void cli_upload_file() {
    char filename[FILENAME_SIZE] = {0};

    printf("Enter filename to upload: ");

    if (fgets(filename, sizeof(filename), stdin) == NULL)
        return;
    
    filename[strcspn(filename, "\n")] = '\00';

    FILE* file = fopen(filename, "r");

    if (file == NULL) {
        log_msg(LOG_ERROR, "File does not exist! Please enter valid filename.\n");
        return;
    }

    // Get size, allocate memory and read contents into the buffer
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    void* contents = malloc(size);

    fread(contents, size, 1, file);
    fclose(file);

    struct FileMagnet* magnet = create_magnet(filename, strlen(filename), contents, size);

    if (magnet == NULL) {
        log_msg(LOG_ERROR, "Error while trying to create magnet link!\n");
        return;
    }

    int res = upload_file(magnet);
    
    free(contents);

    if (res == 0) {
        log_msg(LOG_INFO, "File successfully uploaded!\n");
    }
    else {
        log_msg(LOG_ERROR, "Error while trying to upload the file! Error code: %d\n", res);
    }
}

void cli_show_network_status() {
    show_network_status();
}

int main(int argc, char** argv) {
    if (start_client() != 0) {
        perror("P2P client didn't start properly");
        exit(EXIT_FAILURE);
    }
    
    bool running = true;
    char input[INPUT_SIZE] = {0};
    int choice = -1;
    // pid_t tid = gettid();

    while (running) {
        //printf("Updating CLI - TID: %d\n", tid);

        printf("\n===== KademliaTransfer Menu =====\n");
        printf("1. Show network status\n");
        printf("2. Upload file\n");
        printf("3. Download file\n");
        printf("4. Exit\n");
        printf("Enter your choice: ");

        if (fgets(input, sizeof(input), stdin) == NULL) {
            log_msg(LOG_WARN, "\nError reading input\n");
            continue;
        }

        printf("\n");
        
        input[strcspn(input, "\n")] = '\00';

        if (sscanf(input, "%d", &choice) != 1) {
            log_msg(LOG_WARN, "Invalid input! Please enter a number.\n");
            continue;
        }

        switch (choice) {
            case 1:
                cli_show_network_status();
                break;

            case 2:
                cli_upload_file();
                break;

            case 3:
                cli_download_file();
                break;

            case 4:
                log_msg(LOG_INFO, "Exiting program...\n");
                running = false;
                break;

            default:
                log_msg(LOG_WARN, "Invalid choice! Please select 1-4.\n");
                break;
        }
    }

    stop_client();

    return 0;
}