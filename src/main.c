#include <stdio.h>

#include "network.h"

int main(int argc, char** argv) {
    printf("Hey\n");

    init_network();

    while (1) {
        // Query user input ...
        
        update_network();
    }

    stop_network();

    return 0;
}