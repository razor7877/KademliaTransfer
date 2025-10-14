# Use a minimal runtime image
FROM ubuntu:22.04

# Set working directory
WORKDIR /app

# Copy the prebuilt binary from host to container
COPY build/KademliaClient ./KademliaClient

# Make sure it's executable
RUN chmod +x ./KademliaClient

# Expose ports
EXPOSE 8182
EXPOSE 8183
