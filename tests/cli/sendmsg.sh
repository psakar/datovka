#!/usr/bin/env bash

#pwd

SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "${SCRIPT}")

CMDARGS="${CMDARGS} -D"
CMDARGS="${CMDARGS} --conf-subdir .dsgui"
CMDARGS="${CMDARGS} --debug-verbosity 2"
CMDARGS="${CMDARGS} --log-verbosity 2"
        
. "${SCRIPTPATH}/../../untracked/logins.sh"

if [[ "$OSTYPE" == "linux-gnu" ]]; then
	OS_NAME="Linux"
	APP_BINARY_NAME="datovka"
	ATTACH_LOAD_PATH="${SCRIPTPATH}/attachment"
	APP_PATH="${SCRIPTPATH}/../.."
elif [[ "$OSTYPE" == "darwin"* ]]; then
	OS_NAME="macOS"
	APP_BINARY_NAME="datovka"
	ATTACH_LOAD_PATH="${SCRIPTPATH}/attachment"
	APP_PATH="${SCRIPTPATH}/../.."
elif [[ "$OSTYPE" == "msys" ]]; then
	OS_NAME="Windows"
	APP_BINARY_NAME="datovka-cli.exe"
	ATTACH_LOAD_PATH="${SCRIPTPATH}/attachment"
	ATTACH_LOAD_PATH="${ATTACH_LOAD_PATH:1:${#ATTACH_LOAD_PATH}}"
	ATTACH_LOAD_PATH="${ATTACH_LOAD_PATH:0:1}:${ATTACH_LOAD_PATH:1:${#ATTACH_LOAD_PATH}}"  
	APP_PATH="C:\Program Files (x86)\CZ.NIC\Datovka"  
elif [[ "$OSTYPE" == "win32" ]]; then
	OS_NAME="Windows"
	APP_BINARY_NAME="datovka-cli.exe"
	ATTACH_LOAD_PATH="${SCRIPTPATH}/attachment"
	ATTACH_LOAD_PATH="${ATTACH_LOAD_PATH:1:${#ATTACH_LOAD_PATH}}"
	ATTACH_LOAD_PATH="${ATTACH_LOAD_PATH:0:1}:${ATTACH_LOAD_PATH:1:${#ATTACH_LOAD_PATH}}"  
	APP_PATH="C:\Program Files (x86)\CZ.NIC\Datovka"
else
	echo "ERROR: Unknown platform"
	exit
fi

echo ""
echo "***********************************************************************"
echo "SEND MSG TEST: Create and send a new message from account with"
echo "               usename and password."
echo "***********************************************************************"
echo "---Create and send a new message from user '$USERNAME_SEND'---"
DTIME=$(date +"%Y-%m-%d %T")
DMANNOTATION="Datovka: test CLI ${OS_NAME} - ${DTIME}"
DMATACHMENT="${ATTACH_LOAD_PATH}/dokument.odt;${ATTACH_LOAD_PATH}/dokument.pdf;${ATTACH_LOAD_PATH}/notification.mp3;${ATTACH_LOAD_PATH}/obrazek.jpg;\
${ATTACH_LOAD_PATH}/obrazek.png;${ATTACH_LOAD_PATH}/datova-zprava.zfo"
MSGID=`"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$USERNAME_SEND'" \
	--send-msg "dbIDRecipient='$RECIPIENT_SEND',dmAnnotation='${DMANNOTATION}',dmAttachment='${DMATACHMENT}'" \
	2>/dev/null`
if [ 0 != $? ]; then
	echo "SendMsg: $USERNAME_SEND - ERROR"
	exit
else
	echo "SendMsg: $USERNAME_SEND, msgID: '$MSGID' - OK"
fi
echo ""
echo "Waiting for the server DS - 3 seconds ..."
sleep 3

echo ""
echo "***********************************************************************"
echo "SEND MSG TEST: Create and send a new message from account (OVM) with"
echo "               certificate to multiple recipients (OVM, FO, PO)."
echo "***********************************************************************"
echo "---Create and send a new message from user '$USERNAME_SEND2'---"
DTIME=$(date +"%Y-%m-%d %T")
DMANNOTATION="Datovka: test CLI ${OS_NAME} - ${DTIME}"
DMATACHMENT="${ATTACH_LOAD_PATH}/dokument.odt;${ATTACH_LOAD_PATH}/dokument.pdf;${ATTACH_LOAD_PATH}/notification.mp3;${ATTACH_LOAD_PATH}/obrazek.jpg;\
${ATTACH_LOAD_PATH}/obrazek.png;${ATTACH_LOAD_PATH}/datova-zprava.zfo"
MSGID=`"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} -D \
	--login "username='$USERNAME_SEND2'" \
	--send-msg "dbIDRecipient='$RECIPIENTS_SEND2',dmAnnotation='${DMANNOTATION}',dmAttachment='${DMATACHMENT}'"  \
	2>/dev/null`
if [ 0 != $? ]; then
	echo "SendMultiMsg: $USERNAME_SEND2- ERROR"
	exit
else
	echo "SendMultiMsg: $USERNAME_SEND2 - msgIDs: - $MSGID - OK"
fi
echo ""
echo "Waiting for the server DS - 3 seconds ..."
sleep 3

echo ""
echo "***********************************************************************"
echo "SEND MSG TEST: Create and send a new message from account (PO) to (OVM)."
echo "               All mandatory and optional attributes are filled."
echo "***********************************************************************"
echo "---Create and send a new message from user '$USERNAME_SEND'---"
DTIME=$(date +"%Y-%m-%d %T")
DMANNOTATION="Datovka: test CLI ${OS_NAME} - ${DTIME}"
DMATACHMENT="${ATTACH_LOAD_PATH}/dokument.odt;;${ATTACH_LOAD_PATH}/dokument.pdf;${ATTACH_LOAD_PATH}/notification.mp3;${ATTACH_LOAD_PATH}/obrazek.jpg;\
${ATTACH_LOAD_PATH}/obrazek.png;;;${ATTACH_LOAD_PATH}/datova-zprava.zfo"
MSGID=`"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$USERNAME_SEND'" \
	--send-msg "dbIDRecipient='$RECIPIENT_SEND',dmAttachment='${DMATACHMENT}',dmAnnotation='${DMANNOTATION}',dmPersonalDelivery='1',dmAllowSubstDelivery='1',dmOVM='0',dmPublishOwnID='1',dmToHands='Jan Pokušitel Červeň',dmRecipientRefNumber='98765',dmSenderRefNumber='123456',dmRecipientIdent='CZ98765',dmSenderIdent='CZ123456',dmLegalTitleLaw='1',dmLegalTitleYear='2',dmLegalTitleSect='3',dmLegalTitlePar='4',dmLegalTitlePoint='5'" \
	2>/dev/null`
if [ 0 != $? ]; then
	echo "SendMsg: $USERNAME_SEND - ERROR"
	exit
else
	echo "SendMsg: $USERNAME_SEND - msgID: - $MSGID - OK"
fi
echo ""
echo "Waiting for the server DS - 3 seconds ..."
sleep 3

echo ""
echo "***********************************************************************"
echo "SEND MSG TEST: Create and send a new message non exist recipient."
echo "***********************************************************************"
# this request must finish with error
echo "---Create and send a new message from user '$USERNAME_SEND'---"
DTIME=$(date +"%Y-%m-%d %T")
DMANNOTATION="Datovka: test CLI ${OS_NAME} - ${DTIME}"
DMATACHMENT="${ATTACH_LOAD_PATH}/dokument.odt;${ATTACH_LOAD_PATH}/dokument.pdf;${ATTACH_LOAD_PATH}/notification.mp3;${ATTACH_LOAD_PATH}/obrazek.jpg;\
${ATTACH_LOAD_PATH}/obrazek.png;${ATTACH_LOAD_PATH}/datova-zprava.zfo"
MSGID=`"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$USERNAME_SEND'" \
	--send-msg "dbIDRecipient='$RECIPIENT_SEND_NOTEXIST',dmAnnotation='${DMANNOTATION}',dmAttachment='${DMATACHMENT}'" \
	2>/dev/null`
if [ 0 != $? ]; then
	echo "SendMsg: $USERNAME_SEND - recipient NOT exists - OK"
else
	echo "SendMsg: $USERNAME_SEND, msgID: '$MSGID' - ERROR"
	exit
fi

echo ""
echo "***********************************************************************"
echo "SEND MSG TEST: Create and send a new message - wrong parametrs."
echo "***********************************************************************"
# this request must finish with error
echo "---Create and send a new message from user '$USERNAME_SEND'---"
DMANNOTATION="Datovka: test CLI ${OS_NAME} - Error"
DMATACHMENT="${ATTACH_LOAD_PATH}/dokument.odt;${ATTACH_LOAD_PATH}/dokument.pdf;${ATTACH_LOAD_PATH}/notification.mp3;${ATTACH_LOAD_PATH}/obrazek.jpg;\
${ATTACH_LOAD_PATH}/obrazek.png;${ATTACH_LOAD_PATH}/datova-zprava.zfo;${ATTACH_LOAD_PATH}/xxxxxxxx.zfo"
"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$TEST_USER_SEND1'" \
	--send-msg "dbIDRecipient='$RECIPIENT_SEND',dmAnnotation='${DMANNOTATION}',dmAttachment='${DMATACHMENT}'" \
	2>/dev/null
if [ 0 != $? ]; then
	echo "SendMsg: $USERNAME_SEND - this message has wrong attachment path - OK"
else
	echo "SendMsg: $USERNAME_SEND - ERROR"
	exit
fi

echo ""
echo ""
echo "------------------------------------------------------------------------"
echo "CONGRATULATION: All send message tests were done with success."
echo "------------------------------------------------------------------------"
echo ""
