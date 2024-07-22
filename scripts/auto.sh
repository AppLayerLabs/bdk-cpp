#!/usr/bin/env bash

# debug
# set -x

# set working directory
_AUTO_DIR=$(dirname ${0})

# load modules
. ${_AUTO_DIR}/auto_defines.sh
. ${_AUTO_DIR}/auto_compose.sh
. ${_AUTO_DIR}/auto_actions.sh

# logs, printf wrapper
log ()
{
    printf "$@";
}

# error
log_error ()
{
    printf $_RED; log "$@"; printf $_RESET;
}

# success
log_ok ()
{
    printf $_GREEN; log "$@"; printf $_RESET;
}

# fatal, die: logs error and exit
die ()
{
    log_error "[-] Fatal: $@"; printf $_RESET; exit 1;
}

check_action ()
{
    # any function named as _NAME_action can be executed directly as an action
    ( command -v _${_ACTION}_action 2>&1 > /dev/null ) && return 0

    die "invalid action! Use: $0 help\n"
}

# parse user options
parse_opts ()
{
    # get options
    for opt in "$@"; do
        case $opt in
            # enable debugging
            -x) shift 1; set -x ;;

            # enable verbose
            -v) shift 1; set -v ;;

            # enable errexit
            -e) shift 1; set -e ;;

            # set compose file
            -f) shift 1; _COMPOSE_FILE=$(echo "${1}" | tr , ' '); shift 1 ;;

            # selected services
            -s) shift 1; _COMPOSE_SERVICE=$1; shift ;;
        esac
    done

    # set action
    _ACTION=${1} ; shift ;

    # update compose services variable
    _COMPOSE_SERVICE=$(echo "${_COMPOSE_SERVICE}" | tr , ' ')

    # additional params with will be appended to the action function parameters
    _PARAMS="${@}"
}

# select an action and execute it associated function
handle_action ()
{
    eval _${_ACTION}_action $_PARAMS
}

# main entry point
main ()
{
    # get/set user options (command-line)
    parse_opts "$@"

    # verify action argument
    check_action

    # logs and continue
    log "Params: $(printf '%s' "${_PARAMS[@]}")\n"

    # select build related actions and handle it
    handle_action ${_PARAMS[@]}
}

# main routine
main $@
