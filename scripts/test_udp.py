import socket
import struct
import time

# Configuration
UDP_IP = "172.20.182.43"  # Server IP address
UDP_PORT = 14141          # Server port

# Create UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

try:
    # Example 1: Fetch EVA telemetry (command 0)
    print("=" * 50)
    print("Test 1: Fetch EVA telemetry (command 0)")
    print("=" * 50)

    timestamp = int(time.time())  # Current UNIX timestamp
    command = 0  # EVA.json

    # Pack in big endian format: timestamp (uint32) + command (uint32)
    packet = struct.pack('>II', timestamp, command)

    # Break down packet bytes
    timestamp_bytes = struct.pack('>I', timestamp)
    command_bytes = struct.pack('>I', command)

    print(f"Timestamp: {timestamp} -> bytes: {timestamp_bytes.hex()}")
    print(f"Command: {command} -> bytes: {command_bytes.hex()}")
    print(f"Full packet bytes: {packet.hex()}")
    print(f"Sending packet to {UDP_IP}:{UDP_PORT}")
    sock.sendto(packet, (UDP_IP, UDP_PORT))

    # Wait for response with timeout
    sock.settimeout(5.0)
    data, addr = sock.recvfrom(1024)
    print(f"Received {len(data)} bytes from {addr}")
    print(f"Response: {data.hex()}")

except socket.timeout:
    print("No response received (timeout)")
except Exception as e:
    print(f"Error: {e}")

try:
    # Example 2: Fetch ROVER telemetry (command 1)
    print("\n" + "=" * 50)
    print("Test 2: Fetch ROVER telemetry (command 1)")
    print("=" * 50)

    timestamp = int(time.time())
    command = 1  # ROVER.json

    packet = struct.pack('>II', timestamp, command)

    print(f"Timestamp: {timestamp}")
    print(f"Command: {command}")
    print(f"Sending packet to {UDP_IP}:{UDP_PORT}")
    sock.sendto(packet, (UDP_IP, UDP_PORT))

    sock.settimeout(5.0)
    data, addr = sock.recvfrom(4000)
    print(f"Received {len(data)} bytes from {addr}")
    print(f"Response: {data.hex()}")

except socket.timeout:
    print("No response received (timeout)")
except Exception as e:
    print(f"Error: {e}")

try:
    # Example 3: Control rover throttle (command 1109 with float value)
    print("\n" + "=" * 50)
    print("Test 3: Control rover throttle (command 1109)")
    print("=" * 50)

    timestamp = int(time.time())
    command = 1109  # Throttle command
    throttle_value = 50.0  # 50% throttle

    # Pack: timestamp (uint32) + command (uint32) + throttle value (float)
    packet = struct.pack('>IIf', timestamp, command, throttle_value)

    print(f"Timestamp: {timestamp}")
    print(f"Command: {command} (Throttle)")
    print(f"Value: {throttle_value}")
    print(f"Sending packet to {UDP_IP}:{UDP_PORT}")
    sock.sendto(packet, (UDP_IP, UDP_PORT))

    sock.settimeout(5.0)
    data, addr = sock.recvfrom(1024)
    print(f"Received {len(data)} bytes from {addr}")
    print(f"Response: {data.hex()}")

except socket.timeout:
    print("No response received (timeout)")
except Exception as e:
    print(f"Error: {e}")
finally:
    sock.close()