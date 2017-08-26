#!/bin/bash
set -e

MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

LOGDIR="$MYDIR/logs"

export TIMEOUT=150


function solve {
    GAMEMAP="$1"
    echo "$GAMEMAP"
    LDIR=`$MYDIR/solve.sh "$GAMEMAP" | tail -1`
    [ -d "$LDIR" ] && "$MYDIR/animate.sh" "$LDIR"
}


for i in $(seq 4 10); do
    solve "../spec/maps/sample/contest$i.map"
done


"$MYDIR/stats.sh" "$LOGDIR" > "$LOGDIR/summary.txt"
