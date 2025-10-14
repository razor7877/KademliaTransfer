cmake_minimum_required(VERSION 3.20)
project(KademliaTransfer)

include_directories(
    include
    lib
)

find_package(OpenSSL REQUIRED)

add_executable(KademliaClient
    src/main.c
    src/shared.c
    src/client.c
    src/magnet.c
    src/network.c
    src/rpc.c
    src/http.c
    src/list.c
    src/command.c
    src/log.c
    src/storage.c
    src/peer.c
    src/bucket.c
    src/vector.c

    lib/hash/hashmap.c
)

target_link_libraries(KademliaClient OpenSSL::Crypto)

# Doxygen configuration

include(FetchContent)
FetchContent_Declare(
    doxygen-awesome-css
    URL https://github.com/jothepro/doxygen-awesome-css/archive/refs/heads/main.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(doxygen-awesome-css)

# Save the location the files were cloned into
# This allows us to get the path to doxygen-awesome.css
FetchContent_GetProperties(doxygen-awesome-css SOURCE_DIR AWESOME_CSS_DIR)

# Doxygen generation target

find_package(Doxygen QUIET)

if (DOXYGEN_FOUND)
    # Generate the Doxyfile in binary dir
    set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in)
    set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

    add_custom_target(doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )

else()
    message(STATUS "Doxygen not found — 'make doc' target will be unavailable.")
endif()
