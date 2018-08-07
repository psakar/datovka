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

#ifndef _MESSAGE_EMITTER_H_
#define _MESSAGE_EMITTER_H_

#include <QObject>

/*!
 * @brief Message processing status emitter.
 */
class MessageProcessingEmitter : public QObject {
	Q_OBJECT

signals:
	/*!
	 * @brief Emitted when download message finishes.
	 *
	 * @param[in] usrName       Account identifier (user login name).
	 * @param[in] msgId         Message identifier.
	 * @param[in] deliveryTime  Message delivery time.
	 * @param[in] result        Operation outcome
	 *                          (enum TaskDownloadMessage::Result).
	 * @param[in] errDesc       Error description string.
	 * @param[in] listScheduled True if ran from download message list.
	 */
	void downloadMessageFinished(const QString &usrName, qint64 msgId,
	   const QDateTime &deliveryTime, int result, const QString &errDesc,
	   bool listScheduled);

	/*!
	 * @brief Emitted when download message finishes.
	 *
	 * @param[in] usrName   Account identifier (user login name).
	 * @param[in] direction Sent or received messages
	 *                      (enum MessageDirection).
	 * @param[in] result    Operation outcome
	 *                      (enum TaskDownloadMessageList::Result).
	 * @param[in] errDesc   Error description string.
	 * @param[in] add       Whether to add the obtained value.
	 * @param[in] rt        Number of received messages on server.
	 * @param[in] rn        Number of new received messages (locally unknown).
	 * @param[in] st        Number of sent messages on server.
	 * @param[in] sn        Number of new sent messages (locally unknown).
	 */
	void downloadMessageListFinished(const QString &usrName,
	    int direction, int result, const QString &errDesc,
	    bool add, int rt, int rn, int st, int sn);

	/*!
	 * @brief Emitted when ZFO import finishes.
	 *
	 * @param[in] fileName   ZFO file name.
	 * @param[in] result     Operation outcome
	 *                       (enum TaskImportZfo::Result).
	 * @param[in] resultDesc Result description string.
	 */
	void importZfoFinished(const QString &fileName, int result,
	    const QString &resultDesc);

	/*!
	 * @brief Emitted when message import finishes.
	 *
	 * @param[in] userName      Account user name.
	 * @param[in] errImportList List of unsuccess imports (description).
	 * @param[in] totalMsg      Total number of messages.
	 * @param[in] importedMsg   Total number of imported messages.
	 */
	void importMessageFinished(const QString &userName,
	    const QStringList &errImportList, int totalMsgs, int importedMsgs);

	/*!
	 * @brief This signal is emitted when counted value is changed
	 *
	 * @param[in] label Progress bar label.
	 * @param[in] value Progress value.
	 */
	void progressChange(const QString &label, int value);

	/*!
	 * @brief This signal is emitted when account is changed.
	 *
	 * @param[in] account username.
	 */
	void refreshAccountList(const QString &userName);

	/*!
	 * @brief Emitted when send message finishes.
	 *
	 * @param[in] userName      Account identifier (user login name).
	 * @param[in] transactId    Transaction identifier.
	 * @param[in] result        Operation outcome
	 *                          (enum TaskSendMessage::Result).
	 * @param[in] resultDesc    Result description string.
	 * @param[in] dbIDRecipient Recipient identifier.
	 * @param[in] recipientName Recipient name.
	 * @param[in] isPDZ         True if message was a PDZ.
	 * @param[in] dmId          Message identifier if message has been sent.
	 * @param[in] processFlags Message processing flags.
	 */
	void sendMessageFinished(const QString &userName,
	    const QString &transactId, int result, const QString &resultDesc,
	    const QString &dbIDRecipient, const QString &recipientName,
	    bool isPDZ, qint64 dmId, int processFlags);

	/*!
	 * @brief This signal is emitted when status bar text is changed
	 *
	 * @param[in] text Status bar text.
	 */
	void statusBarChange(const QString &text);

	/*!
	 * @brief Emitted when records management stored messages task finishes.
	 */
	void recordsManagementStoredMessagesFinished(const QString &taskId);
};

#endif /* _MESSAGE_EMITTER_H_ */
