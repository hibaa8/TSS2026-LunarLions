import time
import socket
import struct
import sys

# Simulated Positions (latitude, longitude, heading)
pairs = [(-10033.67, -5673.19, 0),
(-10032.39, -5673.09, 5),
(-10029.74, -5672.30, 10),
(-10027.78, -5672.11, 15),
(-10025.82, -5671.91, 20),
(-10023.89, -5671.71, 25),
(-10021.89, -5671.71, 30),
(-10018.06, -5669.16, 35),
(-10015.51, -5668.97, 40),
(-10014.83, -5668.42, 45),
(-10013.85, -5668.38, 50),
(-10012.83, -5667.21, 55),
(-10010.98, -5666.55, 60),
(-10008.94, -5667.30, 65),
(-10006.58, -5666.91, 70),
(-10004.26, -5665.53, 75),
(-10002.26, -5665.53, 80),
(-10000.99, -5662.20, 85),
(-10000.11, -5661.12, 90),
(-9999.32, -5695.55, 95),
(-9999.32, -5695.55, 100),
 (-10000.11, -5661.12, 95),
 (-10000.99, -5662.20, 90),
 (-10002.26, -5665.53, 85),
 (-10004.26, -5665.53, 80),
 (-10006.58, -5666.91, 75),
 (-10008.94, -5667.30, 70),
 (-10010.98, -5666.55, 65),
 (-10012.83, -5667.21, 60),
 (-10013.85, -5668.38, 55),
 (-10014.83, -5668.42, 50),
 (-10015.51, -5668.97, 45),
 (-10018.06, -5669.16, 40),
 (-10021.89, -5671.71, 35),
 (-10023.89, -5671.71, 30),
 (-10025.82, -5671.91, 25),
 (-10027.78, -5672.11, 20),
 (-10029.74, -5672.30, 15),
 (-10032.39, -5673.09, 10),
 (-10033.67, -5673.19, 5)]

# UDP command mappings
ROVER_POS_X = 1111
ROVER_POS_Y = 1112
ROVER_HEADING = 1114
EVA1_POS_X = 2017
EVA1_POS_Y = 2018
EVA1_HEADING = 2019
EVA2_POS_X = 2020
EVA2_POS_Y = 2021
EVA2_HEADING = 2022

server_ip = sys.argv[1]
server_port = 14141

def send_udp_command(sock, server_addr, command, value):
    """Send a UDP command with a float value"""
    # UDP packet format: [timestamp:4 bytes][command:4 bytes][value:4 bytes]
    timestamp = int(time.time())
    packet = struct.pack('>I', timestamp) + struct.pack('>I', command) + struct.pack('>f', value)
    sock.sendto(packet, server_addr)


def simulate_pos():
    # Create UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    server_addr = (server_ip, server_port)

    i = 0
    while True:
        lat = pairs[i][0]  # Latitude (Y)
        lon = pairs[i][1]  # Longitude (X)
        heading = pairs[i][2]  # Heading

        print(f"Position: lat={lat}, lon={lon}, heading={heading}")

        # Send rover position
        send_udp_command(sock, server_addr, ROVER_POS_X, lon)
        send_udp_command(sock, server_addr, ROVER_POS_Y, lat)
        send_udp_command(sock, server_addr, ROVER_HEADING, heading)

        # Send EVA1 position
        send_udp_command(sock, server_addr, EVA1_POS_X, lon)
        send_udp_command(sock, server_addr, EVA1_POS_Y, lat)
        send_udp_command(sock, server_addr, EVA1_HEADING, heading)

        # Send EVA2 position
        send_udp_command(sock, server_addr, EVA2_POS_X, lon)
        send_udp_command(sock, server_addr, EVA2_POS_Y, lat)
        send_udp_command(sock, server_addr, EVA2_HEADING, heading)

        # Go back to start once at the end
        i = (i + 1) % len(pairs)

        # Wait before next update
        time.sleep(0.50)

# Run script
if __name__ == "__main__":
    simulate_pos()
