#!/usr/bin/env sh

#pwd

SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "${SCRIPT}")

#echo ${SCRIPTPATH}
CMDARGS="${CMDARGS} -D"
CMDARGS="${CMDARGS} --conf-subdir .dsgui"
CMDARGS="${CMDARGS} --debug-verbosity 2"
#CMDARGS="${CMDARGS} --log-verbosity 2"

APP_BINARY_NAME="/../../datovka"
ATTACH_LOAD_PATH="${SCRIPTPATH}/attachment"
ATTACH_SAVE_PATH="${SCRIPTPATH}/../../tmp"
. "${SCRIPTPATH}/../../untracked/logins.sh"

rm -rf $ATTACH_SAVE_PATH
mkdir $ATTACH_SAVE_PATH

echo ""
echo "***********************************************************************"
echo "TEST 01a: Login into databox for all existing accounts where"
echo "          username and password is used and remembered." 
echo "***********************************************************************"
for login in $USER_LOGINS; do
	"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$login'" \
		2>/dev/null
	if [ 0 != $? ]; then
		echo "Login: '$login' - ERROR: account not exists or required data missing!"
		exit
	else
		echo "Login: '$login' - OK"
	fi
done

echo ""
echo "***********************************************************************"
echo "TEST 01b: Login into databox with certificate."
echo "          Note: certificate password is requried."
echo "***********************************************************************"
"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$CERT_LOGIN',otpcode='$CERT_PWD'" \
	2>/dev/null
if [ 0 != $? ]; then
	echo "Login: '$CERT_LOGIN' - ERROR: account not exists or required data missing!"
	exit
else
	echo "Login: $CERT_LOGIN - OK"
fi

echo ""
echo "***********************************************************************"
echo "TEST 01c: Login into databox for all existing accounts where"
echo "          user has restricted privilegies." 
echo "***********************************************************************"
for login in $RESTRICT_LOGINS; do
	"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$login'" \
		2>/dev/null
	if [ 0 != $? ]; then
		echo "Login: '$login' - ERROR: account not exists or required data missing!"
		exit
	else
		echo "Login: '$login' - OK"
	fi
done

echo ""
echo "***********************************************************************"
echo "TEST 01d: Login into databox where password"
echo "          is not stored in the dsgui.conf."
echo "***********************************************************************"
"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$INCOMP_LOGIN',password='$INCOMP_PWD'" \
	2>/dev/null
if [ 0 != $? ]; then
	echo "Login: '$INCOMP_LOGIN' - ERROR: account not exists or required data missing!"
	exit
else
	echo "Login: $INCOMP_LOGIN - OK: external password was used."
fi

echo ""
echo "***********************************************************************"
echo "TEST 01e: Login into databox which does not exists in dsgui.conf."
echo "***********************************************************************"
for login in $USER_NOEXIST_LOGINS; do
	"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$login'" \
		2>/dev/null
	if [ 0 != $? ]; then
		echo "Login: '$login' - OK: account not exists"
	else
		echo "Login: '$login' - ERROR: account exists and it is wrong!"
		exit
	fi
done


echo ""
echo "***********************************************************************"
echo "TEST 02a: Obtaining user info for exist accounts."
echo "***********************************************************************"
#---Get user info for account with username and pwd---
for login in $USER_LOGINS; do
	"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$login'" \
		--get-user-info \
		2>/dev/null
	if [ 0 != $? ]; then
		echo "User info: $login - ERROR"
		exit
	else
		echo "User info: $login - OK"
	fi
done
#---Get user info for account with certificate---
"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$CERT_LOGIN',otpcode='$CERT_PWD'" \
	--get-user-info \
	2>/dev/null
if [ 0 != $? ]; then
	echo "User info: $CERT_LOGIN - ERROR"
	exit
else
	echo "User info: $CERT_LOGIN - OK"
fi

echo ""
echo "***********************************************************************"
echo "TEST 02b: Obtaining owner info for exist accounts."
echo "***********************************************************************"
#---Get owner info for account with username and pwd---
for login in $USER_LOGINS; do
	"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$login'" \
		--get-owner-info \
		2>/dev/null
	if [ 0 != $? ]; then
		echo "Owner info: $login - ERROR"
		exit
	else
		echo "Owner info: $login - OK"
	fi
done
#---Get owner info for account with certificate---
"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$CERT_LOGIN',otpcode='$CERT_PWD'" \
	--get-owner-info \
	2>/dev/null
if [ 0 != $? ]; then
	echo "Owner info: $CERT_LOGIN - ERROR"
	exit
else
	echo "Owner info: $CERT_LOGIN - OK"
fi


echo ""
echo "***********************************************************************"
echo "TEST 03: Get received/sent message list for all accounts."
echo "***********************************************************************"
#---Get message list for account with username and pwd---
for login in $USER_LOGINS; do
	MSGIDS=`"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$login'" \
		--get-msg-list "dmType='received'" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgList (received): $login - ERROR"
		exit
	else
		echo "GetMsgList (received): $login - OK $MSGIDS"
	fi

	MSGIDS=`"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
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
#---Get message list for account with certificate---
MSGIDS=`"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$CERT_LOGIN',otpcode='$CERT_PWD'" \
	--get-msg-list "dmType='all'" \
	2>/dev/null`
if [ 0 != $? ]; then
	echo "GetMsgList (all): $CERT_LOGIN - ERROR"
	exit
else
	echo "GetMsgList (all): $CERT_LOGIN - OK $MSGIDS"
fi



echo ""
echo "***********************************************************************"
echo "TEST 04a: Create and send a new message from account with"
echo "          usename and password and download this message"
echo "          by recipient and export to ZFO."
echo "***********************************************************************"
echo "---Create and send a new message from user '$TEST_USER_SEND1'---"
DTIME=$(date +"%Y-%m-%d %T")
DMANNOTATION="Datovka - test CLI - ${DTIME}"
DMATACHMENT="${ATTACH_LOAD_PATH}/dokument.odt;${ATTACH_LOAD_PATH}/dokument.pdf;${ATTACH_LOAD_PATH}/notification.mp3;${ATTACH_LOAD_PATH}/obrazek.jpg;\
${ATTACH_LOAD_PATH}/obrazek.png;${ATTACH_LOAD_PATH}/datova-zprava.zfo"
MSGID=`"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$TEST_USER_SEND1'" \
	--send-msg "dbIDRecipient='$TEST_RECIP_SEND1',dmAnnotation='${DMANNOTATION}',dmAttachment='${DMATACHMENT}'" \
	2>/dev/null`
if [ 0 != $? ]; then
	echo "SendMsg: $TEST_USER_SEND1 - ERROR"
	exit
else
	echo "SendMsg: $TEST_USER_SEND1, msgID: '$MSGID' - OK"
fi

echo ""
echo "Waiting for the server DS - 10 seconds ..."
sleep 10

#----must be success and return msg ID/IDs
echo ""
echo "---Get received message list for user '$CERT_LOGIN'---"
#----must be success and return msg ID/IDs
RMSGIDS=`"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$CERT_LOGIN',otpcode='$CERT_PWD'" \
	--get-msg-list "dmType='received'" \
	2>/dev/null`
if [ 0 != $? ]; then
	echo "GetMsgList (received): $CERT_LOGIN - ERROR"
	exit
else
	echo "GetMsgList (received): $CERT_LOGIN - OK $RMSGIDS"
fi

echo ""
#----Export complete new messages from database------------------------------
#----must fails
for dmID in $RMSGIDS; do
	RET=`"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$CERT_LOGIN',otpcode='$CERT_PWD'" \
		--get-msg "dmID='$dmID',dmType='received',download='no',zfoFile='${ATTACH_SAVE_PATH}/DMr_$dmID.zfo" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgFromDb '$dmID': $CERT_LOGIN - failed - it is OK"
	else
		echo "GetMsgFromDb '$dmID': $CERT_LOGIN - ERROR"
		exit
	fi
done

#----Export delivery info of new messages from database----------------------
#----must fails
for dmID in $RMSGIDS; do
	RET=`"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$CERT_LOGIN',otpcode='$CERT_PWD'" \
		--get-delivery-info "dmID='$dmID',download='no',zfoFile='${ATTACH_SAVE_PATH}/DMr-info_$dmID.zfo" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgDelInfo '$dmID': $CERT_LOGIN - failed - it is OK"
	else
		echo "GetMsgDelInfo '$dmID': $CERT_LOGIN - ERROR"
		exit
	fi
done

#-----Download complete new messages ISDS-------------------------------------
#----must be success and save zfo file
for dmID in $RMSGIDS; do
	RET=`"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$CERT_LOGIN',otpcode='$CERT_PWD'" \
		--get-msg "dmID='$dmID',dmType='received',zfoFile='${ATTACH_SAVE_PATH}/DMr_$dmID-isds.zfo" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgFromISDS '$dmID': $CERT_LOGIN - ERROR"
		exit
	else
		echo "GetMsgFromISDS '$dmID': $CERT_LOGIN - OK"
		echo "ExportToZFO '$dmID': $CERT_LOGIN - OK"
	fi
done

#----Download delivery info of new messages from ISDS------------------------
#----must be success and save zfo file
for dmID in $RMSGIDS; do
	RET=`"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$CERT_LOGIN',otpcode='$CERT_PWD'" \
		--get-delivery-info "dmID='$dmID',zfoFile='${ATTACH_SAVE_PATH}/DMr-info_$dmID-isds.zfo" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgDelInfoFromISDS '$dmID': $CERT_LOGIN - ERROR"
		exit
	else
		echo "GetMsgDelInfoFromISDS '$dmID': $CERT_LOGIN - OK"
		echo "ExportToZFO '$dmID': $CERT_LOGIN - OK"
	fi
done

#----Export complete messages from database again-----------------------
#----must be success and save zfo file
for dmID in $RMSGIDS; do
	RET=`"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$CERT_LOGIN',otpcode='$CERT_PWD'" \
		--get-msg "dmID='$dmID',dmType='received',download='no',zfoFile='${ATTACH_SAVE_PATH}/DMr_$dmID-db.zfo" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgFromDb '$dmID': $CERT_LOGIN - ERROR"
		exit
	else
		echo "GetMsgFromDb '$dmID': $CERT_LOGIN - OK"
		echo "ExportToZFO '$dmID': $CERT_LOGIN - OK"
	fi
done

#----Export delivery info of messages from again----------------------
#----must be success and save zfo file
for dmID in $RMSGIDS; do
	RET=`"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$CERT_LOGIN',otpcode='$CERT_PWD'" \
		--get-delivery-info "dmID='$dmID',download='no',zfoFile='${ATTACH_SAVE_PATH}/DMr-info_$dmID-db.zfo" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgDelInfoFromDb '$dmID': $CERT_LOGIN - ERROR"
		exit
	else
		echo "GetMsgDelInfoFromDb '$dmID': $CERT_LOGIN - OK"
		echo "ExportToZFO '$dmID': $CERT_LOGIN - OK"
	fi
done


#
#
#-----Get sent message list-------------------------------------------------
#
#
echo ""
echo "---Get sent message list for user '$TEST_USER_SEND1'---"
SMSGIDS=`"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$TEST_USER_SEND1'" \
	--get-msg-list "dmType='sent'" \
	2>/dev/null`
if [ 0 != $? ]; then
	echo "GetMsgList (sent): $TEST_USER_SEND1 - ERROR"
	exit
else
	echo "GetMsgList (sent): $TEST_USER_SEND1 - OK $SMSGIDS"
fi
#--------------------------------------------------------------------------



echo ""
echo "---Download new sent messages for user '$TEST_USER_SEND1'---"
#----Export complete new messages from database------------------------------
#----must fails
for dmID in $RMSGIDS; do
	RET=`"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$TEST_USER_SEND1'" \
		--get-msg "dmID='$dmID',dmType='sent',download='no',zfoFile='${ATTACH_SAVE_PATH}/DMs_$dmID.zfo" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgFromDb '$dmID': $TEST_USER_SEND1 - failed - it is OK"
	else
		echo "GetMsgFromDb '$dmID': $TEST_USER_SEND1 - ERROR"
		exit
	fi
done

#----Export delivery info of new messages from database----------------------
#----must fails
for dmID in $RMSGIDS; do
	RET=`"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$TEST_USER_SEND1'" \
		--get-delivery-info "dmID='$dmID',download='no',zfoFile='${ATTACH_SAVE_PATH}/DMs-info_$dmID.zfo" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgDelInfo '$dmID': $TEST_USER_SEND1 - failed - it is OK"
	else
		echo "GetMsgDelInfo '$dmID': $TEST_USER_SEND1 - ERROR"
		exit
	fi
done

#-----Download complete new messages ISDS-------------------------------------
#----must be success and save zfo file
for dmID in $RMSGIDS; do
	RET=`"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$TEST_USER_SEND1'" \
		--get-msg "dmID='$dmID',dmType='sent',zfoFile='${ATTACH_SAVE_PATH}/DMs_$dmID-isds.zfo" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgFromISDS '$dmID': $TEST_USER_SEND1 - ERROR"
		exit
	else
		echo "GetMsgFromISDS '$dmID': $TEST_USER_SEND1 - OK"
		echo "ExportToZFO '$dmID': $TEST_USER_SEND1 - OK"
	fi
done

#----Download delivery info of new messages from ISDS------------------------
#----must be success and save zfo file
for dmID in $RMSGIDS; do
	RET=`"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$TEST_USER_SEND1'" \
		--get-delivery-info "dmID='$dmID',zfoFile='${ATTACH_SAVE_PATH}/DMs-info_$dmID-isds.zfo" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgDelInfoFromISDS '$dmID': $TEST_USER_SEND1 - ERROR"
		exit
	else
		echo "GetMsgDelInfoFromISDS '$dmID': $TEST_USER_SEND1 - OK"
		echo "ExportToZFO '$dmID': $TEST_USER_SEND1 - OK"
	fi
done

#----Export complete messages from database again-----------------------
#----must be success and save zfo file
for dmID in $RMSGIDS; do
	RET=`"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$TEST_USER_SEND1'" \
		--get-msg "dmID='$dmID',dmType='sent',download='no',zfoFile='${ATTACH_SAVE_PATH}/DMs_$dmID-db.zfo" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgFromDb '$dmID': $TEST_USER_SEND1 - ERROR"
		exit
	else
		echo "GetMsgFromDb '$dmID': $TEST_USER_SEND1 - OK"
		echo "ExportToZFO '$dmID': $TEST_USER_SEND1 - OK"
	fi
done

#----Export delivery info of messages from again----------------------
#----must be success and save zfo file
for dmID in $RMSGIDS; do
	RET=`"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$TEST_USER_SEND1'" \
		--get-delivery-info "dmID='$dmID',download='no',zfoFile='${ATTACH_SAVE_PATH}/DMs-info_$dmID-db.zfo" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgDelInfoFromDb '$dmID': $TEST_USER_SEND1 - ERROR"
		exit
	else
		echo "GetMsgDelInfoFromDb '$dmID': $TEST_USER_SEND1 - OK"
		echo "ExportToZFO '$dmID': $TEST_USER_SEND1 - OK"
	fi
done


echo ""
echo "***********************************************************************"
echo "TEST 04b: Create and send a new message from account (OVM) with"
echo "          certificate to multiple recipients (OVM, FO, PO)."
echo "***********************************************************************"
echo "---Create and send a new message from user '$CERT_LOGIN'---"
DTIME=$(date +"%Y-%m-%d %T")
DMANNOTATION="Datovka - test CLI - ${DTIME}"
DMATACHMENT="${ATTACH_LOAD_PATH}/dokument.odt;${ATTACH_LOAD_PATH}/dokument.pdf;${ATTACH_LOAD_PATH}/notification.mp3;${ATTACH_LOAD_PATH}/obrazek.jpg;\
${ATTACH_LOAD_PATH}/obrazek.png;${ATTACH_LOAD_PATH}/datova-zprava.zfo"
MSGID=`"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} -D \
	--login "username='$CERT_LOGIN',otpcode='$CERT_PWD'" \
	--send-msg "dbIDRecipient='$TEST_RECIPS_SEND',dmAnnotation='${DMANNOTATION}',dmAttachment='${DMATACHMENT}'"  \
	2>/dev/null`
if [ 0 != $? ]; then
	echo "SendMultiMsg: $CERT_LOGIN- ERROR"
	exit
else
	echo "SendMultiMsg: $CERT_LOGIN - OK - $MSGID"
fi

echo ""
echo "Waiting for the server DS - 10 seconds ..."
sleep 10


echo ""
echo "***********************************************************************"
echo "TEST 04c: Create and send a new message from account (PO) to (OVM)."
echo "          All mandatory and optional attributes are filled."
echo "***********************************************************************"
echo "---Create and send a new message from user '$TEST_USER_SEND2'---"
DTIME=$(date +"%Y-%m-%d %T")
DMANNOTATION="Datovka - test CLI - ${DTIME}"
DMATACHMENT="${ATTACH_LOAD_PATH}/dokument.odt;;${ATTACH_LOAD_PATH}/dokument.pdf;${ATTACH_LOAD_PATH}/notification.mp3;${ATTACH_LOAD_PATH}/obrazek.jpg;\
${ATTACH_LOAD_PATH}/obrazek.png;;;${ATTACH_LOAD_PATH}/datova-zprava.zfo"
MSGID=`"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$TEST_USER_SEND2'" \
	--send-msg "dbIDRecipient='$TEST_RECIP_SEND2',dmAttachment='${DMATACHMENT}',dmAnnotation='${DMANNOTATION}',dmPersonalDelivery='1',dmAllowSubstDelivery='1',dmOVM='0',dmPublishOwnID='1',dmToHands='Jan Pokušitel Červeň',dmRecipientRefNumber='98765',dmSenderRefNumber='123456',dmRecipientIdent='CZ98765',dmSenderIdent='CZ123456',dmLegalTitleLaw='1',dmLegalTitleYear='2',dmLegalTitleSect='3',dmLegalTitlePar='4',dmLegalTitlePoint='5'" \
	2>/dev/null`
if [ 0 != $? ]; then
	echo "SendMsg: $TEST_USER_SEND2 - ERROR"
	exit
else
	echo "SendMsg: $TEST_USER_SEND2 - OK - $MSGID"
fi

echo ""
echo "Waiting for the server DS - 10 seconds ..."
sleep 10


echo ""
echo "***********************************************************************"
echo "TEST 05: Get received/sent message list for all accounts."
echo "***********************************************************************"
#---Get message list for account with username and pwd---
for login in $USER_LOGINS; do
	MSGIDS=`"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$login'" \
		--get-msg-list "dmType='received'" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgList (received): $login - ERROR"
		exit
	else
		echo "GetMsgList (received): $login - OK $MSGIDS"
	fi

	MSGIDS=`"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
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
#---Get message list for account with certificate---
MSGIDS=`"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$CERT_LOGIN',otpcode='$CERT_PWD'" \
	--get-msg-list "dmType='received'" \
	2>/dev/null`
if [ 0 != $? ]; then
	echo "GetMsgList (received): $CERT_LOGIN - ERROR"
	exit
else
	echo "GetMsgList (received): $CERT_LOGIN - OK $MSGIDS"
fi

echo ""
echo "***********************************************************************"
echo "TEST 06: Download new message, set as locally read"
echo "         and export to ZFO for username with certificate."
echo "***********************************************************************"
#-----Download complete new messages ISDS-------------------------------------
#----must be success and save zfo file
for dmID in $MSGIDS; do
	RET=`"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$CERT_LOGIN',otpcode='$CERT_PWD'" \
		--get-msg "dmID='$dmID',dmType='received',markDownload='yes',zfoFile='${ATTACH_SAVE_PATH}/DMs_$dmID-isds.zfo" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgFromISDS '$dmID': $TEST_USER_SEND1 - ERROR"
		exit
	else
		echo "GetMsgFromISDS '$dmID': $TEST_USER_SEND1 - OK"
		echo "ExportToZFO '$dmID': $TEST_USER_SEND1 - OK"
	fi
done

#----Download delivery info of new messages from ISDS------------------------
#----must be success and save zfo file
for dmID in $RMSGIDS; do
	RET=`"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
		--login "username='$CERT_LOGIN',otpcode='$CERT_PWD'" \
		--get-delivery-info "dmID='$dmID',zfoFile='${ATTACH_SAVE_PATH}/DMs-info_$dmID-isds.zfo" \
		2>/dev/null`
	if [ 0 != $? ]; then
		echo "GetMsgDelInfoFromISDS '$dmID': $TEST_USER_SEND1 - ERROR"
		exit
	else
		echo "GetMsgDelInfoFromISDS '$dmID': $TEST_USER_SEND1 - OK"
		echo "ExportToZFO '$dmID': $TEST_USER_SEND1 - OK"
	fi
done


echo "***********************************************************************"
echo "* TEST 07: Check messages where attachment missing (via all accounts)."
echo "***********************************************************************"
for login in $USER_LOGINS; do
	"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
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

echo ""
echo "***********************************************************************"
echo "TEST 08: Create and send a new message - wrong parametrs."
echo "***********************************************************************"
# this request must finish with error
DMANNOTATION="Datovka - test CLI - Error"
DMATACHMENT="${ATTACH_LOAD_PATH}/datova-zprava.zfo"
"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$TEST_USER_SEND1'" \
	--send-msg "dbIDRecipient='$TEST_RECIP_WRONG',dmAnnotation='${DMANNOTATION}',dmAttachment='${DMATACHMENT}'" \
	2>/dev/null
if [ 0 != $? ]; then
	echo "SendMsg: $TEST_USER_SEND1 - OK: databox '$TEST_RECIP_WRONG' does not exist."
else
	echo "SendMsg: $TEST_USER_SEND1 - ERROR"
	exit
fi

# this request must finish with error
DMANNOTATION="Datovka - test CLI - Error"
DMATACHMENT="${ATTACH_LOAD_PATH}/dokument.odt;${ATTACH_LOAD_PATH}/dokument.pdf;${ATTACH_LOAD_PATH}/notification.mp3;${ATTACH_LOAD_PATH}/obrazek.jpg;\
${ATTACH_LOAD_PATH}/obrazek.png;${ATTACH_LOAD_PATH}/datova-zprava.zfo;${ATTACH_LOAD_PATH}/xxxxxxxx.zfo"
"${SCRIPTPATH}/${APP_BINARY_NAME}" ${CMDARGS} \
	--login "username='$TEST_USER_SEND1'" \
	--send-msg "dbIDRecipient='$TEST_RECIP_SEND1',dmAnnotation='${DMANNOTATION}',dmAttachment='${DMATACHMENT}'" \
	2>/dev/null
if [ 0 != $? ]; then
	echo "SendMsg: $TEST_USER_SEND1 - OK: this message has wrong attachment path."
else
	echo "SendMsg: $TEST_USER_SEND1 - ERROR"
	exit
fi

echo ""
echo ""
echo "-------------------------------------------------"
echo "CONGRATULATION: All tests were done with success."
echo "-------------------------------------------------"
echo ""
