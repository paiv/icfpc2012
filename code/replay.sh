#!/bin/bash
set -e

MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

LOGDIR="$1"
[ ! -d "$LOGDIR" ] && echo 'usage: ./replay.sh logdir' && exit 1

GAMEMAP="$LOGDIR/map.txt"
PROG=`tail -n 1 "$LOGDIR/solution.txt"`

TARGET_DIR="${TARGET_DIR:-build/release}"
DELAY="${DELAY:-300}"

{ cat "$GAMEMAP"; echo ''; echo "$PROG"; } | "$TARGET_DIR/viz" --delay "$DELAY"
