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

#include <QDateTime>

#include "src/datovka_shared/log/memory_log.h"

MemoryLog::MemoryLog(QObject *parent)
    : QObject(parent),
    m_chars(0),
    m_maxChars(0),
    m_loggedMsgs(),
    m_keys()
{
}

void MemoryLog::setMaxMemory(int bytes)
{
	/* QStrings are UTF-16 encoded. */
	m_maxChars = (bytes > 0) ? (bytes / 2) : 0;

	/* If already filled and setting smaller. */
	assureSpace(0);
}

int MemoryLog::maxMemory(void) const
{
	return m_maxChars * 2;
}

bool MemoryLog::log(const QString &msg)
{
	if (Q_UNLIKELY(msg.isEmpty())) {
		return false;
	}

	if (Q_UNLIKELY(m_maxChars == 0)) {
		/* Log is disabled. */
		return false;
	}

	/*
	 * A single message may exceed the entire log capacity. In such cases
	 * the message is not accepted.
	 */
	if (Q_UNLIKELY(msg.size() > m_maxChars)) {
		return false;
	}

	assureSpace(msg.size());

	storeMsg(uniqueKey(), msg);

	return true;
}

const QString &MemoryLog::message(quint64 key) const
{
	static const QString nullStr;

	const QHash<quint64, QString>::const_iterator iter(m_loggedMsgs.find(key));
	if (Q_UNLIKELY(iter == m_loggedMsgs.end())) {
		Q_ASSERT(0);
		return nullStr;
	}
	return *iter;
}

QList<quint64> MemoryLog::keys(void) const
{
	return m_keys.mid(0, -1);
}

quint64 MemoryLog::uniqueKey(void) const
{
	static const quint64 msecBase = QDateTime::currentMSecsSinceEpoch();
	static const quint64 upperHalf = Q_UINT64_C(1) << 32;

	const quint64 msecOffset = QDateTime::currentMSecsSinceEpoch() - msecBase;

	quint64 key = msecOffset;
	unsigned iter = 0;
	while (m_loggedMsgs.contains(key)) {
		key = msecOffset + ((iter++) * upperHalf);
	}

	return key;
}

void MemoryLog::assureSpace(quint64 charNum)
{
	while ((m_maxChars - m_chars) < (qint64)charNum) {
		const quint64 key = m_keys.dequeue();
		const QHash<quint64, QString>::const_iterator iter(m_loggedMsgs.find(key));
		if (Q_UNLIKELY(iter == m_loggedMsgs.end())) {
			Q_ASSERT(0);
			continue;
		}
		int freedSpace = iter->size();
		m_loggedMsgs.remove(key);
		m_chars -= (quint64)freedSpace;
	}
}

void MemoryLog::storeMsg(quint64 key, const QString &msg)
{
	if (Q_UNLIKELY(m_loggedMsgs.contains(key))) {
		Q_ASSERT(0);
		return;
	}

	m_loggedMsgs.insert(key, msg);
	m_keys.enqueue(key);
	m_chars += msg.size();

	emit logged(key);
}
