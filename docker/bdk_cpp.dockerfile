# Copyright (c) [2023-2024] [AppLayer Developers]
# This software is distributed under the MIT License.
# See the LICENSE.txt file in the project root for more information.

# Start from a base Debian image
FROM debian:trixie

# Update the system
RUN apt-get update && apt-get upgrade -y

# Install Docker-specific dependencies
RUN apt-get -y install nano vim unison curl jq unzip gcovr

# Copy the deps script to the container
COPY scripts/deps.sh /

# Install dependencies
RUN bash ./deps.sh --install

# Create a directory for sonarcloud
RUN mkdir /root/.sonar

# Copy sonarcloud scripts to sonarcloud
COPY scripts/sonarcloud.sh /sonarcloud

# Copy Unison configuration file
COPY sync.prf /root/.unison/sync.prf

# Copy the entrypoint script
COPY docker/entrypoint.sh /entrypoint.sh

# Copy the entrypoint script
COPY scripts/sonarcloud.sh /sonarcloud.sh

# Execute sonarcloud install script
RUN /sonarcloud.sh

# Update running paths
ENV PATH=/root/.sonar/build-wrapper-linux-x86:$PATH
ENV PATH=/root/.sonar/sonar-scanner-6.1.0.4477-linux-x64/bin:$PATH

# Copy the entrypoint script
COPY docker/entrypoint.sh /entrypoint.sh

