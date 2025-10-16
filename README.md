# KademliaTransfer

KademliaTransfer is a lightweight P2P file-sharing client developed in C. It it based on the Kademlia distributed hash table for peer discovery and network traversal, and uses HTTP for file transfers.

# Docker Setup

Build the fleet of nodes:
```
docker-compose build
```

Start the fleet of nodes in background:
```
docker-compose up -d
```

Stop the fleet of nodes and delete them:
```
docker-compose down -v
```

Start a single interactive node
```
docker run -it --rm --network kademliatransfer_kadnet --name manual_node -v $(pwd)/files:/app/files mynode
```