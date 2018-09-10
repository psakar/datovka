#!/usr/bin/env bash

#pwd

SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "${SCRIPT}")

bash ${SCRIPTPATH}/login.sh
bash ${SCRIPTPATH}/sendmsg.sh
bash ${SCRIPTPATH}/getmsg.sh
bash ${SCRIPTPATH}/find.sh

echo ""
echo ""
echo "-------------------------------------------------"
echo "CONGRATULATION: All tests were done with success."
echo "-------------------------------------------------"
echo ""
