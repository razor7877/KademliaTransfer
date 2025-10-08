import socket
import struct
import hashlib
import sys

# Server address
SERVER_IP = "127.0.0.1"
SERVER_PORT = 8182

# RPC constants
PING = 1
STORE = 2
FIND_NODE = 4
RPC_MAGIC = b"KDMT"

HASH_SIZE = 32  # SHA256
RPC_HEADER_FORMAT = "<4sii"  # magic, packet_size, call_type
RPC_HEADER_SIZE = struct.calcsize(RPC_HEADER_FORMAT)

# struct RPCFind { RPCMessageHeader header; HashID key; }
RPC_FIND_FORMAT = RPC_HEADER_FORMAT + f"{HASH_SIZE}s"
RPC_FIND_SIZE = struct.calcsize(RPC_FIND_FORMAT)

def sha256_file(filepath: str) -> bytes:
    h = hashlib.sha256()
    with open(filepath, "rb") as f:
        while chunk := f.read(4096):
            h.update(chunk)
    return h.digest()

def create_find_node_packet(filepath: str) -> bytes:
    key_hash = sha256_file(filepath)

    packet_size = RPC_FIND_SIZE
    header = struct.pack(RPC_HEADER_FORMAT, RPC_MAGIC, packet_size, FIND_NODE)
    packet = header + key_hash
    return packet

def send_rpc_request(data: bytes):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect((SERVER_IP, SERVER_PORT))
        sock.sendall(data)
        print(f"Sent {len(data)} bytes to {SERVER_IP}:{SERVER_PORT}")

        # Try receiving a response
        try:
            response = sock.recv(1024)
            if response:
                print(f"Received {len(response)} bytes: {response}")
            else:
                print("Connection closed by server.")
        except socket.timeout:
            print("No response received.")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <file_for_key>")
        sys.exit(1)

    filepath = sys.argv[1]
    packet = create_find_node_packet(filepath)
    print(f"Sending FIND_NODE RPC ({len(packet)} bytes)")
    send_rpc_request(packet)
