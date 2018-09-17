#!/usr/bin/env bash

# Obtain location of source root.
src_root () {
	local SCRIPT_LOCATION=""
	local SYSTEM=$(uname -s)
	if [ ! "x${SYSTEM}" = "xDarwin" ]; then
		local SCRIPT=$(readlink -f "$0")
		SCRIPT_LOCATION=$(dirname $(readlink -f "$0"))
	else
		SCRIPT_LOCATION=$(cd "$(dirname "$0")"; pwd)
	fi

	echo $(cd "$(dirname "${SCRIPT_LOCATION}")"; cd ..; pwd)
}

SRC_ROOT=$(src_root)
cd "${SRC_ROOT}"

SCRIPT_PATH="${SRC_ROOT}/tests/cli"

. "${SCRIPT_PATH}/helper.sh" # Contains HAVE_ERROR variable.


. "${SRC_ROOT}/untracked/logins.sh"

APP_BINARY_PATH="$1"
if [ ! -e "${APP_BINARY_PATH}" ]; then
	echo_error "ERROR: Cannot locate tested binary."
	exit 1
fi
OS_NAME="$2"
ATTACH_LOAD_PATH="$3"
if [ "x${ATTACH_LOAD_PATH}" = "x" -o ! -d "${ATTACH_LOAD_PATH}" ]; then
	echo_error "ERROR: Cannot locate directory where to load attachments from."
	exit 1
fi

CMDARGS=""
CMDARGS="${CMDARGS} -D"
CMDARGS="${CMDARGS} --conf-subdir .dsgui"
CMDARGS="${CMDARGS} --debug-verbosity 2"
CMDARGS="${CMDARGS} --log-verbosity 2"


echo ""
echo "***********************************************************************"
echo "SEND MSG TEST: Create and send a new message from account using"
echo "               a usename and password."
echo "***********************************************************************"
echo "---Create and send a new message from user '$USERNAME_SEND'---"
DTIME=$(date +"%Y-%m-%d %T")
DMANNOTATION="Datovka: test CLI ${OS_NAME} - ${DTIME}"
DMATACHMENT="${ATTACH_LOAD_PATH}/obrazek.jpg"
MSGID=$("${APP_BINARY_PATH}" ${CMDARGS} \
	--login "username='$USERNAME_SEND'" \
	--send-msg "dbIDRecipient='$RECIPIENT_SEND',dmAnnotation='${DMANNOTATION}',dmAttachment='${DMATACHMENT}'" \
	2>/dev/null)
if [ 0 != $? ]; then
	echo_error "SendMsg: $USERNAME_SEND - ERROR"
else
	echo_success "SendMsg: $USERNAME_SEND, msgID: '$MSGID' - OK"
fi
echo ""
echo "Waiting for the ISDS server - 3 seconds ..."
sleep 3

echo ""
echo "***********************************************************************"
echo "SEND MSG TEST: Create and send a new message from account (OVM) using"
echo "               a certificate to multiple recipients (OVM, FO, PO)."
echo "***********************************************************************"
echo "---Create and send a new message from user '$USERNAME_SEND2'---"
DTIME=$(date +"%Y-%m-%d %T")
DMANNOTATION="Datovka: test CLI ${OS_NAME} - ${DTIME}"
DMATACHMENT="${ATTACH_LOAD_PATH}/dokument.odt;${ATTACH_LOAD_PATH}/dokument.pdf;${ATTACH_LOAD_PATH}/notification.mp3;${ATTACH_LOAD_PATH}/obrazek.png"
MSGID=$("${APP_BINARY_PATH}" ${CMDARGS} -D \
	--login "username='$USERNAME_SEND2'" \
	--send-msg "dbIDRecipient='$RECIPIENTS_SEND2',dmAnnotation='${DMANNOTATION}',dmAttachment='${DMATACHMENT}'"  \
	2>/dev/null)
if [ 0 != $? ]; then
	echo_error "SendMultiMsg: $USERNAME_SEND2- ERROR"
else
	echo_success "SendMultiMsg: $USERNAME_SEND2 - msgIDs: - $MSGID - OK"
fi
echo ""
echo "Waiting for the ISDS server - 3 seconds ..."
sleep 3

echo ""
echo "***********************************************************************"
echo "SEND MSG TEST: Create and send a new message from account (PO) to (OVM)."
echo "               All mandatory and optional attributes are filled."
echo "***********************************************************************"
echo "---Create and send a new message from user '$USERNAME_SEND'---"
DTIME=$(date +"%Y-%m-%d %T")
DMANNOTATION="Datovka: test CLI ${OS_NAME} - ${DTIME}"
DMATACHMENT="${ATTACH_LOAD_PATH}/dokument.odt;${ATTACH_LOAD_PATH}/dokument.pdf;${ATTACH_LOAD_PATH}/obrazek.png"
MSGID=$("${APP_BINARY_PATH}" ${CMDARGS} \
	--login "username='$USERNAME_SEND'" \
	--send-msg "dbIDRecipient='$RECIPIENT_SEND',dmAttachment='${DMATACHMENT}',dmAnnotation='${DMANNOTATION}',dmPersonalDelivery='1',dmAllowSubstDelivery='1',dmOVM='0',dmPublishOwnID='1',dmToHands='Jan Pokušitel Červeň',dmRecipientRefNumber='98765',dmSenderRefNumber='123456',dmRecipientIdent='CZ98765',dmSenderIdent='CZ123456',dmLegalTitleLaw='1',dmLegalTitleYear='2',dmLegalTitleSect='3',dmLegalTitlePar='4',dmLegalTitlePoint='5'" \
	2>/dev/null)
if [ 0 != $? ]; then
	echo_error "SendMsg: $USERNAME_SEND - ERROR"
else
	echo_success "SendMsg: $USERNAME_SEND - msgID: - $MSGID - OK"
fi
echo ""
echo "Waiting for the ISDS server - 3 seconds ..."
sleep 3

echo ""
echo "***********************************************************************"
echo "SEND MSG TEST: Create and send a new message to a non-existent recipient."
echo "***********************************************************************"
# this request must finish with error
echo "---Create and send a new message from user '$USERNAME_SEND'---"
DTIME=$(date +"%Y-%m-%d %T")
DMANNOTATION="Datovka: test CLI ${OS_NAME} - ${DTIME}"
DMATACHMENT="${ATTACH_LOAD_PATH}/dokument.pdf"
MSGID=$("${APP_BINARY_PATH}" ${CMDARGS} \
	--login "username='$USERNAME_SEND'" \
	--send-msg "dbIDRecipient='$RECIPIENT_SEND_NOTEXIST',dmAnnotation='${DMANNOTATION}',dmAttachment='${DMATACHMENT}'" \
	2>/dev/null)
if [ 0 != $? ]; then
	echo_success "SendMsg: $USERNAME_SEND - recipient NOT exists - OK"
else
	echo_error "SendMsg: $USERNAME_SEND, msgID: '$MSGID' - ERROR"
fi

echo ""
echo "***********************************************************************"
echo "SEND MSG TEST: Create and send a new message - wrong parameters."
echo "***********************************************************************"
# this request must finish with error
echo "---Create and send a new message from user '$USERNAME_SEND'---"
DMANNOTATION="Datovka: test CLI ${OS_NAME} - Error"
DMATACHMENT="${ATTACH_LOAD_PATH}/dokument.odt;${ATTACH_LOAD_PATH}/xxxxxxxx.zfo"
"${APP_BINARY_PATH}" ${CMDARGS} \
	--login "username='$TEST_USER_SEND1'" \
	--send-msg "dbIDRecipient='$RECIPIENT_SEND',dmAnnotation='${DMANNOTATION}',dmAttachment='${DMATACHMENT}'" \
	2>/dev/null
if [ 0 != $? ]; then
	echo_success "SendMsg: $USERNAME_SEND - this message has wrong attachment path - OK"
else
	echo_error "SendMsg: $USERNAME_SEND - ERROR"
fi

if [ "x${HAVE_ERROR}" = "xfalse" ]; then
	echo ""
	echo_success "-----------------------------------------------------------------------"
	echo_success "SUCCESS: All send message tests have finished as expected."
	echo_success "-----------------------------------------------------------------------"
	echo ""
	exit 0
else
	echo ""
	echo_error "-----------------------------------------------------------------------"
	echo_error "FAILURE: Some send message tests have failed!"
	echo_error "-----------------------------------------------------------------------"
	echo ""
	exit 1
fi
