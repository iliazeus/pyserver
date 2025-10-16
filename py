#!/bin/sh
set -eu

if [ -z "${1-}" -o "${1-}" == "-h" ]; then
    mode="-h"
elif [ "$1" == "-c" -o "$1" == "-C" ]; then
    mode="$1"
    command="${2?}"
    shift 2
elif [ "$1" == "-p" -o "$1" == "-i" ]; then
    mode="$1"
    shift
else
    mode=""
fi

if [ "$mode" == "-h" ]; then
    echo "py - pysession client (https://github.com/iliazeus/pysession)"
    echo "Usage: py -c <code> [...args]  - run code"
    echo "       py -C <code> [...args]  - run code with stdin"
    echo "       py -p [...args]         - print args"
    echo "       py -i [...args]         - interactive mode"
    echo "       py | py -h              - this text"
    exit 0
fi

send_args() {
    printf '%d\n%s' ${#mode} "$mode"
    printf '%d\n' $#
    for v in "$@"; do
        printf '%d\n%s' ${#v} "$v"
    done
    printf '\n'
    if [ "$mode" == "-c" ]; then
        printf '%s\n' "$command"
    elif [ "$mode" == "-C" ]; then
        printf '%s\n' "$command"
        cat
    elif [ "$mode" == "-p" ]; then
        printf 'print(*(eval(x) for x in argv[1:]))\n'
    else
        cat
    fi
}

connect() {
    nc -NU "${PYSERVER_SOCKET-./.py.sock}"
}

receive_status() {
    if [ "$mode" == "-i" ]; then
        cat
    else
        while IFS= read -r line; do
            if [ -n "${lastline+x}" ]; then
                printf '%s\n' "$lastline"
            fi
            lastline="$line"
        done
        exit "$line"
    fi
}

send_args "$@" | connect | receive_status
