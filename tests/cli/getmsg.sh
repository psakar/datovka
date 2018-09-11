#!/usr/bin/env bash

SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "${SCRIPT}")

CMDARGS="${CMDARGS} -D"
CMDARGS="${CMDARGS} --conf-subdir .dsgui"
CMDARGS="${CMDARGS} --debug-verbosity 2"
CMDARGS="${CMDARGS} --log-verbosity 2"

. "${SCRIPTPATH}/../../untracked/logins.sh"

# You can change save location
AT_FILE_SAVE_PATH="${SCRIPTPATH}/../../tmp/"

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

rm -rf $AT_FILE_SAVE_PATH
mkdir $AT_FILE_SAVE_PATH

if [[ "$OSTYPE" == "linux-gnu" ]]; then
	OS_NAME="Linux"
	APP_BINARY_NAME="datovka"
	ATTACH_LOAD_PATH="${SCRIPTPATH}/attachment"
	APP_PATH="${SCRIPTPATH}/../.."
	ATTACH_SAVE_PATH="${AT_FILE_SAVE_PATH}"
elif [[ "$OSTYPE" == "darwin"* ]]; then
	OS_NAME="MacOS"
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
	print_error "ERROR: Unknown platform"
	exit 1
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
		print_error "GetMsgList (received): $login - ERROR"		
	else
		print_success "GetMsgList (received): $login - OK $MSGIDS"
	fi
	MSGIDS=`"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$login'" \
		--get-msg-list "dmType='sent'" \
		2>/dev/null`
	if [ 0 != $? ]; then
		print_error "GetMsgList (sent): $login - ERROR"
	else
		print_success "GetMsgList (sent): $login - OK $MSGID"
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
		print_error "GetMsgList (received) + complete download: $login - ERROR"
	else
		print_success "GetMsgList (received) + complete download: $login - OK $MSGIDS"
	fi
	MSGIDS=`"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$login'" \
		--get-msg-list "dmType='sent',complete='yes'" \
		2>/dev/null`
	if [ 0 != $? ]; then
		print_error "GetMsgList (sent) + complete download: $login - ERROR"
	else
		print_success "GetMsgList (sent) + complete download: $login - OK $MSGID"
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
DMANNOTATION="Datovka: test CLI ${OS_NAME} - ${DTIME}"
DMATACHMENT="${ATTACH_LOAD_PATH}/dokument.odt;${ATTACH_LOAD_PATH}/dokument.pdf;${ATTACH_LOAD_PATH}/obrazek.png"
MSGID=`"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$USERNAME_SEND'" \
	--send-msg "dbIDRecipient='$RECIPIENT_SEND',dmAnnotation='${DMANNOTATION}',dmAttachment='${DMATACHMENT}'" \
	2>/dev/null`
if [ 0 != $? ]; then
	print_error "SendMsg: $USERNAME_SEND - ERROR"
else
	print_success "SendMsg: $USERNAME_SEND, msgID: '$MSGID' - OK"
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
	print_error "GetMsgList (received): $USERNAME_SEND2 - ERROR"
else
	print_success "GetMsgList (received): $USERNAME_SEND2 - OK $RMSGIDS"
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
		print_success "GetMsgFromDb '$dmID': $USERNAME_SEND2 - failed - it is OK"
	else
		print_error "GetMsgFromDb '$dmID': $USERNAME_SEND2 - ERROR"
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
		print_success "GetMsgDelInfo '$dmID': $USERNAME_SEND2 - failed - it is OK"
	else
		print_error "GetMsgDelInfo '$dmID': $USERNAME_SEND2 - ERROR"
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
		print_error "GetMsgFromISDS '$dmID': $USERNAME_SEND2 - ERROR"
	else
		print_success "GetMsgFromISDS '$dmID': $USERNAME_SEND2 - OK"
		print_success "ExportToZFO '$dmID': $USERNAME_SEND2 - OK"
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
		print_error "GetMsgDelInfoFromISDS '$dmID': $USERNAME_SEND2- ERROR"
	else
		print_success "GetMsgDelInfoFromISDS '$dmID': $USERNAME_SEND2 - OK"
		print_success "ExportToZFO '$dmID': $USERNAME_SEND2 - OK"
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
		print_error "GetMsgFromDb '$dmID': $USERNAME_SEND2 - ERROR"
	else
		print_success "GetMsgFromDb '$dmID': $USERNAME_SEND2 - OK"
		print_success "ExportToZFO '$dmID': $USERNAME_SEND2 - OK"
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
		print_error "GetMsgDelInfoFromDb '$dmID': $USERNAME_SEND2 - ERROR"
	else
		print_success "GetMsgDelInfoFromDb '$dmID': $USERNAME_SEND2 - OK"
		print_success "ExportToZFO '$dmID': $USERNAME_SEND2 - OK"
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
	print_error "GetMsgList (sent): $USERNAME_SEND - ERROR"
else
	print_success "GetMsgList (sent): $USERNAME_SEND - OK $SMSGIDS"
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
		print_success "GetMsgFromDb '$dmID': $USERNAME_SEND - failed - it is OK"
	else
		print_error "GetMsgFromDb '$dmID': $USERNAME_SEND - ERROR"
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
		print_success "GetMsgDelInfo '$dmID': $USERNAME_SEND - failed - it is OK"
	else
		print_error "GetMsgDelInfo '$dmID': $USERNAME_SEND - ERROR"
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
		print_error "GetMsgFromISDS '$dmID': $USERNAME_SEND - ERROR"
	else
		print_success "GetMsgFromISDS '$dmID': $USERNAME_SEND - OK"
		print_success "ExportToZFO '$dmID': $USERNAME_SEND - OK"
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
		print_error "GetMsgDelInfoFromISDS '$dmID': $USERNAME_SEND - ERROR"
	else
		print_success "GetMsgDelInfoFromISDS '$dmID': $USERNAME_SEND - OK"
		print_success "ExportToZFO '$dmID': $USERNAME_SEND - OK"
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
		print_error "GetMsgFromDb '$dmID': $USERNAME_SEND - ERROR"
	else
		print_success "GetMsgFromDb '$dmID': $USERNAME_SEND - OK"
		print_success "ExportToZFO '$dmID': $USERNAME_SEND - OK"
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
		print_error "GetMsgDelInfoFromDb '$dmID': $USERNAME_SEND - ERROR"
	else
		print_success "GetMsgDelInfoFromDb '$dmID': $USERNAME_SEND - OK"
		print_success "ExportToZFO '$dmID': $USERNAME_SEND - OK"
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
		print_error "GetMsgFromISDS '$dmID': $USERNAME_SEND2 - ERROR"
	else
		print_success "GetMsgFromISDS '$dmID': $USERNAME_SEND2 - OK"
		print_success "ExportToZFO '$dmID': $USERNAME_SEND2 - OK"
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
		print_error "GetMsgDelInfoFromISDS '$dmID': $USERNAME_SEND2 - ERROR"
	else
		print_success "GetMsgDelInfoFromISDS '$dmID': $USERNAME_SEND2 - OK"
		print_success "ExportToZFO '$dmID': $USERNAME_SEND2 - OK"
	fi
done

echo ""
echo "***********************************************************************"
echo "* GET MSG LIST:: Check messages where attachment missing (via all accounts)."
echo "***********************************************************************"
for login in $USERNAMES; do
	"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$login'" \
		--check-attachment \
		2>/dev/null
	if [ 0 != $? ]; then
		print_error "CheckAttach: $login - ERROR"
		echo ""
	else
		print_success "CheckAttach: $login - OK"
		echo ""
	fi
done

echo ""
echo "***********************************************************************"
echo "* GET MSG ID LIST FROM DB:: Get all received message IDs (via all accounts)."
echo "***********************************************************************"
for login in $USERNAMES; do
	"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$login'" \
		--get-msg-ids "dmType='received'" \
		2>/dev/null
	if [ 0 != $? ]; then
		print_error "GetMsgIDs-received: $login - ERROR"
		echo ""
	else
		print_success "GetMsgIDs-received: $login - OK"
		echo ""
	fi
done

echo ""
echo "***********************************************************************"
echo "* GET MSG ID LIST FROM DB:: Get all sent message IDs (via all accounts)."
echo "***********************************************************************"
for login in $USERNAMES; do
	"${APP_PATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$login'" \
		--get-msg-ids "dmType='sent'" \
		2>/dev/null
	if [ 0 != $? ]; then
		print_error "GetMsgIDs-sent: $login - ERROR"
		echo ""
	else
		print_success "GetMsgIDs-sent: $login - OK"
		echo ""
	fi
done

if [ "$SOME_ERROR" = false ] ; then
	echo ""
	print_success "-----------------------------------------------------------------------"
	print_success "SUCCESS: All get message list and download message tests were done."
	print_success "-----------------------------------------------------------------------"
	echo ""
	exit 0
else 
	echo ""
	print_error "-----------------------------------------------------------------------"
	print_error "FAIL: Some get message list and download message tests have been failed!"
	print_error "-----------------------------------------------------------------------"
	echo ""
	exit 1
fi
