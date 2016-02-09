#!/usr/bin/env sh

#pwd

SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "${SCRIPT}")

CMDARGS="${CMDARGS} -D"
CMDARGS="${CMDARGS} --conf-subdir .dsgui"
CMDARGS="${CMDARGS} --debug-verbosity 2"
CMDARGS="${CMDARGS} --log-verbosity 2"

APP_BINARY_NAME="/../../datovka"
ATTACH_LOAD_PATH="${SCRIPTPATH}/attachment"

sh ${SCRIPTPATH}/login.sh
sh ${SCRIPTPATH}/sendmsg.sh
sh ${SCRIPTPATH}/getmsg.sh
sh ${SCRIPTPATH}/find.sh

echo ""
echo ""
echo "-------------------------------------------------"
echo "CONGRATULATION: All tests were done with success."
echo "-------------------------------------------------"
echo ""
