#!/usr/bin/env bash

SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "${SCRIPT}")

CMDARGS="${CMDARGS} -D"
CMDARGS="${CMDARGS} --conf-subdir .dsgui"
CMDARGS="${CMDARGS} --debug-verbosity 2"
CMDARGS="${CMDARGS} --log-verbosity 2"

. "${SCRIPTPATH}/../../untracked/logins.sh"

SOME_ERROR=false

print_error() {
	SOME_ERROR=true
	RED='\033[1;31m' # Set Color
	NC='\033[0m' # No Color
	echo -e ${RED}$1${NC}
}

print_success() {
	GREEN='\033[1;32m' # Set Color
	NC='\033[0m' # No Color
	echo -e ${GREEN}$1${NC}
}

if [[ "$OSTYPE" == "linux-gnu" ]]; then
	APP_BINARY_NAME="datovka"
	APP_PATH="${SCRIPTPATH}/../.."
elif [[ "$OSTYPE" == "darwin"* ]]; then
	APP_BINARY_NAME="datovka"
	APP_PATH="${SCRIPTPATH}/../../datovka.app/Contents/MacOS"
elif [[ "$OSTYPE" == "msys" ]]; then
	APP_BINARY_NAME="datovka-cli.exe"
	APP_PATH="C:\Program Files (x86)\CZ.NIC\Datovka"
elif [[ "$OSTYPE" == "win32" ]]; then
	APP_BINARY_NAME="datovka-cli.exe"
	APP_PATH="C:\Program Files (x86)\CZ.NIC\Datovka"
else
	print_error "ERROR: Unknown platform"
	exit 1
fi

echo ""
echo "***********************************************************************"
echo "FIND DATABOX TEST: Find databoxes by ic, firmname"
echo "***********************************************************************"
# this request must finish with success
"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$USERNAME_FIND_DATABOX1'" \
	--find-databox "dbType='OVM',ic='12345678'" \
	2>/dev/null
if [ 0 != $? ]; then
	print_error "FindDatabox: $USERNAME_FIND_DATABOX1 - ERROR"
else
	print_success "FindDatabox: $USERNAME_FIND_DATABOX1 - OK"
fi
echo ""

# this request must finish with success
"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$USERNAME_FIND_DATABOX1'" \
	--find-databox "dbType='OVM',ic='00000001'" \
	2>/dev/null
if [ 0 != $? ]; then
	print_error "FindDatabox: $USERNAME_FIND_DATABOX1 - ERROR"
else
	print_success "FindDatabox: $USERNAME_FIND_DATABOX1 - OK"
fi
echo ""

# this request must finish with success
"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$USERNAME_FIND_DATABOX2'" \
	--find-databox "dbType='OVM',firmName='Ministerstvo dopravy'" \
	2>/dev/null
if [ 0 != $? ]; then
	print_error "FindDatabox: $USERNAME_FIND_DATABOX2 - ERROR"
else
	print_success "FindDatabox: $USERNAME_FIND_DATABOX2 - OK"
fi
echo ""

# this request must finish with error
"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$USERNAME_FIND_DATABOX2'" \
	--find-databox "dbType='FO',firmName='Ministerstvo dopravy'" \
	2>/dev/null
if [ 0 != $? ]; then
	print_success "FindDatabox: $USERNAME_FIND_DATABOX2 - ERROR: it is OK"
else
	print_error "FindDatabox: $USERNAME_FIND_DATABOX2 - ERROR"
fi

if [ "$SOME_ERROR" = false ] ; then
	echo ""
	print_success "-----------------------------------------------------------------------"
	print_success "SUCCESS: All find databox tests were done with success."
	print_success "-----------------------------------------------------------------------"
	echo ""
	exit 0
else 
	echo ""
	print_error "-----------------------------------------------------------------------"
	print_error "FAIL: Some find databox tests have been failed!"
	print_error "-----------------------------------------------------------------------"
	echo ""
	exit 1
fi
