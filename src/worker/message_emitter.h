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
	 * @brief Emitted when message download failed.
	 *
	 * @note Message identifier -1 means list of received messages,
	 *     -2 means list of sent messages.
	 *
	 * @param[in] usrName Account identifier (user login name).
	 * @param[in] msgId Message identifier.
	 * @param[in] err   Error description.
	 */
	void downloadFail(const QString &usrName, qint64 msgId,
	    const QString &err);

	/*!
	 * @brief This signal is emitted when account is processed.
	 *     It sends the number of processed messages.
	 *
	 * @param[in] add Whether to add the obtained value.
	 * @param[in] rt Number of received messages on server.
	 * @param[in] rn Number of new received messages (locally unknown).
	 * @param[in] st Number of sent messages on server.
	 * @param[in] sn Number of new sent messages (locally unknown).
	 */
	void downloadListSummary(bool add, int rt, int rn, int st, int sn);

	/*!
	 * @brief Emitted when download process succeeds.
	 *
	 * @note Message identifier -1 means list of received messages,
	 *     -2 means list of sent messages.
	 *
	 * @param[in] usrName Account identifier (user login name).
	 * @param[in] msgId Message identifier.
	 */
	void downloadSuccess(const QString &usrName, qint64 msgId);

	/*!
	 * @brief Emitted when download message finishes.
	 *
	 * @param[in] usrName Account identifier (user login name).
	 * @param[in] msgId   Message identifier.
	 * @param[in] result  Operation outcome
	 *                    (enum TaskDownloadMessage::Result).
	 * @param[in] errDesc Error description string.
	 */
	void downloadMessageFinished(const QString &usrName, qint64 msgId,
	    int result, const QString &errDesc);

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
	 * @brief This signal is emitted when counted value is changed
	 *
	 * @param[in] label Progress bar label.
	 * @param[in] value Progress value.
	 */
	void progressChange(const QString &label, int value);
};

/*!
 * @brief This object is used to emit signals from message processing workers.
 */
extern MessageProcessingEmitter globMsgProcEmitter;

#endif /* _MESSAGE_EMITTER_H_ */
