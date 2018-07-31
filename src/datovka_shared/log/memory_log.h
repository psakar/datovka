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

#include <QHash>
#include <QList>
#include <QObject>
#include <QQueue>

/*!
 * @brief Hold log messages in memory.
 *
 * @note The operations are not thread safe. They aren't internally mutex-guarded.
 */
class MemoryLog : public QObject {
	Q_OBJECT

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] parent Parent object.
	 */
	explicit MemoryLog(QObject *parent = Q_NULLPTR);

	/*!
	 * @brief Set maximal amount of memory to hold messages.
	 *
	 * @brief[in] bytes Amount of memory for strings.
	 */
	void setMaxMemory(int bytes);

	/*!
	 * @brief Get memory limit.
	 *
	 * @return Maximal amount of memory which is reserved for log messages.
	 */
	int maxMemory(void) const;

	/*!
	 * @brief Write a message to memory log.
	 *
	 * @param[in] msg Message.
	 * @return True when message has been stored.
	 */
	bool log(const QString &msg);

	/*!
	 * @brief Get a message for given key.
	 *
	 * @param[in] key Identifying the message.
	 * @return Reference to held string, null string if the key does not exist.
	 */
	const QString &message(quint64 key) const;

	/*!
	 * @brief Return list of held keys.
	 *
	 * @return Sequence of held keys.
	 */
	QList<quint64> keys(void) const;

signals:
	/*!
	 * @brief This signed is emitted when a new message is stored.
	 */
	void logged(quint64 key);

private:
	/*!
	 * @brief Generates a unique key.
	 *
	 * @return A key which does not exist in the storage.
	 */
	quint64 uniqueKey(void) const;

	/*!
	 * @brief Make space for specified numbers of characters.
	 *
	 * @param[in] charNum Number of characters which are needed in the log.
	 */
	void assureSpace(quint64 charNum);

	/*!
	 * @brief Append another string under given key.
	 *
	 * @note Emits the logged signal.
	 *
	 * @param[in] key Entry key.
	 * @param[in] msg Message to be stored.
	 */
	void storeMsg(quint64 key, const QString &msg);

	qint64 m_chars; /*!< Number of logged string characters. */
	qint64 m_maxChars; /*!< Maximal number of characters to be held. */

	QHash<quint64, QString> m_loggedMsgs;
	QQueue<quint64> m_keys;
};
