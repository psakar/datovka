/*
 * Copyright (C) 2014-2018 CZ.NIC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
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

#pragma once

#include <QDialog>
#include <QItemSelection>
#include <QList>
#include <QSet>
#include <QTimer>

#include "src/datovka_shared/isds/message_interface.h"
#include "src/io/message_db.h"
#include "src/io/message_db_set.h"
#include "src/models/attachments_model.h"
#include "src/models/data_box_contacts_model.h"
#include "src/worker/task.h"
#include "src/worker/task_send_message.h"

namespace Ui {
	class DlgSendMessage;
}

/*!
 * @brief Returns message prefix depending on whether it is sent ore received.
 *
 * @param[in] messageDb Message database.
 * @param[in] dmId Message identifier number.
 * @return Prefix strings 'DDZ', 'ODZ' or 'DZ'.
 *
 * @todo Move this function somewhere else
 *     (and clean up the code of the following dialogue.
 */
const QString &dzPrefix(const MessageDb *messageDb, qint64 dmId);

/*!
 * @brief Send message dialogue.
 */
class DlgSendMessage : public QDialog {
	Q_OBJECT

public:
	/*!
	 * @brief Action to be performed.
	 */
	enum Action {
		ACT_NEW, /* Create new message. */
		ACT_REPLY, /* Fill dialogue as a reply on a message. */
		ACT_FORWARD, /* Forward supplied messages as ZFO attachments. */
		ACT_NEW_FROM_TMP /* Use existing message as a template. */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] messageDbSetList List of available accounts.
	 * @param[in] action What king of action should be performed.
	 * @param[in] msgIds  List of messages used to fill the dialogue.
	 * @param[in] userName The account the dialogue has been invoked from.
	 * @param[in] composeSerialised Serialised content data.
	 * @param[in] mw Pointer to main window.
	 * @param[in] parent Parent widget.
	 */
	DlgSendMessage(const QList<Task::AccountDescr> &messageDbSetList,
	    enum Action action, const QList<MessageDb::MsgId> &msgIds,
	    const QString &userName, const QString &composeSerialised,
	    class MainWindow *mw, QWidget *parent = Q_NULLPTR);

	/*!
	 * @brief Destructor.
	 */
	virtual
	~DlgSendMessage(void);

signals:
	/*!
	 * @brief This signal is emitted whenever the attachment path has been
	 *     used and should be stored.
	 *
	 * @param[in] userName User name identifying the related account.
	 * @param[in] attDir Used attachment directory.
	 */
	void usedAttachmentPath(const QString &userName, const QString &attDir);

private slots:
	/*!
	 * @brief Disable records management upload if immediate download is disabled.
	 */
	void immDownloadStateChanged(int state);

	/*!
	 * @brief Enable immediate download if records management upload is enabled.
	 */
	void immRecMgmtUploadStateChanged(int state);

	/*!
	 * @brief Check input fields' sanity and activate search button.
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
	void deleteRecipientEntries(void);

	/*!
	 * @brief Show/hide optional form elements.
	 */
	void showOptionalFormElements(void);

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

	/*!
	 * @brief Open attachment in default application.
	 *
	 * @param[in] index Index identifying the line. If invalid index passed
	 *                  then selected item is taken from the selection
	 *                  model.
	 */
	void openSelectedAttachment(const QModelIndex &index = QModelIndex());

	/*!
	 * @brief ISDS connection keep-alive function.
	 */
	void pingIsdsServer(void) const;

	/*!
	 * @brief Set account information and database for selected account.
	 *
	 * @param[in] fromComboIdx Index of selected 'From' combo box item.
	 */
	void setAccountInfo(int fromComboIdx);

	/*!
	 * @brief Send message/multiple messages.
	 */
	void sendMessage(void);

	/*!
	 * @brief Gathers status after sending messages via ISDS interface.
	 *     Shows result table if all data collected.
	 *
	 * @param[in] userName User name identifying the sender account.
	 * @param[in] transactId Unique transaction identifier.
	 * @param[in] result Sending status.
	 * @param[in] resultDesc Result description string.
	 * @param[in] dbIDRecipient Recipient data box identifier.
	 * @param[in] recipientName Recipient data box name.
	 * @param[in] isPDZ True if message was attempted to send as commercial
	 *                  message.
	 * @param[in] dmId Sent message identifier.
	 * @param[in] processFlags Message processig flags.
	 */
	void collectSendMessageStatus(const QString &userName,
	    const QString &transactId, int result, const QString &resultDesc,
	    const QString &dbIDRecipient, const QString &recipientName,
	    bool isPDZ, qint64 dmId, int processFlags);

private:
	/*!
	 * @brief Initialises the dialogue content.
	 *
	 * @param[in] action Specifies how the dialogue should be initialised.
	 * @param[in] msgIds Identifiers of messages to fill the dialogue with.
	 */
	void initContent(enum Action action,
	    const QList<MessageDb::MsgId> &msgIds);

	/*!
	 * @brief Set dialogue content with ZFO attachments.
	 *
	 * @param[in] msgIds Identifiers of messages to fill the dialogue with.
	 */
	void fillContentAsForward(const QList<MessageDb::MsgId> &msgIds);

	/*!
	 * @brief Set the dialogue content as reply.
	 *
	 * @param[in] msgIds Identifiers of messages to fill the dialogue with.
	 */
	void fillContentAsReply(const QList<MessageDb::MsgId> &msgIds);

	/*!
	 * @brief Set the dialogue content from template message.
	 *
	 * @param[in] msgIds Identifiers of messages to fill the dialogue with.
	 */
	void fillContentFromTemplate(const QList<MessageDb::MsgId> &msgIds);

	/*!
	 * @brief Set the dilaogue content according to serialised compose data.
	 *
	 * @param[in] composeSerialised Serialised CLI::CmdCompose data.
	 */
	void fillContentCompose(const QString &composeSerialised);

	/*!
	 * @brief Inserts attachment files.
	 *
	 * @param[in] filePaths List of file paths.
	 */
	void insertAttachmentFiles(const QStringList &filePaths);

	/*!
	 * @brief Creates a notification QMessageBox informing the user about
	 *     the number of created commercial messages.
	 * @return QMessageBox::StandardButton value describing the pressed
	 *         button.
	 */
	int notifyOfPDZ(int pdzCount);

	/*!
	 * @brief Calculates and shows total attachment size.
	 */
	bool calculateAndShowTotalAttachSize(void);

	/*!
	 * @brief Append data box into recipient list.
	 *
	 * @param[in] boxId Data box identifier.
	 */
	void addRecipientBox(const QString &boxId);

	/*!
	 * @brief Appends data boxes into recipient list.
	 *
	 * @note This is a convenience method that calls addRecipientBox().
	 *
	 * @param[in] boxIds List of data box identifiers.
	 */
	void addRecipientBoxes(const QStringList &boxIds);

	/*!
	 * @brief Queries ISDS for remaining PDZ credit.
	 *
	 * @param[in] userName User name identifying the account.
	 * @param[in] dnId Data box identifier.
	 * @return String containing the amount of remaining credit in Czech
	 *         crowns.
	 */
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

	/*!
	 * @brief Sets envelope content according to dialogue content.
	 *
	 * @param[out] envelope Envelope to be set.
	 * @return True on success.
	 */
	bool buildEnvelope(Isds::Envelope &envelope) const;

	/*!
	 * @brief Appends attachments to list of document descriptors.
	 *
	 * @param[out] documents List to append data to.
	 * @return True on success.
	 */
	bool buildDocuments(QList<Isds::Document> &documents) const;

	/*!
	 * @brief Send message via standard ISDS interface for third party apps.
	 *
	 * @param[in] recipEntries List of recipients.
	 */
	void sendMessageISDS(
	    const QList<BoxContactsModel::PartialEntry> &recipEntries);

	/*!
	 * @brief Set icon set to buttons.
	 */
	void setIcons(void);

	Ui::DlgSendMessage *m_ui; /*!< UI generated from UI file. */

	QTimer m_keepAliveTimer; /*!< Keeps connection to ISDS alive. */
	const QList<Task::AccountDescr> m_messageDbSetList; /*!< Available accounts.*/

	QString m_userName; /*!< Selected user name (login). */
	QString m_boxId; /*!< Name of data box associated with selected user. */
	QString m_senderName; /*!< Sender (data box) name. */
	QString m_dbType; /*!< Data box type identifier string. */
	bool m_dbEffectiveOVM; /*! True if selected data box has effective OVM. */
	bool m_dbOpenAddressing; /*! True if selected box has open addressing.  */
	bool m_isLoggedIn; /*!< True if account has already logged in. */

	QString m_lastAttAddPath; /*! Last attachment location. */
	QString m_pdzCredit; /*! String containing credit value. */

	QString m_dmType; /*!< Message type. */
	QString m_dmSenderRefNumber; /*!< Message reference number. */

	MessageDbSet *m_dbSet; /*!< Pointer to database container. */

	BoxContactsModel m_recipTableModel; /*!< Data box table model. */
	AttachmentTblModel m_attachModel; /*!< Attachment table model. */

	/* Used to collect sending results. */
	QSet<QString> m_transactIds; /*!< Temporary transaction identifiers. */
	QList<TaskSendMessage::ResultData> m_sentMsgResultList; /*!< Send status list. */

	class MainWindow *const m_mw; /*!< Pointer to main window. */
};
