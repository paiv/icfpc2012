#!/bin/bash
set -e

MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

TEAMID=${TEAMID:-"TEAMID"}
TARGET="icfp-$TEAMID.tgz"

if [ -f "$TARGET" ]; then
    rm "$TARGET"
fi

tar -czf "$TARGET" \
    install \
    PACKAGES \
    README \
    -C "$MYDIR/.." \
    Makefile \
    src/

md5 "$TARGET"
