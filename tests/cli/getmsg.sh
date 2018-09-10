#!/usr/bin/env bash

#pwd

SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "${SCRIPT}")

CMDARGS="${CMDARGS} -D"
CMDARGS="${CMDARGS} --conf-subdir .dsgui"
CMDARGS="${CMDARGS} --debug-verbosity 2"
CMDARGS="${CMDARGS} --log-verbosity 2"

. "${SCRIPTPATH}/../../untracked/logins.sh"

# You can change save location
AT_FILE_SAVE_PATH="${SCRIPTPATH}/../../tmp/"

rm -rf $AT_FILE_SAVE_PATH
mkdir $AT_FILE_SAVE_PATH

if [[ "$OSTYPE" == "linux-gnu" ]]; then
	OS_NAME="Linux"
	APP_BINARY_NAME="datovka"
	ATTACH_LOAD_PATH="${SCRIPTPATH}/attachment"
	APP_PATH="${SCRIPTPATH}/../.."
	ATTACH_SAVE_PATH="${AT_FILE_SAVE_PATH}"
elif [[ "$OSTYPE" == "darwin"* ]]; then
	OS_NAME="macOS"
	APP_BINARY_NAME="datovka"
	ATTACH_LOAD_PATH="${SCRIPTPATH}/attachment"
	APP_PATH="${SCRIPTPATH}/../../datovka.app/Contents/MacOS"
	ATTACH_SAVE_PATH="${AT_FILE_SAVE_PATH}"
elif [[ "$OSTYPE" == "msys" ]]; then
	OS_NAME="Windows"
	APP_BINARY_NAME="datovka-cli.exe"
	ATTACH_LOAD_PATH="${SCRIPTPATH}/attachment"
	ATTACH_LOAD_PATH="${ATTACH_LOAD_PATH:1:${#ATTACH_LOAD_PATH}}"
	ATTACH_LOAD_PATH="${ATTACH_LOAD_PATH:0:1}:${ATTACH_LOAD_PATH:1:${#ATTACH_LOAD_PATH}}"
	ATTACH_SAVE_PATH="${AT_FILE_SAVE_PATH}"
	ATTACH_SAVE_PATH="${ATTACH_SAVE_PATH:1:${#ATTACH_SAVE_PATH}}"
	ATTACH_SAVE_PATH="${ATTACH_SAVE_PATH:0:1}:${ATTACH_SAVE_PATH:1:${#ATTACH_SAVE_PATH}}"
	APP_PATH="C:\Program Files (x86)\CZ.NIC\Datovka"
elif [[ "$OSTYPE" == "win32" ]]; then
	OS_NAME="Windows"
	APP_BINARY_NAME="datovka-cli.exe"
	ATTACH_LOAD_PATH="${SCRIPTPATH}/attachment"
	ATTACH_LOAD_PATH="${ATTACH_LOAD_PATH:1:${#ATTACH_LOAD_PATH}}"
	ATTACH_LOAD_PATH="${ATTACH_LOAD_PATH:0:1}:${ATTACH_LOAD_PATH:1:${#ATTACH_LOAD_PATH}}"
	ATTACH_SAVE_PATH="${AT_FILE_SAVE_PATH}"
	ATTACH_SAVE_PATH="${ATTACH_SAVE_PATH:1:${#ATTACH_SAVE_PATH}}"
	ATTACH_SAVE_PATH="${ATTACH_SAVE_PATH:0:1}:${ATTACH_SAVE_PATH:1:${#ATTACH_SAVE_PATH}}"
	APP_PATH="C:\Program Files (x86)\CZ.NIC\Datovka"
else
	echo "ERROR: Unknown platform"
	exit
fi

echo ""
echo "***********************************************************************"
echo "MSGLIST TEST: Get received/sent message list only"
echo "              for selected accounts."
echo "***********************************************************************"
#---Get message list for account with username and pwd---
for login in $USERNAMES_MSGLIST_ONLY; do
	MSGIDS=`"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$login'" \
		--get-msg-list "dmType='received'" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgList (received): $login - ERROR"
		exit
	else
		echo "GetMsgList (received): $login - OK $MSGIDS"
	fi
	MSGIDS=`"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$login'" \
		--get-msg-list "dmType='sent'" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgList (sent): $login - ERROR"
		exit
	else
		echo "GetMsgList (sent): $login - OK $MSGID"
	fi
done


echo ""
echo "***********************************************************************"
echo "MSGLIST TEST: Get received/sent message list + complete msgs"
echo "              for selected accounts."
echo "***********************************************************************"
#---Get message list for account with username and pwd---
for login in $USERNAMES_MSGLIST_COMPLETE; do
	MSGIDS=`"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$login'" \
		--get-msg-list "dmType='received',complete='yes'" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgList (received) + complete download: $login - ERROR"
		exit
	else
		echo "GetMsgList (received) + complete download: $login - OK $MSGIDS"
	fi
	MSGIDS=`"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$login'" \
		--get-msg-list "dmType='sent',complete='yes'" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgList (sent) + complete download: $login - ERROR"
		exit
	else
		echo "GetMsgList (sent) + complete download: $login - OK $MSGID"
	fi
done


echo ""
echo "***********************************************************************"
echo "MSGLIST TEST: Create and send a new message from account with"
echo "              usename and password and download this message"
echo "              by recipient, save attachment and export to ZFO."
echo "***********************************************************************"
echo "---Create and send a new message from user '$USERNAME_SEND'---"
DTIME=$(date +"%Y-%m-%d %T")
DMANNOTATION="Datovka - test CLI - ${DTIME}"
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
echo "Waiting for the server DS - 5 seconds ..."
sleep 5

#----must be success and return msg ID/IDs
echo ""
echo "---Get received message list for user '$USERNAME_SEND2'---"
#----must be success and return msg ID/IDs
RMSGIDS=`"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$USERNAME_SEND2'" \
	--get-msg-list "dmType='received'" \
	2>/dev/null`
if [ 0 != $? ]; then
	echo "GetMsgList (received): $USERNAME_SEND2 - ERROR"
	exit
else
	echo "GetMsgList (received): $USERNAME_SEND2 - OK $RMSGIDS"
fi

echo ""
#----Export complete new messages from database------------------------------
#----must fails
for dmID in $RMSGIDS; do
	RET=`"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$USERNAME_SEND2'" \
		--get-msg "dmID='$dmID',dmType='received',download='no',zfoFile='${ATTACH_SAVE_PATH}/DMr_$dmID.zfo'" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgFromDb '$dmID': $USERNAME_SEND2 - failed - it is OK"
	else
		echo "GetMsgFromDb '$dmID': $USERNAME_SEND2 - ERROR"
		exit
	fi
done

#----Export delivery info of new messages from database----------------------
#----must fails
for dmID in $RMSGIDS; do
	RET=`"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$USERNAME_SEND2'" \
		--get-delivery-info "dmID='$dmID',download='no',zfoFile='${ATTACH_SAVE_PATH}/DMr-info_$dmID.zfo'" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgDelInfo '$dmID': $USERNAME_SEND2 - failed - it is OK"
	else
		echo "GetMsgDelInfo '$dmID': $USERNAME_SEND2 - ERROR"
		exit
	fi
done

#-----Download complete new messages ISDS-------------------------------------
#----must be success and save zfo file
for dmID in $RMSGIDS; do
	RET=`"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$USERNAME_SEND2'" \
		--get-msg "dmID='$dmID',dmType='received',zfoFile='${ATTACH_SAVE_PATH}/DMr_$dmID-isds.zfo'" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgFromISDS '$dmID': $USERNAME_SEND2 - ERROR"
		exit
	else
		echo "GetMsgFromISDS '$dmID': $USERNAME_SEND2 - OK"
		echo "ExportToZFO '$dmID': $USERNAME_SEND2 - OK"
	fi
done

#----Download delivery info of new messages from ISDS------------------------
#----must be success and save zfo file
for dmID in $RMSGIDS; do
	RET=`"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$USERNAME_SEND2'" \
		--get-delivery-info "dmID='$dmID',zfoFile='${ATTACH_SAVE_PATH}/DMr-info_$dmID-isds.zfo'" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgDelInfoFromISDS '$dmID': $USERNAME_SEND2- ERROR"
		exit
	else
		echo "GetMsgDelInfoFromISDS '$dmID': $USERNAME_SEND2 - OK"
		echo "ExportToZFO '$dmID': $USERNAME_SEND2 - OK"
	fi
done

#----Export complete messages from database again-----------------------
#----must be success and save zfo file and save attachment
for dmID in $RMSGIDS; do
	RET=`"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$USERNAME_SEND2'" \
		--get-msg "dmID='$dmID',dmType='received',download='no',zfoFile='${ATTACH_SAVE_PATH}/DMr_$dmID-db.zfo',attachmentDir='${ATTACH_SAVE_PATH}'" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgFromDb '$dmID': $USERNAME_SEND2 - ERROR"
		exit
	else
		echo "GetMsgFromDb '$dmID': $USERNAME_SEND2 - OK"
		echo "ExportToZFO '$dmID': $USERNAME_SEND2 - OK"
	fi
done

#----Export delivery info of messages from again----------------------
#----must be success and save zfo file
for dmID in $RMSGIDS; do
	RET=`"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$USERNAME_SEND2'" \
		--get-delivery-info "dmID='$dmID',download='no',zfoFile='${ATTACH_SAVE_PATH}/DMr-info_$dmID-db.zfo'" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgDelInfoFromDb '$dmID': $USERNAME_SEND2 - ERROR"
		exit
	else
		echo "GetMsgDelInfoFromDb '$dmID': $USERNAME_SEND2 - OK"
		echo "ExportToZFO '$dmID': $USERNAME_SEND2 - OK"
	fi
done

#
#
#-----Get sent message list-------------------------------------------------
#
#
echo ""
echo "---Get sent message list for user '$USERNAME_SEND'---"
SMSGIDS=`"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$USERNAME_SEND'" \
	--get-msg-list "dmType='sent'" \
	2>/dev/null`
if [ 0 != $? ]; then
	echo "GetMsgList (sent): $USERNAME_SEND - ERROR"
	exit
else
	echo "GetMsgList (sent): $USERNAME_SEND - OK $SMSGIDS"
fi
#--------------------------------------------------------------------------


echo ""
echo "---Download new sent messages for user '$USERNAME_SEND'---"
#----Export complete new messages from database------------------------------
#----must fails
for dmID in $RMSGIDS; do
	RET=`"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$USERNAME_SEND'" \
		--get-msg "dmID='$dmID',dmType='sent',download='no',zfoFile='${ATTACH_SAVE_PATH}/DMs_$dmID.zfo'" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgFromDb '$dmID': $USERNAME_SEND - failed - it is OK"
	else
		echo "GetMsgFromDb '$dmID': $USERNAME_SEND - ERROR"
		exit
	fi
done

#----Export delivery info of new messages from database----------------------
#----must fails
for dmID in $RMSGIDS; do
	RET=`"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$USERNAME_SEND'" \
		--get-delivery-info "dmID='$dmID',download='no',zfoFile='${ATTACH_SAVE_PATH}/DMs-info_$dmID.zfo'" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgDelInfo '$dmID': $USERNAME_SEND - failed - it is OK"
	else
		echo "GetMsgDelInfo '$dmID': $USERNAME_SEND - ERROR"
		exit
	fi
done

#-----Download complete new messages ISDS-------------------------------------
#----must be success and save zfo file
for dmID in $RMSGIDS; do
	RET=`"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$USERNAME_SEND'" \
		--get-msg "dmID='$dmID',dmType='sent',zfoFile='${ATTACH_SAVE_PATH}/DMs_$dmID-isds.zfo'" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgFromISDS '$dmID': $USERNAME_SEND - ERROR"
		exit
	else
		echo "GetMsgFromISDS '$dmID': $USERNAME_SEND - OK"
		echo "ExportToZFO '$dmID': $USERNAME_SEND - OK"
	fi
done

#----Download delivery info of new messages from ISDS------------------------
#----must be success and save zfo file
for dmID in $RMSGIDS; do
	RET=`"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$USERNAME_SEND'" \
		--get-delivery-info "dmID='$dmID',zfoFile='${ATTACH_SAVE_PATH}/DMs-info_$dmID-isds.zfo'" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgDelInfoFromISDS '$dmID': $USERNAME_SEND - ERROR"
		exit
	else
		echo "GetMsgDelInfoFromISDS '$dmID': $USERNAME_SEND - OK"
		echo "ExportToZFO '$dmID': $USERNAME_SEND - OK"
	fi
done

#----Export complete messages from database again-----------------------
#----must be success and save zfo file
for dmID in $RMSGIDS; do
	RET=`"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$USERNAME_SEND'" \
		--get-msg "dmID='$dmID',dmType='sent',download='no',zfoFile='${ATTACH_SAVE_PATH}/DMs_$dmID-db.zfo'" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgFromDb '$dmID': $USERNAME_SEND - ERROR"
		exit
	else
		echo "GetMsgFromDb '$dmID': $USERNAME_SEND - OK"
		echo "ExportToZFO '$dmID': $USERNAME_SEND - OK"
	fi
done

#----Export delivery info of messages from again----------------------
#----must be success and save zfo file
for dmID in $RMSGIDS; do
	RET=`"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$USERNAME_SEND'" \
		--get-delivery-info "dmID='$dmID',download='no',zfoFile='${ATTACH_SAVE_PATH}/DMs-info_$dmID-db.zfo'" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgDelInfoFromDb '$dmID': $USERNAME_SEND - ERROR"
		exit
	else
		echo "GetMsgDelInfoFromDb '$dmID': $USERNAME_SEND - OK"
		echo "ExportToZFO '$dmID': $USERNAME_SEND - OK"
	fi
done


echo ""
echo "***********************************************************************"
echo "GET MSG TEST: Download new message, set as locally read"
echo "              and export to ZFO for username."
echo "***********************************************************************"
#-----Download complete new messages ISDS-------------------------------------
#----must be success and save zfo file
for dmID in $MSGIDS; do
	RET=`"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$USERNAME_SEND2'" \
		--get-msg "dmID='$dmID',dmType='received',markDownload='yes',zfoFile='${ATTACH_SAVE_PATH}/DMs_$dmID-isds.zfo'" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgFromISDS '$dmID': $USERNAME_SEND2 - ERROR"
		exit
	else
		echo "GetMsgFromISDS '$dmID': $USERNAME_SEND2 - OK"
		echo "ExportToZFO '$dmID': $USERNAME_SEND2 - OK"
	fi
done

#----Download delivery info of new messages from ISDS------------------------
#----must be success and save zfo file
for dmID in $RMSGIDS; do
	RET=`"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$USERNAME_SEND2'" \
		--get-delivery-info "dmID='$dmID',zfoFile='${ATTACH_SAVE_PATH}/DMs-info_$dmID-isds.zfo'" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgDelInfoFromISDS '$dmID': $USERNAME_SEND2 - ERROR"
		exit
	else
		echo "GetMsgDelInfoFromISDS '$dmID': $USERNAME_SEND2 - OK"
		echo "ExportToZFO '$dmID': $USERNAME_SEND2 - OK"
	fi
done


echo "***********************************************************************"
echo "* GET MSG LIST:: Check messages where attachment missing (via all accounts)."
echo "***********************************************************************"
for login in $USERNAMES; do
	"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$login'" \
		--check-attachment \
		2>/dev/null
	if [ 0 != $? ]; then
		echo "CheckAttach: $login - ERROR"
		echo ""
		exit
	else
		echo "CheckAttach: $login - OK"
		echo ""
	fi
done

echo "***********************************************************************"
echo "* GET MSG ID LIST FROM DB:: Get all received message IDs (via all accounts)."
echo "***********************************************************************"
for login in $USERNAMES; do
	"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$login'" \
		--get-msg-ids "dmType='received'" \
		2>/dev/null
	if [ 0 != $? ]; then
		echo "GetMsgIDs-received: $login - ERROR"
		echo ""
		exit
	else
		echo "GetMsgIDs-received: $login - OK"
		echo ""
	fi
done

echo "***********************************************************************"
echo "* GET MSG ID LIST FROM DB:: Get all sent message IDs (via all accounts)."
echo "***********************************************************************"
for login in $USERNAMES; do
	"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$login'" \
		--get-msg-ids "dmType='sent'" \
		2>/dev/null
	if [ 0 != $? ]; then
		echo "GetMsgIDs-sent: $login - ERROR"
		echo ""
		exit
	else
		echo "GetMsgIDs-sent: $login - OK"
		echo ""
	fi
done

echo ""
echo ""
echo "------------------------------------------------------------------------"
echo "CONGRATULATION: All get/download message tests were done with success."
echo "------------------------------------------------------------------------"
echo ""
