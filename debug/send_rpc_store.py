import socket
import struct
import hashlib
import sys

# Server address
SERVER_IP = "127.0.0.1"
SERVER_PORT = 8182

PING = 1
STORE = 2
RPC_MAGIC = b"KDMT"

# Sizes
K_VALUE = 2           # match your C definition
HASH_SIZE = 32        # SHA-256
PEER_STRUCT_SIZE = 80  # placeholder: adjust if needed
# struct RPCMessageHeader { 4s + int + int }
RPC_HEADER_FORMAT = "<4sii"
RPC_HEADER_SIZE = struct.calcsize(RPC_HEADER_FORMAT)

# struct RPCKeyValue { HashID key[32]; size_t num_values; RPCPeer values[K_VALUE] }
RPC_KEYVALUE_FORMAT = f"{HASH_SIZE}sQ{K_VALUE * PEER_STRUCT_SIZE}s"
RPC_KEYVALUE_SIZE = struct.calcsize(RPC_KEYVALUE_FORMAT)

# struct RPCStore { RPCMessageHeader header; RPCKeyValue key_value; }
RPC_STORE_FORMAT = RPC_HEADER_FORMAT + RPC_KEYVALUE_FORMAT
RPC_STORE_SIZE = struct.calcsize(RPC_STORE_FORMAT)

def sha256_file(filepath: str) -> bytes:
    h = hashlib.sha256()
    with open(filepath, "rb") as f:
        while chunk := f.read(4096):
            h.update(chunk)
    return h.digest()


def create_store_packet(filepath: str) -> bytes:
    key_hash = sha256_file(filepath)
    num_values = 0  # no peers yet
    values = bytes(K_VALUE * PEER_STRUCT_SIZE)  # empty peer slots

    key_value = struct.pack(RPC_KEYVALUE_FORMAT, key_hash, num_values, values)

    packet_size = RPC_HEADER_SIZE + RPC_KEYVALUE_SIZE
    header = struct.pack(RPC_HEADER_FORMAT, RPC_MAGIC, packet_size, STORE)

    return header + key_value


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
        print(f"Usage: {sys.argv[0]} <file_to_store>")
        sys.exit(1)

    filepath = sys.argv[1]
    packet = create_store_packet(filepath)
    print(f"Sending STORE RPC ({len(packet)} bytes)")
    send_rpc_request(packet)
