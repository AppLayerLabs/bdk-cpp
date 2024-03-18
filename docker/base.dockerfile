# Copyright (c) [2023-2024] [Sparq Network]
# This software is distributed under the MIT License.
# See the LICENSE.txt file in the project root for more information.

# Start from a base Debian image
FROM debian:bookworm as build

# Update the system
RUN apt-get update && apt-get upgrade -y

# Install dependencies
RUN apt-get install -y libasan8

# FROM gcr.io/distroless/cc
# COPY --from=build /lib/x86_64-linux-gnu/libc.so.6 /lib/x86_64-linux-gnu
# COPY --from=build /lib/x86_64-linux-gnu/libm.so.6 /lib/x86_64-linux-gnu
# COPY --from=build /lib64/ld-linux-x86-64.so.2 /lib64
# COPY --from=build /lib/x86_64-linux-gnu/libgcc_s.so.1 /lib/x86_64-linux-gnu
# COPY --from=build /usr/lib/x86_64-linux-gnu/libstdc++.so.6 /usr/lib/x86_64-linux-gnu
# COPY --from=build /usr/lib/x86_64-linux-gnu/libasan.so.8 /usr/lib/x86_64-linux-gnu

