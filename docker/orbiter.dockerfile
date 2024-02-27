# Copyright (c) [2023-2024] [Sparq Network]
# This software is distributed under the MIT License.
# See the LICENSE.txt file in the project root for more information.

# Start from a base Debian image
FROM debian:bookworm

# Update the system
RUN apt-get update && apt-get upgrade -y

# Install dependencies
RUN apt-get install -y libasan8
