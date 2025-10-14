#!/bin/bash

# Check if container name is provided
if [ -z "$1" ]; then
    echo "Usage: $0 <container_name>"
    exit 1
fi

CONTAINER_NAME="$1"

# Check if container exists
if ! docker ps -a --format '{{.Names}}' | grep -qw "$CONTAINER_NAME"; then
    echo "Container '$CONTAINER_NAME' does not exist."
    exit 1
fi

# Check if container is running
if [ "$(docker inspect -f '{{.State.Running}}' $CONTAINER_NAME)" = "true" ]; then
    echo "Container '$CONTAINER_NAME' is running. Attaching..."
    docker exec -it "$CONTAINER_NAME" /bin/bash
else
    echo "Container '$CONTAINER_NAME' is stopped. Starting interactively..."
    docker start -ai "$CONTAINER_NAME"
fi
