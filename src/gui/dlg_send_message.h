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

#include <QDateTime>
#include <QDialog>
#include <QFileDialog>
#include <QTimer>
#include <QTreeView>

#include "src/common.h"
#include "src/io/message_db_set.h"
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

	class MsgSendingResult {
	public:
		int status;
		QString dbID;
		QString recipientName;
		qint64 dmId;
		bool isPDZ;
		QString errInfo;

		MsgSendingResult(void)
		    : status(-1), dbID(), recipientName(), dmId(-1),
		    isPDZ(false), errInfo()
		{ }
	};

	DlgSendMessage(MessageDbSet &dbSet, const QString &dbId,
	    const QString &senderName, Action action, qint64 msgId,
	    const QDateTime &deliveryTime,
	    const QString &userName, const QString &dbType,
	    bool dbEffectiveOVM, bool dbOpenAddressing,
	    QString &lastAttAddPath, const QString &pdzCredit,
	    QWidget *parent = 0);

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
	void tableItemDoubleClicked(QTableWidgetItem *item);

private:
	QTimer *pingTimer;
	qint64 m_msgID;
	QDateTime m_deliveryTime;
	const QString m_dbId;
	const QString m_senderName;
	const Action m_action;
	const QString m_userName;
	const QString m_dbType;
	bool m_dbEffectiveOVM;
	bool m_dbOpenAddressing;
	QString &m_lastAttAddPath;
	const QString m_pdzCredit;
	MessageDbSet &m_dbSet;
	int m_attachSize;
	QString m_dmType;
	QString m_dmSenderRefNumber;

	void initNewMessageDialog(void);
	int cmptAttachmentSize(void);
	void fillDlgAsReply(void);
	void fillDlgFromTmpMsg(void);
	int showInfoAboutPDZ(int pdzCnt);

	struct isds_list *buildDocuments(void) const;
	struct isds_envelope *buildEnvelope(void) const;
	MsgSendingResult sendSingleMessage(struct isds_message *message,
	    int row) const;

	static
	QByteArray getFileBase64(const QString &filePath);
	QString getUserInfoFormIsds(QString idDbox);
};


#endif /* _DLG_SEND_MESSAGE_H_ */
