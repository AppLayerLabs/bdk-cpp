#!/usr/bin/env bash

# _ACTIONS="help,build,start,stop,setup,shell,root,clear"

# colors
_RED=""
_GREEN=""
_RESET=""
_YELLOW=""

# action: default start
_ACTION="help"

# compose variables: file and service
_COMPOSE_FILE=${_COMPOSE_FILE:-"docker-compose.yml"}
_COMPOSE_SERVICE=

set_colors ()
{
    # True if the file whose file descriptor number is
    # file_descriptor is open and is associated with a terminal.
    # test(1)
    test -t 1 && {
        _RED="\033[31m"
        _GREEN="\033[32m"
        _YELLOW="\033[33m"
        _RESET="\033[m"
    }
}

# setup colors variables
set_colors
