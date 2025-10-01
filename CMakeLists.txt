cmake_minimum_required(VERSION 3.20)
project(KademliaTransfer)

include_directories(
    src
)

add_executable(KademliaClient
    src/main.c
    src/shared.c
)
