/*
 * Copyright (C) 2014-2015 CZ.NIC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations including
 * the two.
 */

#ifndef _DLG_SEND_MESSAGE_H_
#define _DLG_SEND_MESSAGE_H_

#include <QDialog>
#include <QFileDialog>
#include <QTimer>
#include <QTreeView>

#include "src/common.h"
#include "src/io/message_db.h"
#include "ui_dlg_send_message.h"
#include "src/io/isds_sessions.h"
#include "src/models/accounts_model.h"


class DlgSendMessage : public QDialog, public Ui::SendMessage {
    Q_OBJECT

public:
	enum Action {
		ACT_NEW,
		ACT_REPLY,
		ACT_NEW_FROM_TMP
	};

	class sendMsgResultStruct {
	public:
		QString dbID;
		QString recipientName;
		QString dmID;
		bool isPDZ;
		int status;
		QString errInfo;
	};

	DlgSendMessage(MessageDb &messDb, QString &dbId, QString &senderName,
	    Action action, qint64 msgId,
	    const AccountModel::SettingsMap &accountInfo,
	    QString dbType, bool dbEffectiveOVM, bool dbOpenAddressing,
	    QString &lastAttAddPath, QString &pdzCredit,
	    QWidget *parent = 0
	    );

private slots:
	void on_cancelButton_clicked(void);
	void showOptionalForm(int);
	void showOptionalFormAndSet(int);
	void addAttachmentFile(void);
	void deleteAttachmentFile(void);
	void openAttachmentFile(void);
	void addRecipientFromLocalContact(void);
	void deleteRecipientData(void);
	void findAndAddRecipient(void);
	void recItemSelect(void);
	void attItemSelect(void);
	void checkInputFields(void);
	void tableItemInsRem(void);
	void sendMessage(void);
	void pingIsdsServer(void);
	void addDbIdToRecipientList(void);

private:
	QTimer *pingTimer;
	void initNewMessageDialog(void);
	qint64 m_msgID;
	const QString m_dbId;
	const QString m_senderName;
	const Action m_action;
	AccountModel::SettingsMap m_accountInfo;
	QString m_dbType;
	bool m_dbEffectiveOVM;
	bool m_dbOpenAddressing;
	QString &m_lastAttAddPath;
	QString &m_pdzCredit;
	MessageDb &m_messDb;
	int m_attachSize;
	QString m_dmType;
	QString m_dmSenderRefNumber;

	int cmptAttachmentSize(void);
	void fillDlgAsReply(void);
	void fillDlgFromTmpMsg(void);
	int showInfoAboutPDZ(int pdzCnt);
	QString getFileBase64(QString filePath);
	int getFileSizeFromBase64(QString fileBase64);
	QString getUserInfoFormIsds(QString idDbox);
};


#endif /* _DLG_SEND_MESSAGE_H_ */
