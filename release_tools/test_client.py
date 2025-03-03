#!/usr/bin/env python3

import socket

# Server details
HOST = "127.0.0.1"  # Change this if your server runs on a different address
PORT = 31415        # Change this if your server listens on a different port

def send_command(command):
    """Send a command to the WSPR server using a TCP socket and return the response."""
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((HOST, PORT))  # Connect to the server
            s.sendall(command.encode() + b"\n")  # Send command with newline

            # Receive the response
            response = s.recv(1024).decode().strip()
            return response
    except ConnectionRefusedError:
        return "Error: Could not connect to the server. Is it running?"
    except Exception as e:
        return f"Error: {e}"

def main():
    print("WSPR Client (type 'exit' to quit)")
    while True:
        command = input("Enter command: ").strip()
        if command.lower() == "exit":
            print("Exiting...")
            break

        response = send_command(command)
        print(f"Response: {response}")

if __name__ == "__main__":
    main()