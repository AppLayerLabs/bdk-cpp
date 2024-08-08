#!/usr/bin/env bash

OS=linux
ARCH="x64"
TMP_PATH=/tmp
INSTALL_PATH=/root/.sonar
VERIFY_CORRECTNESS=false

check_status() {
    exit_status=$?
    if [ $exit_status -ne 0 ]; then
        echo "ERROR $1"
        exit $exit_status
    fi
}

realpath() {
    readlink -f "$1"
}

parse_args() {
    while getopts "hv" arg; do
        case $arg in
            x) set -x
               ;;
            v) VERIFY_CORRECTNESS=true
               echo "Verify correctness is set to true"
               ;;
            ?) exit 0 ;;
        esac
    done
}

config_sonar_path() {
    echo "Installation path is '${INSTALL_PATH}'"

    test ! -z "${INSTALL_PATH}"
    check_status "Empty installation path specified"

    if [[ ! -e "${INSTALL_PATH}" ]]; then
        mkdir -p "${INSTALL_PATH}"
        check_status "Failed to create non-existing installation path '${INSTALL_PATH}'"
    fi

    ABSOLUTE_INSTALL_PATH=$(realpath "${INSTALL_PATH}")
    echo "Absolute installation path is '${ABSOLUTE_INSTALL_PATH}'"

    test -d "${INSTALL_PATH}"
    check_status "Installation path '${INSTALL_PATH}' is not a directory (absolute path is '${ABSOLUTE_INSTALL_PATH}')"

    test -r "${INSTALL_PATH}"
    check_status "Installation path '${INSTALL_PATH}' is not readable (absolute path is '${ABSOLUTE_INSTALL_PATH}')"

    test -w "${INSTALL_PATH}"
    check_status "Installation path '${INSTALL_PATH}' is not writeable (absolute path is '${ABSOLUTE_INSTALL_PATH}')"
}

set_sonar_vars() {
    SONAR_HOST_URL=${SONAR_HOST_URL:-https://sonarcloud.io}
    SONAR_SCANNER_NAME="sonar-scanner"
    SONAR_SCANNER_SUFFIX="linux-x64"
    SONAR_SCANNER_VERSION=$(curl -sSL -H "Accept: application/vnd.github+json" \
                                 https://api.github.com/repos/SonarSource/sonar-scanner-cli/releases/latest | jq -r '.tag_name')
    check_status "Failed to fetch latest sonar-scanner version from GitHub API"
    SONAR_SCANNER_DIR="${INSTALL_PATH}/sonar-scanner-${SONAR_SCANNER_VERSION}-${SONAR_SCANNER_SUFFIX}"
    SONAR_SCANNER_URL="https://binaries.sonarsource.com/Distribution/sonar-scanner-cli/sonar-scanner-cli-${SONAR_SCANNER_VERSION}-${OS}-${ARCH}.zip"
    check_status "Failed to download ${OS} ${ARCH} sonar-scanner checksum from '${SONAR_SCANNER_URL}'"
    BUILD_WRAPPER_SUFFIX="linux-x86"
    BUILD_WRAPPER_NAME="build-wrapper-linux-x86-64"
    BUILD_WRAPPER_DIR="${INSTALL_PATH}/build-wrapper-${BUILD_WRAPPER_SUFFIX}"
    BUILD_WRAPPER_URL=${SONAR_HOST_URL}/static/cpp/build-wrapper-${BUILD_WRAPPER_SUFFIX}.zip

    echo "sonar-scanner-version=${SONAR_SCANNER_VERSION}"
    echo "sonar-scanner-url-${OS}-${ARCH}=${SONAR_SCANNER_URL}"
    echo "sonar-scanner-dir=${SONAR_SCANNER_DIR}"
    echo "sonar-scanner-bin=${SONAR_SCANNER_DIR}/bin/${SONAR_SCANNER_NAME}"
    echo "build-wrapper-url=${SONAR_HOST_URL}/static/cpp/build-wrapper-${BUILD_WRAPPER_SUFFIX}.zip"
    echo "build-wrapper-dir=${BUILD_WRAPPER_DIR}"
    echo "build-wrapper-bin=${BUILD_WRAPPER_DIR}/${BUILD_WRAPPER_NAME}"
}

fetch_sonar() {
    echo "Downloading '${SONAR_SCANNER_URL}'"
    curl -sSLo "${TMP_PATH}/sonar-scanner.zip" "${SONAR_SCANNER_URL}"
    check_status "Failed to download '${SONAR_SCANNER_URL}'"
    echo "Downloading '${BUILD_WRAPPER_URL}'"
    curl -sSLo "${TMP_PATH}/build-wrapper-linux-x86.zip" "${BUILD_WRAPPER_URL}"
    check_status "Failed to download '${BUILD_WRAPPER_URL}'"
}

decompress_sonar() {
    echo "Decompressing"
    unzip -o -d "${INSTALL_PATH}" "${TMP_PATH}/sonar-scanner.zip"
    check_status "Failed to unzip the archive into '${INSTALL_PATH}'"
    unzip -o -d "${INSTALL_PATH}" "${TMP_PATH}/build-wrapper-linux-x86.zip"
    check_status "Failed to unzip the archive into '${INSTALL_PATH}'"
}

main() {
    parse_args "$@"
    set_sonar_vars
    config_sonar_path
    fetch_sonar
    decompress_sonar
}

main "$@"
