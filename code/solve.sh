#!/bin/bash
set -e

trap "exit" INT TERM
trap "kill 0" EXIT


MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
NOW=`date -u +'%Y%m%d%H%M%S'`
LOGDIR="$MYDIR/logs/$NOW"

GAMEMAP="${1:-$MYDIR/../spec/maps/sample/sample1.map}"
MAPNAME=`basename "$GAMEMAP"`
MAPNAME="${MAPNAME%.map}"

TIMEOUT="${TIMEOUT:-150}"
LOGDIR="$LOGDIR-$MAPNAME"

TARGET_DIR="${TARGET_DIR:-build/release}"

mkdir -p "$LOGDIR"

cat "$GAMEMAP" | tee "$LOGDIR/map.txt"
echo ''

set +e
PROG=`cat "$GAMEMAP" | PAIV_TIMEOUT="$TIMEOUT" timeout -2 "$TIMEOUT" "$TARGET_DIR/lifter" 2> "$LOGDIR/stderr.log" `
set -e

# exec 3< <(cat "$GAMEMAP" | "$TARGET_DIR/lifter" 2> "$LOGDIR/stderr.log")
# pid=$!
# echo "pid: $pid"
# (sleep $TIMEOUT && kill -2 $pid) 2>/dev/null & watcher=$!
# wait $pid 2>/dev/null && pkill -HUP -P $watcher
# PROG=$(cat <&3)

{ cat "$GAMEMAP"; echo ''; echo "$PROG"; } | "$TARGET_DIR/validate" 2>> "$LOGDIR/stderr.log" 1> "$LOGDIR/solution.txt"

cat "$LOGDIR/solution.txt" || cat "$LOGDIR/stderr.log"
echo ''
echo "${LOGDIR#$MYDIR/}"
