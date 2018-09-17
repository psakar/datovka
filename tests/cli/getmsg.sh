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
ATTACH_SAVE_DIR="$4"
if [ "x${ATTACH_SAVE_DIR}" = "x" -o ! -d "${ATTACH_SAVE_DIR}" ]; then
	echo_error "ERROR: cannot access directory which to save attachments into."
	exit 1
fi

CMDARGS="${CMDARGS} -D"
CMDARGS="${CMDARGS} --conf-subdir .dsgui"
CMDARGS="${CMDARGS} --debug-verbosity 2"
CMDARGS="${CMDARGS} --log-verbosity 2"


echo ""
echo "***********************************************************************"
echo "MSGLIST TEST: Get lists of received and sent messages"
echo "              for selected accounts."
echo "***********************************************************************"
#---Get message list for account with username and pwd---
for login in $USERNAMES_MSGLIST_ONLY; do
	MSGIDS=$("${APP_BINARY_PATH}" ${CMDARGS} \
		--login "username='$login'" \
		--get-msg-list "dmType='received'" \
		2>/dev/null)
	if [ 0 != $? ]; then
		echo_error "GetMsgList (received): $login - ERROR"
	else
		echo_success "GetMsgList (received): $login - OK $MSGIDS"
	fi
	MSGIDS=$("${APP_BINARY_PATH}" ${CMDARGS} \
		--login "username='$login'" \
		--get-msg-list "dmType='sent'" \
		2>/dev/null)
	if [ 0 != $? ]; then
		echo_error "GetMsgList (sent): $login - ERROR"
	else
		echo_success "GetMsgList (sent): $login - OK $MSGID"
	fi
done


echo ""
echo "***********************************************************************"
echo "MSGLIST TEST: Get lists of received and sent messages and complete"
echo "              messages for selected accounts."
echo "***********************************************************************"
#---Get message list for account with username and pwd---
for login in $USERNAMES_MSGLIST_COMPLETE; do
	MSGIDS=$("${APP_BINARY_PATH}" ${CMDARGS} \
		--login "username='$login'" \
		--get-msg-list "dmType='received',complete='yes'" \
		2>/dev/null)
	if [ 0 != $? ]; then
		echo_error "GetMsgList (received) + complete download: $login - ERROR"
	else
		echo_success "GetMsgList (received) + complete download: $login - OK $MSGIDS"
	fi
	MSGIDS=$("${APP_BINARY_PATH}" ${CMDARGS} \
		--login "username='$login'" \
		--get-msg-list "dmType='sent',complete='yes'" \
		2>/dev/null)
	if [ 0 != $? ]; then
		echo_error "GetMsgList (sent) + complete download: $login - ERROR"
	else
		echo_success "GetMsgList (sent) + complete download: $login - OK $MSGID"
	fi
done


echo ""
echo "***********************************************************************"
echo "MSGLIST TEST: Create and send a new message from an account with"
echo "              a usename and password and download this message"
echo "              using the recipient account, save the attachments"
echo "              and export to ZFO."
echo "***********************************************************************"
echo "---Create and send a new message from user '$USERNAME_SEND'---"
DTIME=$(date +"%Y-%m-%d %T")
DMANNOTATION="Datovka: test CLI ${OS_NAME} - ${DTIME}"
DMATACHMENT="${ATTACH_LOAD_PATH}/dokument.odt;${ATTACH_LOAD_PATH}/dokument.pdf;${ATTACH_LOAD_PATH}/obrazek.png"
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
echo "Waiting for the ISDS server - 5 seconds ..."
sleep 5

#----must be success and return msg ID/IDs
echo ""
echo "---Get received message list for user '$USERNAME_SEND2'---"
#----must be success and return msg ID/IDs
RMSGIDS=$("${APP_BINARY_PATH}" ${CMDARGS} \
	--login "username='$USERNAME_SEND2'" \
	--get-msg-list "dmType='received'" \
	2>/dev/null)
if [ 0 != $? ]; then
	echo_error "GetMsgList (received): $USERNAME_SEND2 - ERROR"
else
	echo_success "GetMsgList (received): $USERNAME_SEND2 - OK $RMSGIDS"
fi

echo ""
#----Export complete new messages from database------------------------------
#----must fails
for dmID in $RMSGIDS; do
	RET=$("${APP_BINARY_PATH}" ${CMDARGS} \
		--login "username='$USERNAME_SEND2'" \
		--get-msg "dmID='$dmID',dmType='received',download='no',zfoFile='${ATTACH_SAVE_DIR}/DMr_$dmID.zfo'" \
		2>/dev/null)
	if [ 0 != $? ]; then
		echo_success "GetMsgFromDb '$dmID': $USERNAME_SEND2 - failed - it is OK"
	else
		echo_error "GetMsgFromDb '$dmID': $USERNAME_SEND2 - ERROR"
	fi
done

#----Export delivery info of new messages from database----------------------
#----must fails
for dmID in $RMSGIDS; do
	RET=$("${APP_BINARY_PATH}" ${CMDARGS} \
		--login "username='$USERNAME_SEND2'" \
		--get-delivery-info "dmID='$dmID',download='no',zfoFile='${ATTACH_SAVE_DIR}/DMr-info_$dmID.zfo'" \
		2>/dev/null)
	if [ 0 != $? ]; then
		echo_success "GetMsgDelInfo '$dmID': $USERNAME_SEND2 - failed - it is OK"
	else
		echo_error "GetMsgDelInfo '$dmID': $USERNAME_SEND2 - ERROR"
	fi
done

#-----Download complete new messages ISDS-------------------------------------
#----must be success and save zfo file
for dmID in $RMSGIDS; do
	RET=$("${APP_BINARY_PATH}" ${CMDARGS} \
		--login "username='$USERNAME_SEND2'" \
		--get-msg "dmID='$dmID',dmType='received',zfoFile='${ATTACH_SAVE_DIR}/DMr_$dmID-isds.zfo'" \
		2>/dev/null)
	if [ 0 != $? ]; then
		echo_error "GetMsgFromISDS '$dmID': $USERNAME_SEND2 - ERROR"
	else
		echo_success "GetMsgFromISDS '$dmID': $USERNAME_SEND2 - OK"
		echo_success "ExportToZFO '$dmID': $USERNAME_SEND2 - OK"
	fi
done

#----Download delivery info of new messages from ISDS------------------------
#----must be success and save zfo file
for dmID in $RMSGIDS; do
	RET=$("${APP_BINARY_PATH}" ${CMDARGS} \
		--login "username='$USERNAME_SEND2'" \
		--get-delivery-info "dmID='$dmID',zfoFile='${ATTACH_SAVE_DIR}/DMr-info_$dmID-isds.zfo'" \
		2>/dev/null)
	if [ 0 != $? ]; then
		echo_error "GetMsgDelInfoFromISDS '$dmID': $USERNAME_SEND2- ERROR"
	else
		echo_success "GetMsgDelInfoFromISDS '$dmID': $USERNAME_SEND2 - OK"
		echo_success "ExportToZFO '$dmID': $USERNAME_SEND2 - OK"
	fi
done

#----Export complete messages from database again-----------------------
#----must be success and save zfo file and save attachment
for dmID in $RMSGIDS; do
	RET=$("${APP_BINARY_PATH}" ${CMDARGS} \
		--login "username='$USERNAME_SEND2'" \
		--get-msg "dmID='$dmID',dmType='received',download='no',zfoFile='${ATTACH_SAVE_DIR}/DMr_$dmID-db.zfo',attachmentDir='${ATTACH_SAVE_DIR}'" \
		2>/dev/null)
	if [ 0 != $? ]; then
		echo_error "GetMsgFromDb '$dmID': $USERNAME_SEND2 - ERROR"
	else
		echo_success "GetMsgFromDb '$dmID': $USERNAME_SEND2 - OK"
		echo_success "ExportToZFO '$dmID': $USERNAME_SEND2 - OK"
	fi
done

#----Export delivery info of messages from again----------------------
#----must be success and save zfo file
for dmID in $RMSGIDS; do
	RET=$("${APP_BINARY_PATH}" ${CMDARGS} \
		--login "username='$USERNAME_SEND2'" \
		--get-delivery-info "dmID='$dmID',download='no',zfoFile='${ATTACH_SAVE_DIR}/DMr-info_$dmID-db.zfo'" \
		2>/dev/null)
	if [ 0 != $? ]; then
		echo_error "GetMsgDelInfoFromDb '$dmID': $USERNAME_SEND2 - ERROR"
	else
		echo_success "GetMsgDelInfoFromDb '$dmID': $USERNAME_SEND2 - OK"
		echo_success "ExportToZFO '$dmID': $USERNAME_SEND2 - OK"
	fi
done

#
#
#-----Get sent message list-------------------------------------------------
#
#
echo ""
echo "---Get sent message list for user '$USERNAME_SEND'---"
SMSGIDS=$("${APP_BINARY_PATH}" ${CMDARGS} \
	--login "username='$USERNAME_SEND'" \
	--get-msg-list "dmType='sent'" \
	2>/dev/null)
if [ 0 != $? ]; then
	echo_error "GetMsgList (sent): $USERNAME_SEND - ERROR"
else
	echo_success "GetMsgList (sent): $USERNAME_SEND - OK $SMSGIDS"
fi
#--------------------------------------------------------------------------


echo ""
echo "---Download messages newly sent to user '$USERNAME_SEND'---"
#----Export complete new messages from database------------------------------
#----must fails
for dmID in $RMSGIDS; do
	RET=$("${APP_BINARY_PATH}" ${CMDARGS} \
	--login "username='$USERNAME_SEND'" \
		--get-msg "dmID='$dmID',dmType='sent',download='no',zfoFile='${ATTACH_SAVE_DIR}/DMs_$dmID.zfo'" \
		2>/dev/null)
	if [ 0 != $? ]; then
		echo_success "GetMsgFromDb '$dmID': $USERNAME_SEND - failed - it is OK"
	else
		echo_error "GetMsgFromDb '$dmID': $USERNAME_SEND - ERROR"
	fi
done

#----Export delivery info of new messages from database----------------------
#----must fails
for dmID in $RMSGIDS; do
	RET=$("${APP_BINARY_PATH}" ${CMDARGS} \
	--login "username='$USERNAME_SEND'" \
		--get-delivery-info "dmID='$dmID',download='no',zfoFile='${ATTACH_SAVE_DIR}/DMs-info_$dmID.zfo'" \
		2>/dev/null)
	if [ 0 != $? ]; then
		echo_success "GetMsgDelInfo '$dmID': $USERNAME_SEND - failed - it is OK"
	else
		echo_error "GetMsgDelInfo '$dmID': $USERNAME_SEND - ERROR"
	fi
done

#-----Download complete new messages ISDS-------------------------------------
#----must be success and save zfo file
for dmID in $RMSGIDS; do
	RET=$("${APP_BINARY_PATH}" ${CMDARGS} \
	--login "username='$USERNAME_SEND'" \
		--get-msg "dmID='$dmID',dmType='sent',zfoFile='${ATTACH_SAVE_DIR}/DMs_$dmID-isds.zfo'" \
		2>/dev/null)
	if [ 0 != $? ]; then
		echo_error "GetMsgFromISDS '$dmID': $USERNAME_SEND - ERROR"
	else
		echo_success "GetMsgFromISDS '$dmID': $USERNAME_SEND - OK"
		echo_success "ExportToZFO '$dmID': $USERNAME_SEND - OK"
	fi
done

#----Download delivery info of new messages from ISDS------------------------
#----must be success and save zfo file
for dmID in $RMSGIDS; do
	RET=$("${APP_BINARY_PATH}" ${CMDARGS} \
	--login "username='$USERNAME_SEND'" \
		--get-delivery-info "dmID='$dmID',zfoFile='${ATTACH_SAVE_DIR}/DMs-info_$dmID-isds.zfo'" \
		2>/dev/null)
	if [ 0 != $? ]; then
		echo_error "GetMsgDelInfoFromISDS '$dmID': $USERNAME_SEND - ERROR"
	else
		echo_success "GetMsgDelInfoFromISDS '$dmID': $USERNAME_SEND - OK"
		echo_success "ExportToZFO '$dmID': $USERNAME_SEND - OK"
	fi
done

#----Export complete messages from database again-----------------------
#----must be success and save zfo file
for dmID in $RMSGIDS; do
	RET=$("${APP_BINARY_PATH}" ${CMDARGS} \
	--login "username='$USERNAME_SEND'" \
		--get-msg "dmID='$dmID',dmType='sent',download='no',zfoFile='${ATTACH_SAVE_DIR}/DMs_$dmID-db.zfo'" \
		2>/dev/null)
	if [ 0 != $? ]; then
		echo_error "GetMsgFromDb '$dmID': $USERNAME_SEND - ERROR"
	else
		echo_success "GetMsgFromDb '$dmID': $USERNAME_SEND - OK"
		echo_success "ExportToZFO '$dmID': $USERNAME_SEND - OK"
	fi
done

#----Export delivery info of messages from again----------------------
#----must be success and save zfo file
for dmID in $RMSGIDS; do
	RET=$("${APP_BINARY_PATH}" ${CMDARGS} \
	--login "username='$USERNAME_SEND'" \
		--get-delivery-info "dmID='$dmID',download='no',zfoFile='${ATTACH_SAVE_DIR}/DMs-info_$dmID-db.zfo'" \
		2>/dev/null)
	if [ 0 != $? ]; then
		echo_error "GetMsgDelInfoFromDb '$dmID': $USERNAME_SEND - ERROR"
	else
		echo_success "GetMsgDelInfoFromDb '$dmID': $USERNAME_SEND - OK"
		echo_success "ExportToZFO '$dmID': $USERNAME_SEND - OK"
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
	RET=$("${APP_BINARY_PATH}" ${CMDARGS} \
		--login "username='$USERNAME_SEND2'" \
		--get-msg "dmID='$dmID',dmType='received',markDownload='yes',zfoFile='${ATTACH_SAVE_DIR}/DMs_$dmID-isds.zfo'" \
		2>/dev/null)
	if [ 0 != $? ]; then
		echo_error "GetMsgFromISDS '$dmID': $USERNAME_SEND2 - ERROR"
	else
		echo_success "GetMsgFromISDS '$dmID': $USERNAME_SEND2 - OK"
		echo_success "ExportToZFO '$dmID': $USERNAME_SEND2 - OK"
	fi
done

#----Download delivery info of new messages from ISDS------------------------
#----must be success and save zfo file
for dmID in $RMSGIDS; do
	RET=$("${APP_BINARY_PATH}" ${CMDARGS} \
		--login "username='$USERNAME_SEND2'" \
		--get-delivery-info "dmID='$dmID',zfoFile='${ATTACH_SAVE_DIR}/DMs-info_$dmID-isds.zfo'" \
		2>/dev/null)
	if [ 0 != $? ]; then
		echo_error "GetMsgDelInfoFromISDS '$dmID': $USERNAME_SEND2 - ERROR"
	else
		echo_success "GetMsgDelInfoFromISDS '$dmID': $USERNAME_SEND2 - OK"
		echo_success "ExportToZFO '$dmID': $USERNAME_SEND2 - OK"
	fi
done

echo ""
echo "***********************************************************************"
echo "* GET MSG LIST:: Check messages with missing attachments (all accounts)."
echo "***********************************************************************"
for login in $USERNAMES; do
	"${APP_BINARY_PATH}" ${CMDARGS} \
		--login "username='$login'" \
		--check-attachment \
		2>/dev/null
	if [ 0 != $? ]; then
		echo_error "CheckAttach: $login - ERROR"
		echo ""
	else
		echo_success "CheckAttach: $login - OK"
		echo ""
	fi
done

echo ""
echo "***********************************************************************"
echo "* GET MSG ID LIST FROM DB:: Get all received message IDs (all accounts)."
echo "***********************************************************************"
for login in $USERNAMES; do
	"${APP_BINARY_PATH}" ${CMDARGS} \
		--login "username='$login'" \
		--get-msg-ids "dmType='received'" \
		2>/dev/null
	if [ 0 != $? ]; then
		echo_error "GetMsgIDs-received: $login - ERROR"
		echo ""
	else
		echo_success "GetMsgIDs-received: $login - OK"
		echo ""
	fi
done

echo ""
echo "***********************************************************************"
echo "* GET MSG ID LIST FROM DB:: Get all sent message IDs (all accounts)."
echo "***********************************************************************"
for login in $USERNAMES; do
	"${APP_BINARY_PATH}" ${CMDARGS} \
		--login "username='$login'" \
		--get-msg-ids "dmType='sent'" \
		2>/dev/null
	if [ 0 != $? ]; then
		echo_error "GetMsgIDs-sent: $login - ERROR"
		echo ""
	else
		echo_success "GetMsgIDs-sent: $login - OK"
		echo ""
	fi
done

if [ "x${HAVE_ERROR}" = "xfalse" ]; then
	echo ""
	echo_success "-----------------------------------------------------------------------"
	echo_success "SUCCESS: All get message list and download message tests finished as expected."
	echo_success "-----------------------------------------------------------------------"
	echo ""
	exit 0
else
	echo ""
	echo_error "-----------------------------------------------------------------------"
	echo_error "FAILURE: Some get message list and download message tests have failed!"
	echo_error "-----------------------------------------------------------------------"
	echo ""
	exit 1
fi
