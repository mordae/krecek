#!/usr/bin/env python3
import socket

PORT = 13571
BROADCAST = "<broadcast>"

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

# Listen on all interfaces
sock.bind(("", PORT))

print("RF Broadcast Server started")
print(f"Listening on UDP {PORT}, rebroadcasting to LAN")

while True:
    data, addr = sock.recvfrom(2048)

    print(f"[RX from {addr}] {data.hex()}")

    # rebroadcast to whole LAN
    sock.sendto(data, (BROADCAST, PORT))

