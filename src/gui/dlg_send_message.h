/*
 * Copyright (C) 2014-2017 CZ.NIC
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
#include <QList>
#include <QSet>
#include <QTimer>
#include <QTreeView>

#include "src/common.h"
#include "src/io/message_db.h"
#include "src/io/message_db_set.h"
#include "src/models/data_box_contacts_model.h"
#include "src/models/files_model.h"
#include "src/worker/task.h"
#include "src/worker/task_send_message.h"
#include "src/web/json.h"
#include "ui_dlg_send_message.h"

/*!
 * @brief Returns message prefix depending on whether it is sent ore received.
 *
 * @param[in] messageDb Message database.
 * @param[in] dmId Message identifier number.
 * @return Prefix.
 *
 * @todo Move this function somewhere else
 *     (and clean up the code of the following dialogue.
 */
const QString &dzPrefix(MessageDb *messageDb, qint64 dmId);

/*!
 * @brief Send message dialogue.
 */
class DlgSendMessage : public QDialog, public Ui::SendMessage {
    Q_OBJECT

public:
	enum Action {
		ACT_NEW,
		ACT_REPLY,
		ACT_FORWARD,
		ACT_NEW_FROM_TMP
	};

	/*!
	 * @brief Constructor.
	 */
	DlgSendMessage(const QList<Task::AccountDescr> &messageDbSetList,
	    Action action, const QList<MessageDb::MsgId> &msgIds,
	    const QString &userName, class MainWindow *mv,
	    QWidget *parent = Q_NULLPTR);

signals:
	void doActionAfterSentMsgSignal(const QString, const QString);

private slots:
	/*!
	 * @brief Check input fields sanity and activate search button.
	 */
	void checkInputFields(void);

	/*!
	 * @brief Add recipient from contacts held in database.
	 */
	void addRecipientFromLocalContact(void);

	/*!
	 * @brief Add recipients found via ISDS search.
	 */
	void addRecipientFromISDSSearch(void);

	/*!
	 * @brief Manually add a data box identifier.
	 */
	void addRecipientManually(void);

	/*!
	 * @brief Activates control elements based on selected recipients.
	 *
	 * @param[in] selected Newly selected items.
	 * @param[in] deselected Deselected items.
	 */
	void recipientSelectionChanged(const QItemSelection &selected,
	    const QItemSelection &deselected);

	/*!
	 * @brief Deletes selected row from the recipient list.
	 */
	void deleteRecipientData(void);

	/*!
	 * @brief Show/hide optional form elements.
	 */
	void showOptionalForm(void);

	/*!
	 * @brief Add attachment file.
	 */
	void addAttachmentFile(void);

	/*!
	 * @brief Activates control elements based on selected attachments.
	 *
	 * @param[in] selected Newly selected items.
	 * @param[in] deselected Deselected items.
	 */
	void attachmentSelectionChanged(const QItemSelection &selected,
	    const QItemSelection &deselected);

	/*!
	 * @brief Remove selected attachment entries.
	 */
	void deleteSelectedAttachmentFiles(void);

	/*
	 * @brief Open attachment in default application.
	 *
	 * @param index Index identifying the line. If invalid index passed then
	 *              selected item is taken from the selection model.
	 */
	void openSelectedAttachment(const QModelIndex &index = QModelIndex());

	void sendMessage(void);
	void collectSendMessageStatus(const QString &userName,
	    const QString &transactId, int result, const QString &resultDesc,
	    const QString &dbIDRecipient, const QString &recipientName,
	    bool isPDZ, qint64 dmId);
	void sendMessageMojeIdAction(const QString &userName,
	    const QStringList &result, const QString &error);
	void pingIsdsServer(void);
	void attachmentDataChanged(const QModelIndex &topLeft,
	    const QModelIndex &bottomRight, const QVector<int> &roles);
	void setAccountInfo(int item);

private:
	/*!
	 * @brief Initialises the dialogue content.
	 */
	void initContent(void);

	QTimer m_keepAliveTimer;
	const QList<Task::AccountDescr> m_messageDbSetList;
	const QList<MessageDb::MsgId> m_msgIds;
	QString m_dbId;
	QString m_senderName;
	const Action m_action;
	QString m_userName;
	QString m_dbType;
	bool m_dbEffectiveOVM;
	bool m_dbOpenAddressing;
	QString m_lastAttAddPath;
	QString m_pdzCredit;
	QString m_dmType;
	QString m_dmSenderRefNumber;
	class MainWindow *m_mv;
	MessageDbSet *m_dbSet;
	bool m_isLogged;
	DbFlsTblModel m_attachmentModel; /*!< Attachment model. */
	bool m_isWebDatovkaAccount;

	BoxContactsModel m_recipientTableModel; /*!< Model of data boxes. */

	/* Used to collect sending results. */
	QSet<QString> m_transactIds;
	QList<TaskSendMessage::ResultData> m_sentMsgResultList;

	bool calculateAndShowTotalAttachSize(void);
	void fillDlgAsReply(void);
	void fillDlgAsForward(void);
	void fillDlgFromTmpMsg(void);
	int showInfoAboutPDZ(int pdzCnt);

	bool buildDocuments(QList<IsdsDocument> &documents) const;
	bool buildEnvelope(IsdsEnvelope &envelope) const;
	bool buildEnvelopeWebDatovka(JsonLayer::Envelope &envelope) const;
	bool buildFileListWebDatovka(QList<JsonLayer::File> &fileList) const;

	/*
	 * Insert list of databoxes into recipient list.
	*/
	void insertDataboxesToRecipientList(const QStringList &dbIDs);


	static
	QString getPDZCreditFromISDS(const QString &userName,
	    const QString &dbId);

	/*!
	 * @brief Query ISDS whether data box has effective OVM set.
	 *
	 * @param[in] userName User account to generate the query from.
	 * @param[in] boxId Data box to ask for OVM status.
	 * @return True if \a boxId has effective OVM status set.
	 */
	static
	bool queryISDSBoxEOVM(const QString &userName, const QString &boxId);

};


#endif /* _DLG_SEND_MESSAGE_H_ */
