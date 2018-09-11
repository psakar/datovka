#!/usr/bin/env bash

SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "${SCRIPT}")

IS_ERROR=false

print_error() {
	IS_ERROR=true
	RED='\033[1;31m' # Set Color
	NC='\033[0m' # No Color
	echo -e ${RED}$1${NC}
}

print_success() {
	GREEN='\033[1;32m' # Set Color
	NC='\033[0m' # No Color
	echo -e ${GREEN}$1${NC}
}

# Run Login tests
bash ${SCRIPTPATH}/login.sh
if [ 0 != $? ]; then
	IS_ERROR=true
fi

# Run Send message tests
bash ${SCRIPTPATH}/sendmsg.sh
if [ 0 != $? ]; then
	IS_ERROR=true
fi

# Run Get message list and download message tests
bash ${SCRIPTPATH}/getmsg.sh
if [ 0 != $? ]; then
	IS_ERROR=true
fi

# Run Find databox tests
bash ${SCRIPTPATH}/find.sh
if [ 0 != $? ]; then
	IS_ERROR=true
fi

if [ "$IS_ERROR" = false ] ; then
	echo ""
	print_success "======================================================================="
	print_success "CONGRATULATION: All CLI tests were done with success."
	print_success "======================================================================="
	echo ""
	exit 0
else 
	echo ""
	print_error "======================================================================="
	print_error "FAIL: Some CLI tests have been failed!!! See backtrace for error."
	print_error "======================================================================="
	echo ""
	exit 1
fi

