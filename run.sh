#!/usr/bin/env bash

set -e

make

use_gdb=false
if egrep -q "^ *gl_debug_break += +true" grapro.cfg; then
    use_gdb=true
    echo "use gdb"
fi

config_file="grapro.cfg"
if [[ -n "$1" ]]; then
    config_file="$1"
fi
echo "Using config file: ${config_file}"

if [[ "$(hostname)" == "pkl03" ]]; then
    if [[ "$use_gdb" = true ]]; then
        gdb -ex=r --args primusrun ./bin/grapro -c "${config_file}"
    else
        primusrun ./bin/grapro -c "${config_file}"
    fi
else
    if [[ "$use_gdb" = true ]]; then
        gdb -ex=r --args ./bin/grapro -c "${config_file}"
    else
        ./bin/grapro -c "${config_file}"
    fi
fi
