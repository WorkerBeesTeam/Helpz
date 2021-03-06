#!/bin/bash

LOCKFILE="/tmp/`basename $0`.lock"

[ -f $LOCKFILE ] && exit 0
touch $LOCKFILE

FIND_PATH=$1
if [ -z "$1" ]; then
    FIND_PATH=..
elif [ "${FIND_PATH: -1}" = "/" ]; then
    FIND_PATH=${FIND_PATH::-1}
fi

PATH_TO=$2
if [ -z "$2" ]; then
    PATH_TO=.
fi

[ -d $PATH_TO/Helpz ] || mkdir -p $PATH_TO/Helpz
rm -f $PATH_TO/Helpz/* || true
find $FIND_PATH -name "*.h" ! -path "$FIND_PATH/include/Helpz/*" ! -path "$FIND_PATH/DTLS/test/*" -exec ln -s ../{} $PATH_TO/Helpz/ \;

rm -f $LOCKFILE

exit 0
