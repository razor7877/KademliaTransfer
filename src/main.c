#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "client.h"
#include "log.h"
#include "peer.h"

#define INPUT_SIZE 256
#define FILENAME_SIZE 512

static int copy_file(const char *src_path, const char *dest_path) {
  FILE *src = fopen(src_path, "rb");
  if (!src)
    return -1;

  FILE *dest = fopen(dest_path, "wb");
  if (!dest) {
    fclose(src);
    return -1;
  }

  char buffer[BUF_SIZE];
  size_t bytes;
  while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
    if (fwrite(buffer, 1, bytes, dest) != bytes) {
      fclose(src);
      fclose(dest);
      return -1;
    }
  }

  fclose(src);
  fclose(dest);
  return 0;
}

void cli_download_file() {
  char magnet_link[FILENAME_SIZE] = {0};

  printf("Enter filename of magnet to download: ");

  if (fgets(magnet_link, sizeof(magnet_link), stdin) == NULL)
    return;

  magnet_link[strcspn(magnet_link, "\n")] = '\0';

  struct stat st;
  if (stat(magnet_link, &st) != 0) {
    log_msg(LOG_ERROR, "File does not exist! Please entir valid filename.");
    return;
  }

  if (S_ISDIR(st.st_mode)) {
    log_msg(LOG_ERROR, "Can't open directory as file");
    return;
  }

  // Load magnet file contents from disk
  FILE *file = fopen(magnet_link, "r");
  if (file == NULL) {
    log_msg(LOG_ERROR, "File does not exist! Please enter valid filename.");
    return;
  }

  // Get size, allocate memory and read contents into the buffer
  fseek(file, 0, SEEK_END);
  long size = ftell(file);

  if (size < 0) {
    log_msg(LOG_ERROR, "Invalid file size or ftell failed");
    fclose(file);
    return;
  }

  fseek(file, 0, SEEK_SET);

  void *contents = malloc((size_t)size);
  pointer_not_null(contents, "cli_download_file malloc error");

  if (fread(contents, 1, size, file) != (size_t)size) {
    log_msg(LOG_ERROR, "Couldn't read entire file from disk - read %zu", read);
    fclose(file);
    return;
  }
  fclose(file);

  struct FileMagnet *magnet = parse_magnet_from_uri(contents, size);

  free(contents);

  if (magnet == NULL) {
    log_msg(LOG_ERROR, "Error while parsing magnet link contents! Please use a "
                       "valid magnet file.\n");
    return;
  }

  int res = download_file(magnet);
  free_magnet(magnet);

  if (res == 0) {
    log_msg(LOG_INFO, "File successfully downloaded!\n");
  } else {
    log_msg(LOG_ERROR,
            "Error while trying to download the file! Error code: %d\n", res);
  }
}

void cli_upload_file() {
  char input_path[FILENAME_SIZE] = {0};

  printf("Enter filename to upload (with optional path): ");

  if (fgets(input_path, sizeof(input_path), stdin) == NULL)
    return;

  input_path[strcspn(input_path, "\n")] = '\0';

  // Extract just the filename from the path
  const char *filename = strrchr(input_path, '/');
  filename = (filename) ? filename + 1 : input_path;

  FILE *file = fopen(input_path, "r");
  if (file == NULL) {
    log_msg(
        LOG_ERROR,
        "File does not exist at path '%s'! Please enter a valid filename.\n",
        input_path);
    return;
  }

  // Get size, allocate memory and read contents into the buffer
  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);

  void *contents = malloc(size);

  fread(contents, size, 1, file);
  fclose(file);

  struct FileMagnet *magnet = create_magnet(input_path, strlen(input_path));

  if (magnet == NULL) {
    log_msg(LOG_ERROR, "Error while trying to create magnet link!\n");
    return;
  }

  // Create the upload directory if not already present
  const char upload_dir[] = "./upload";
  if (access(upload_dir, F_OK) == -1) {
    if (mkdir(upload_dir, 0755) == -1) {
      log_msg(LOG_ERROR, "Failed to create upload directory %s: %s", upload_dir,
              strerror(errno));
      free_magnet(magnet);
      return;
    }
  }

  char new_path[512 + sizeof(upload_dir)] = {0};
  snprintf(new_path, sizeof(new_path), "%s/%s", upload_dir, filename);
  if (copy_file(input_path, new_path) != 0) {
    log_msg(LOG_WARN, "Could not copy uploaded file to %s: %s", new_path,
            strerror(errno));
  } else {
    log_msg(LOG_INFO, "Copied uploaded file to: %s", new_path);
  }

  int res = upload_file(magnet);
  free(contents);

  if (res == 0) {
    log_msg(LOG_INFO, "File successfully uploaded!\n");

    char *magnet_uri = save_magnet_to_uri(magnet);

    // Allocate new filename with ".torrent" appended
    const char suffix[] = ".torrent";
    char magnet_filename[512] = {0};
    snprintf(magnet_filename, sizeof(magnet_filename), "%s/%s%s", upload_dir,
             magnet->display_name, suffix);

    log_msg(LOG_INFO, "Saving following magnet URI: %s", magnet_uri);

    FILE *magnet_file = fopen(magnet_filename, "w+");
    if (!magnet_file) {
      log_msg(LOG_ERROR, "Error creating magnet file %s: %s", magnet_filename,
              strerror(errno));
      free(magnet_uri);
      free_magnet(magnet);
      return;
    }

    size_t uri_size = strlen(magnet_uri);
    size_t written = fwrite(magnet_uri, uri_size, 1, magnet_file);
    fclose(magnet_file);
    free(magnet_uri);

    if (written < 1)
      log_msg(LOG_ERROR, "Unable to write entire magnet URI to %s: %s",
              magnet_filename, strerror(errno));
    else
      log_msg(LOG_INFO, "Magnet file saved to: %s", magnet_filename);
  } else {
    log_msg(LOG_ERROR,
            "Error while trying to upload the file! Error code: %d\n", res);
  }

  free_magnet(magnet);
}

void cli_show_network_status() { show_network_status(); }

int main(int argc, char **argv) {
  if (start_client() != 0) {
    perror("P2P client didn't start properly");
    return -1;
  }

  bool disable_cli = false;
  char *env = getenv("DISABLE_CLI");
  if (env && strcmp(env, "1") == 0) {
    disable_cli = true;
  }

  if (!disable_cli) {
    bool running = true;
    char input[INPUT_SIZE] = {0};
    int choice = 0;

    while (running) {
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

      char *end_ptr;
      choice = (int)strtol(input, &end_ptr, 10);

      if (end_ptr == input || *end_ptr != '\0') {
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
  } else {
    log_msg(LOG_INFO, "CLI disabled, running headless mode...\n");
    while (1) {
      // Might want to implement something better for graceful exits
      sleep(60);
    }
  }

  stop_client();

  return 0;
}