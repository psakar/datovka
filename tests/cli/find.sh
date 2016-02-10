#!/usr/bin/env sh

#pwd

SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "${SCRIPT}")

CMDARGS="${CMDARGS} -D"
CMDARGS="${CMDARGS} --conf-subdir .dsgui"
CMDARGS="${CMDARGS} --debug-verbosity 2"
CMDARGS="${CMDARGS} --log-verbosity 2"

APP_BINARY_NAME="/../../datovka"
. "${SCRIPTPATH}/../../untracked/logins.sh"

echo ""
echo "***********************************************************************"
echo "FIND DATABOX TEST: Find databoxes by ic, firmname, ... ($USERNAME_FIND_DATABOX1)"
echo "***********************************************************************"
# this request must finish with success
"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$USERNAME_FIND_DATABOX1'" \
	--find-databox "dbType='OVM',ic='12345678'" \
	2>/dev/null
if [ 0 != $? ]; then
	echo "FindDatabox: $USERNAME_FIND_DATABOX1 - ERROR"
	exit
else
	echo "FindDatabox: $USERNAME_FIND_DATABOX1 - OK"
fi
echo ""

# this request must finish with success
"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$USERNAME_FIND_DATABOX1'" \
	--find-databox "dbType='OVM',ic='00000001'" \
	2>/dev/null
if [ 0 != $? ]; then
	echo "FindDatabox: $USERNAME_FIND_DATABOX1 - ERROR"
	exit
else
	echo "FindDatabox: $USERNAME_FIND_DATABOX1 - OK"
fi
echo ""

# this request must finish with success
"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$USERNAME_FIND_DATABOX2'" \
	--find-databox "dbType='OVM',firmName='Ministerstvo dopravy'" \
	2>/dev/null
if [ 0 != $? ]; then
	echo "FindDatabox: $USERNAME_FIND_DATABOX2 - ERROR"
	exit
else
	echo "FindDatabox: $USERNAME_FIND_DATABOX2 - OK"
fi
echo ""

# this request must finish with error
"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$USERNAME_FIND_DATABOX2'" \
	--find-databox "dbType='FO',firmName='Ministerstvo dopravy'" \
	2>/dev/null
if [ 0 != $? ]; then
	echo "FindDatabox: $USERNAME_FIND_DATABOX2 - ERROR: it is OK"
else
	echo "FindDatabox: $USERNAME_FIND_DATABOX2 - ERROR"
	exit
fi

echo ""
echo ""
echo "-------------------------------------------------"
echo "CONGRATULATION: All find tests were done with success."
echo "-------------------------------------------------"
echo ""
