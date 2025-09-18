# Use Ubuntu as the base image
FROM ubuntu:22.04

# Install build tools and dependencies
RUN apt-get update && apt-get install -y gcc make build-essential

# Set the working directory
WORKDIR /app

# Copy all project files into the container
COPY . /app

# Build the server using the same command as build.bat
RUN gcc -g src/network.c src/data.c src/server.c src/lib/cjson/cJSON.c src/lib/simulation/sim_engine.c src/lib/simulation/sim_algorithms.c -o server.exe -lm

# Expose the port used by the server (14141 by default, adjust if needed)
EXPOSE 14141

# Set the default command to run the server with fake stdin (prevents immediate exit)
CMD ["sh", "-c", "tail -f /dev/null | ./server.exe"]