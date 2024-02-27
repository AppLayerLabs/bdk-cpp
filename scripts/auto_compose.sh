#!/usr/bin/env bash

_join()
{
    local separator=$1 ; shift ;
    local compose_file_opt=""

    for file in $@; do
        compose_file_opt="${compose_file_opt}${separator}${file}"
    done
    echo ${compose_file_opt}
}

_docker_tag_exists()
{
    local _tag=$1
    local _result=$(docker images $_tag --format "{{.Repository}}/{{.Tag}}: {{.ID}}")

    return `test -n "${_result}"`
}

_compose_action ()
{
    local _action=$1 ; shift;
    local _params=$@
    local _compose_files_opt=$(_join ' -f ' ${_COMPOSE_FILE})

    docker-compose ${_compose_files_opt} ${_action} ${_params} ${_COMPOSE_SERVICE}
}

_compose_exec ()
{
    local _service=${1} ; shift;
    local _compose_files_opt=$(_join ' -f ' ${_COMPOSE_FILE})

    docker-compose ${_compose_files_opt} \
                    exec ${_service} $@
}

_compose_root_exec ()
{
    local _service=${1} ; shift;
    local _compose_files_opt=$(_join ' -f ' ${_COMPOSE_FILE})

    docker-compose ${_compose_files_opt} \
                    exec -u root ${_service} $@
}

_compose_exec_test()
{
    local _service=$1
    local _directory=$2

    _compose_exec $_service clojure -M:env/test:run/test -d $_directory
}

_compose_config ()
{
    local _option=$1; shift;

    _compose_action config ${_option}
}
