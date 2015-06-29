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



echo "***********************************************************************"
echo "* TEST 01 - Check messages where attachment missing (via all accounts)*"
echo "***********************************************************************"


for login in $LOGINS; do
	"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$login'" \
		--check-attachment \
		2>/dev/null
	if [ 0 != $? ]; then
		echo CHYBA
		exit
	fi
done





echo "***********************************************************************"
echo "TEST 02 - Create and send a new message"
echo "***********************************************************************"

DTIME=$(date +"%Y-%m-%d %T")
DMANNOTATION="Datovka - test CLI - ${DTIME}"
DMATACHMENT="${ATTACH_PATH}/dokument.odt;${ATTACH_PATH}/dokument.pdf;${ATTACH_PATH}/notification.mp3;${ATTACH_PATH}/obrazek.jpg;\
${ATTACH_PATH}/obrazek.png;${ATTACH_PATH}/datova-zprava.zfo;"
"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$login'" \
	--send-msg "dbIDRecipient='$databox',dmAnnotation='${DMANNOTATION}',dmAttachment='${DMATACHMENT}'" \
	2>/dev/null
if [ 0 != $? ]; then
	echo CHYBA
	exit
fi

DTIME=$(date +"%Y-%m-%d %T")
DMANNOTATION="Datovka - test CLI - ${DTIME}"
DMATACHMENT="${ATTACH_PATH}/dokument.odt;;${ATTACH_PATH}/dokument.pdf;${ATTACH_PATH}/notification.mp3;${ATTACH_PATH}/obrazek.jpg;\
${ATTACH_PATH}/obrazek.png;;;${ATTACH_PATH}/datova-zprava.zfo;;;;"
"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$login'" \
	--send-msg "dbIDRecipient='$databox',dmAnnotation='${DMANNOTATION}',dmAttachment='${DMATACHMENT}'" \
	2>/dev/null
if [ 0 != $? ]; then
	echo CHYBA
	exit
fi

DTIME=$(date +"%Y-%m-%d %T")
DMANNOTATION="Datovka - test CLI - ${DTIME}"
DMATACHMENT="${ATTACH_PATH}/dokument.odt;${ATTACH_PATH}/dokument.pdf;${ATTACH_PATH}/notification.mp3;${ATTACH_PATH}/obrazek.jpg;\
${ATTACH_PATH}/obrazek.png;${ATTACH_PATH}/datova-zprava.zfo;"
"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$login'" \
	--send-msg "dbIDRecipient='$databox',dmAnnotation='${DMANNOTATION}',dmAttachment='${DMATACHMENT}'" \
	2>/dev/null
if [ 0 != $? ]; then
	echo CHYBA
	exit
fi

DTIME=$(date +"%Y-%m-%d %T")
DMANNOTATION="Datovka - test CLI - ${DTIME}"
DMATACHMENT="${ATTACH_PATH}/dokument.odt;${ATTACH_PATH}/dokument.pdf;${ATTACH_PATH}/notification.mp3;${ATTACH_PATH}/obrazek.jpg;\
${ATTACH_PATH}/obrazek.png;${ATTACH_PATH}/datova-zprava.zfo;"
"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} -D \
	--login "username='$login'" \
	--send-msg "dbIDRecipient='$databox;$databox',dmAnnotation='${DMANNOTATION}',dmAttachment='${DMATACHMENT}'"  \
	2>/dev/null
if [ 0 != $? ]; then
	echo CHYBA
fi
