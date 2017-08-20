#!/bin/bash
set -e

MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

LOGDIR="$1"
shift

[ ! -d "$LOGDIR" ] && echo 'usage: ./animate.sh logdir' && exit 1

DELAY=0 "$MYDIR/replay.sh" "$LOGDIR" > "$LOGDIR/replay.txt"

VIZ="$MYDIR/../viz/gifit.py"

"$VIZ" $* "$LOGDIR/replay.txt" "$LOGDIR/replay.gif"
