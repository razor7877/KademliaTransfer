# Stage 1: Build
FROM ubuntu:22.04 AS builder

# Install build tools
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    gdb \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# Copy source code
COPY CMakeLists.txt .
COPY src/ ./src
COPY include/ ./include
COPY lib/ ./lib

# Build using CMake
RUN mkdir -p build && cd build && \
    cmake .. && make -j$(nproc)

# Stage 2: Runtime
FROM ubuntu:22.04

WORKDIR /app

# Copy built binary
COPY --from=builder /build/build/KademliaClient ./KademliaClient

# Make executable
RUN chmod +x ./KademliaClient

# Expose ports
EXPOSE 8182
EXPOSE 8183

# Run the client, passing the node name as an argument
# ENTRYPOINT ["./KademliaClient"]
