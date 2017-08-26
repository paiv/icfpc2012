#!/bin/bash
set -e

MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

LOGDIR="${1:-$MYDIR/logs}"

function dump {
    GAME="$1"
    [ -f "$GAME/solution.txt" ] && {
        SCORE=`head -1 "$GAME/solution.txt"`
        MAPNAME=`basename "$GAME" | cut -d '-' -f 2`

        MAPURL="${GAME#$LOGDIR/}/map.txt"
        SOLURL="${GAME#$LOGDIR/}/solution.txt"

        echo "* [$MAPNAME]($MAPURL): [$SCORE points]($SOLURL)"
    }
}


function get_stats {
    LDIR="$1"
    for d in $LDIR/*; do
        dump "$d"
    done
}

get_stats "$LOGDIR"
