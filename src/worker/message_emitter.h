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
