#!/usr/bin/env sh

SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "${SCRIPT}")

BINARY="rm_test_app"
URL=""
TOKEN=""

"${SCRIPTPATH}/${BINARY}" --base-url "${URL}" --token "${TOKEN}" --ignore-ssl-errors
