#!/usr/bin/env sh

#pwd

SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "${SCRIPT}")

#echo ${SCRIPTPATH}
#CMDARGS="${CMDARGS} -D"
#CMDARGS="${CMDARGS} --conf-subdir .dsgui"
#CMDARGS="${CMDARGS} --debug-verbosity 2"
#CMDARGS="${CMDARGS} --log-verbosity 2"

APP_BINARY_NAME="/../../datovka"
ATTACH_PATH="${SCRIPTPATH}/attachment"
. "${SCRIPTPATH}/../../untracked/logins.sh"


echo "************************************************************************"
echo "TEST 01A: Login into databox for accounts where username and pwd is used"
echo "************************************************************************"

for login in $USER_LOGINS; do
	"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$login'" \
		2>/dev/null
	if [ 0 != $? ]; then
		echo "Login: $login - ERROR - account not exists or data missing"
	else
		echo "Login: $login - OK"
	fi
done


echo "************************************************************************"
echo "TEST 01B: Login into databox with certificate - cert pwd is requried"
echo "************************************************************************"

"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$CERT_LOGIN'" \
	2>/dev/null
if [ 0 != $? ]; then
	echo "Login: $CERT_LOGIN - ERROR - cert/pwd missing"
else
	echo "Login: $CERT_LOGIN - OK"
fi

"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$CERT_LOGIN',otpcode='$CERT_PWD'" \
	2>/dev/null
if [ 0 != $? ]; then
	echo "Login: $CERT_LOGIN - ERROR - account not exists or data missing"
else
	echo "Login: $CERT_LOGIN - OK"
fi


echo "************************************************************************"
echo "TEST 01C: Login into databox which is not saved password"
echo "************************************************************************"

"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$INCOMP_LOGIN'" \
	2>/dev/null
if [ 0 != $? ]; then
	echo "Login: $INCOMP_LOGIN - ERROR - data/pwd missing"
else
	echo "Login: $INCOMP_LOGIN - OK"
fi

"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$INCOMP_LOGIN',password='$INCOMP_PWD'" \
	2>/dev/null
if [ 0 != $? ]; then
	echo "Login: $INCOMP_LOGIN - ERROR - account not exists or data missing"
else
	echo "Login: $INCOMP_LOGIN - OK"
fi


echo "***********************************************************************"
echo "TEST 02A: Obtaining user info and owner info for configured accounts"
echo "***********************************************************************"

for login in $USER_LOGINS; do
	"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$login'" \
		--get-user-info \
		2>/dev/null
	if [ 0 != $? ]; then
		echo "User info: $login - ERROR"
	else
		echo "User info: $login - OK"
	fi
done

for login in $USER_LOGINS; do
	"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$login'" \
		--get-owner-info \
		2>/dev/null
	if [ 0 != $? ]; then
		echo "Owner info: $login - ERROR"
	else
		echo "Owner info: $login - OK"
	fi
done

"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$CERT_LOGIN',otpcode='$CERT_PWD'" \
	--get-user-info \
	2>/dev/null
if [ 0 != $? ]; then
	echo "User info: $CERT_LOGIN - ERROR"
else
	echo "User info: $CERT_LOGIN - OK"
fi

"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$CERT_LOGIN',otpcode='$CERT_PWD'" \
	--get-owner-info \
	2>/dev/null
if [ 0 != $? ]; then
	echo "Owner info: $CERT_LOGIN - ERROR"
else
	echo "Owner info: $CERT_LOGIN - OK"
fi


echo "***********************************************************************"
echo "TEST 03: Create and send a new message"
echo "***********************************************************************"


DTIME=$(date +"%Y-%m-%d %T")
DMANNOTATION="Datovka - test CLI - ${DTIME}"
DMATACHMENT="${ATTACH_PATH}/dokument.odt;${ATTACH_PATH}/dokument.pdf;${ATTACH_PATH}/notification.mp3;${ATTACH_PATH}/obrazek.jpg;\
${ATTACH_PATH}/obrazek.png;${ATTACH_PATH}/datova-zprava.zfo"
"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$TEST3_USER'" \
	--send-msg "dbIDRecipient='$TEST3_WRONG_RECIP',dmAnnotation='${DMANNOTATION}',dmAttachment='${DMATACHMENT}'" \
	2>/dev/null
if [ 0 != $? ]; then
	echo "SendMsg: $TEST3_USER - ERROR - account '$TEST3_WRONG_RECIP' does not exist"
else
	echo "SendMsg: $TEST3_USER - OK"
fi

DTIME=$(date +"%Y-%m-%d %T")
DMANNOTATION="Datovka - test CLI - ${DTIME}"
DMATACHMENT="${ATTACH_PATH}/dokument.odt;${ATTACH_PATH}/dokument.pdf;${ATTACH_PATH}/notification.mp3;${ATTACH_PATH}/obrazek.jpg;\
${ATTACH_PATH}/obrazek.png;${ATTACH_PATH}/datova-zprava.zfo;${ATTACH_PATH}/xxxxxxxx.zfo"
"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$TEST3_USER'" \
	--send-msg "dbIDRecipient='$TEST3_RECIP',dmAnnotation='${DMANNOTATION}',dmAttachment='${DMATACHMENT}'" \
	2>/dev/null
if [ 0 != $? ]; then
	echo "SendMsg: $TEST3_USER - ERROR - attachment file not found"
else
	echo "SendMsg: $TEST3_USER - OK"
fi

DTIME=$(date +"%Y-%m-%d %T")
DMANNOTATION="Datovka - test CLI - ${DTIME}"
DMATACHMENT="${ATTACH_PATH}/dokument.odt;${ATTACH_PATH}/dokument.pdf;${ATTACH_PATH}/notification.mp3;${ATTACH_PATH}/obrazek.jpg;\
${ATTACH_PATH}/obrazek.png;${ATTACH_PATH}/datova-zprava.zfo"
"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$TEST3_USER'" \
	--send-msg "dbIDRecipient='$TEST3_RECIP',dmAnnotation='${DMANNOTATION}',dmAttachment='${DMATACHMENT}'" \
	2>/dev/null
if [ 0 != $? ]; then
	echo "SendMsg: $TEST3_USER - ERROR"
else
	echo "SendMsg: $TEST3_USER - OK"
fi


DTIME=$(date +"%Y-%m-%d %T")
DMANNOTATION="Datovka - test CLI - ${DTIME}"
DMATACHMENT="${ATTACH_PATH}/dokument.odt;${ATTACH_PATH}/dokument.pdf;${ATTACH_PATH}/notification.mp3;${ATTACH_PATH}/obrazek.jpg;\
${ATTACH_PATH}/obrazek.png;${ATTACH_PATH}/datova-zprava.zfo"
"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} -D \
	--login "username='$CERT_LOGIN',otpcode='$CERT_PWD'" \
	--send-msg "dbIDRecipient='$TEST3_RECIPS',dmAnnotation='${DMANNOTATION}',dmAttachment='${DMATACHMENT}'"  \
	2>/dev/null
if [ 0 != $? ]; then
	echo "SendMsg: $CERT_LOGIN- ERROR"
else
	echo "SendMsg: $CERT_LOGIN - OK"
fi


DTIME=$(date +"%Y-%m-%d %T")
DMANNOTATION="Datovka - test CLI - ${DTIME}"
DMATACHMENT="${ATTACH_PATH}/dokument.odt;;${ATTACH_PATH}/dokument.pdf;${ATTACH_PATH}/notification.mp3;${ATTACH_PATH}/obrazek.jpg;\
${ATTACH_PATH}/obrazek.png;;;${ATTACH_PATH}/datova-zprava.zfo"
"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$TEST3_USER'" \
	--send-msg "dbIDRecipient='$TEST3_RECIP',dmAttachment='${DMATACHMENT}',dmAnnotation='${DMANNOTATION}',dmPersonalDelivery='1',dmAllowSubstDelivery='1',dmOVM='0',dmPublishOwnID='0',dmToHands='Jan Pokušitel Červeň',dmRecipientRefNumber='98765',dmSenderRefNumber='123456',dmRecipientIdent='CZ98765',dmSenderIdent='CZ123456',dmLegalTitleLaw='1',dmLegalTitleYear='2',dmLegalTitleSect='3',dmLegalTitlePar='4',dmLegalTitlePoint='5'" \
	2>/dev/null
if [ 0 != $? ]; then
	echo "SendMsg: $TEST3_USER - ERROR"
else
	echo "SendMsg: $TEST3_USER - OK"
fi


echo "***********************************************************************"
echo "* TEST 04: Check messages where attachment missing (via all accounts)"
echo "***********************************************************************"


for login in $LOGINS; do
	"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$login'" \
		--check-attachment \
		2>/dev/null
	if [ 0 != $? ]; then
		echo "CheckAttach: $login - ERROR"
	else
		echo "CheckAttach: $login - OK"
	fi
done
