#!/usr/bin/env python3
import socket

PORT = 13570

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(("", PORT))

clients = set()

print("RF Routing Server running")
print(f"Listening on UDP port {PORT}")

while True:
    data, addr = sock.recvfrom(2048)

    # add client
    clients.add(addr)

    print(f"[RX from {addr}] {data.hex()}")

    # send to all other clients
    for c in clients:
        if c != addr:
            sock.sendto(data, c)

