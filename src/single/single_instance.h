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

#include <QObject>
#include <QSharedMemory>
#include <QString>

/*!
 * @brief Class for checking whether the is another application instance.
 */
class SingleInstance : public QObject {
	Q_OBJECT

public:
	/*!
	 * @brief Defines action described by the message.
	 */
	enum MsgType {
		MTYPE_UNKNOWN = -1, /*! Convenience value. */
		MTYPE_RAISE_MAIN_WIN = 0 /*!<
		                          * Notification that the main window
		                          * ought to be raised.
		                          */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] shMemKey Shared memory key (usually configuration path).
	 * @param[in] parent Pointer to parent object.
	 */
	explicit SingleInstance(const QString &shMemKey = QString(),
	    QObject *parent = Q_NULLPTR);

	/*!
	 * @brief Performs a system-wide check whether there is another
	 *     instance.
	 *
	 * @return True if there is another instance of application running in
	 *     the system.
	 */
	bool existsInSystem(void) const;

	/*!
	 * @brief Performs a check whether the current user is running another
	 *     instance of the application.
	 *
	 * @return True if there is another instance of the application running
	 *     under the current user.
	 */
//	bool existsUnderCurrentUser(void) const;

	/*!
	 * @brief Performs a check whether there is another application running
	 *     in the same configuration directory.
	 */
//	bool existsWithSameConfiguration(void) const;

	/*!
	 * @brief Sends a message to the first instance.
	 *
	 * @param[in] msgType Type of the message (enum MsgType).
	 * @param[in] msgVal Message parameter.
	 * @return True is message has been sent.
	 */
	bool sendMessage(int msgType, const QString &msgVal = QString());

private slots:
	/*!
	 * @brief Check for message sent via shared memory.
	 */
	void checkMessage(void);

private:
	bool m_memoryExisted; /*!< True if shared memory already existed. */
	QSharedMemory m_shMem; /*!< Shared memory. */
};

/*!
 * @brief Single instance signal emitter.
 */
class SingleInstanceEmitter : public QObject {
	Q_OBJECT

signals:
	/*!
	 * @brief Emitted when message received.
	 *
	 * @param[in] msgType Type of the message (enum MsgType).
	 * @param[in] msgVal Received message parameter.
	 */
	void messageReceived(int msgType, const QString &msgVal);
};
