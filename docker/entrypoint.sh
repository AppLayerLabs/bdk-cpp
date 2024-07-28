#!/bin/sh

# Start Unison in the background, ignoring files that should not be synced
nohup unison -repeat 1 /bdk-volume /bdk-cpp -auto -batch \
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
> /dev/null 2>&1 & /bin/bash
