#!/bin/sh
set -eu

{
    printf '%d\n' $#
    for v in "$@"; do
        printf '%d\n%s' ${#v} "$v"
    done
    printf '\n'
    cat
} | nc -NU "${PYSERVER_SOCKET-./.py.sock}"
