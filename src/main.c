#include <stdio.h>

#include "client.h"

int main(int argc, char** argv) {
    start_client();

    while (1) {
        update_client();
    }

    stop_client();

    return 0;
}