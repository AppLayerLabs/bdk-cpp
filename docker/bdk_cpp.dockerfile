# Copyright (c) [2023-2024] [AppLayer Developers]
# This software is distributed under the MIT License.
# See the LICENSE.txt file in the project root for more information.

# Start from a base Debian image
FROM debian:trixie

# Update the system
RUN apt-get update && apt-get upgrade -y

# Install dependencies
RUN apt-get install -y build-essential \
    cmake \
    tmux \
    clang-tidy \
    autoconf \
    libtool \
    pkg-config \
    libabsl-dev \
    libboost-all-dev \
    libc-ares-dev \
    libcrypto++-dev \
    libgrpc-dev \
    libgrpc++-dev \
    librocksdb-dev \
    libscrypt-dev \
    libsnappy-dev \
    libssl-dev \
    zlib1g-dev \
    openssl \
    protobuf-compiler \
    protobuf-compiler-grpc \
    nano \
    vim \
    unison \
    git

# Set the working directory in the Docker container
WORKDIR /bdk-cpp

# Copy the local folder to the container
COPY . /bdk-cpp

# Create the synchronized directory
RUN mkdir /bdk-volume

# Copy Unison configuration file
COPY sync.prf /root/.unison/sync.prf

# Copy the entrypoint script
COPY docker/entrypoint.sh /entrypoint.sh
