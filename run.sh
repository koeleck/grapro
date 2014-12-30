#!/bin/sh

set -e

make

if [[ "$(hostname)" == "pkl03" ]]; then
    primusrun ./bin/grapro -c grapro.cfg
else
    ./bin/grapro -c grapro.cfg
fi
