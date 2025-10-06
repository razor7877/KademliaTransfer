import socket
import struct

# Server address
SERVER_IP = "127.0.0.1"
SERVER_PORT = 8182

PING = 1
STORE = 2
FIND_NODE = 4
FIND_VALUE = 8
PING_RESPONSE = 16
STORE_RESPONSE = 32
FIND_NODE_RESPONSE = 64
FIND_VALUE_RESPONSE = 128

# struct RPCMessageHeader {
#   char magic_number[4];
#   int packet_size;
#   enum RPCCallType call_type;
# }

#  - 4s  : 4-byte magic number
#  - i   : 4-byte int (packet_size)
#  - i   : 4-byte enum (call_type)
RPC_HEADER_FORMAT = "<4sii"

def create_ping_packet():
    magic = b"KDMT"
    call_type = PING
    packet_size = struct.calcsize(RPC_HEADER_FORMAT)

    header = struct.pack(RPC_HEADER_FORMAT, magic, packet_size, call_type)
    return header

def send_rpc_request(data: bytes):
    """Send TCP packet to the client"""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect((SERVER_IP, SERVER_PORT))
        sock.sendall(data)
        print(f"Sent {len(data)} bytes to {SERVER_IP}:{SERVER_PORT}")

        # Get response from peer
        try:
            response = sock.recv(1024)
            if response:
                print(f"Received {len(response)} bytes: {response}")
            else:
                print("Connection closed by server.")
        except socket.timeout:
            print("No response received.")

if __name__ == "__main__":
    packet = create_ping_packet()
    print(f"Sending PING RPC ({len(packet)} bytes): {packet}")
    send_rpc_request(packet)
