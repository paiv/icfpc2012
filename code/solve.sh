#!/bin/bash
set -e

MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

GAMEMAP="${1:-$MYDIR/../spec/maps/sample/sample1.map}"
TARGET_DIR="${TARGET_DIR:-build/release}"

PROG=`cat "$GAMEMAP" | "$TARGET_DIR/lifter"`

{ cat "$GAMEMAP"; echo ''; echo "$PROG"; } | "$TARGET_DIR/validate"
