# KademliaTransfer

KademliaTransfer is a lightweight P2P file-sharing client developed in C. It it based on the Kademlia distributed hash table for peer discovery and network traversal, and uses HTTP for file transfers.

# Docker Setup

## Fleet of nodes

1. Build the fleet of nodes:
```
docker-compose build
```

2. Start the fleet of nodes in background:
```
docker-compose up -d
```

3. Stop the fleet of nodes and delete them:
```
docker-compose down -v
```

## Interactive node

1. Build the container from project folder
```
docker build . -t kademlia_node
```

2. Start a single interactive node - This mounts the "files" folder into the container so you can easily interact with the downloaded and uploaded files from the host machine
```
docker run -it --rm --network kademliatransfer_kadnet --name manual_node -v $(pwd)/files:/app/files kademlia_node
```

# Trying out the project

1. Clone the project
```
git clone https://github.com/razor7877/KademliaTransfer.git
```

2. Run CMake configuration from project folder
```
cmake -B build/
```

3. Compile the project in build folder
```
make
```

4. Start the Docker fleet to establish a working P2P network, all the nodes discovering each others by regularly sending broadcast discovery packets

5. Open two terminals, starting an interactive node on each of those

You can then try the following out:
- Add a file to be uploaded in the mounted "files" folder from the host
- In the first terminal, upload this file to the network (files/my_file_name), it will be replicated so that up to K peers own it, and those K peers will also be able to tell anyone that contacts them all the owners of the file (themselves and the others)
- The magnet link containing information about the file is generated in upload/ folder and also printed to the terminal, copy this and paste into a new file in the "files" folder
- In the second terminal, download the file from the network (files/my_torrent_magnet), the client will automatically traverse the network by getting closest peers to the file, until eventually finding someone that knows the owners of the file, it will then try downloading the file using HTTP
- Now, you can retry doing this, but stop the first client before downloading from the second. Because of the automatic P2P replication, the file is still available from many other peers.