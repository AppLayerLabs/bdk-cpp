# Copyright (c) [2023-2024] [AppLayer Developers]
# This software is distributed under the MIT License.
# See the LICENSE.txt file in the project root for more information.

# Start from a base Debian image
FROM debian:trixie

# Set shell to Bash because Docker standards are stupid
SHELL ["/bin/bash", "-c"]

# Update the system
RUN apt-get update && apt-get upgrade -y

# Set the working directory in the Docker container
WORKDIR /bdk-cpp

# Copy the local folder to the container
COPY . /bdk-cpp

# Install Docker-specific dependencies
RUN apt-get -y install nano vim unison curl jq unzip

# Install dependencies
RUN bash /bdk-cpp/scripts/deps.sh --install

# Create the synchronized directory
RUN mkdir /bdk-volume

# Copy Unison configuration file
COPY sync.prf /root/.unison/sync.prf

# Start Unison in the background, ignoring files that should not be synced
CMD nohup unison -repeat 1 /bdk-volume /bdk-cpp -auto -batch \
    -ignore 'Name {build}' \
    -ignore 'Name {build_local_testnet}' \
    -ignore 'Name {.vscode}' \
    -ignore 'Name {proto/metrics.pb.cc}' \
    -ignore 'Name {proto/metrics.pb.h}' \
    -ignore 'Name {proto/vm.grpc.pb.cc}' \
    -ignore 'Name {proto/vm.grpc.pb.h}' \
    -ignore 'Name {proto/vm.pb.cc}' \
    -ignore 'Name {proto/vm.pb.h}' \
    -ignore 'Name {storageVM}' \
    -ignore 'Name {info.txt}' \
    -ignore 'Name {.vscode}' \
    -ignore 'Name {vmInfo.txt}' \
    -ignore 'Name {*.[pP][bB].[hH]}' \
    -ignore 'Name {tests/node_modules}' \
    -ignore 'Name {depends/x86_64-pc-linux-gnu}' \
    -ignore 'Name {scripts/AIO-setup.log}' \
    -ignore 'Name {compile_commands.json}' \
    -ignore 'Name {.cache}' \
    -ignore 'Name {Dockerfile}' \
    -ignore 'Name {docker-compose.yml}' \
    -ignore 'Name {sync.prf}' \
    -ignore 'Name {kateproject}' \
    -ignore 'Name {*.o}' \
    -ignore 'Name {*.gch}' \
    > /dev/null 2>&1 & \
    /bin/bash
