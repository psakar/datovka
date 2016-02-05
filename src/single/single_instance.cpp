/*
 * Copyright (C) 2014-2016 CZ.NIC
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

#include <cstring>
#include <QByteArray>
#include <QSystemSemaphore>
#include <QTimer>

#include "src/log/log.h"
#include "src/single/single_instance.h"

#define UNIQUE_SEM_ID "CZ.NIC_Datovka_(e-gov_client)_semaphore"
#define UNIQUE_MEM_ID "CZ.NIC_Datovka_(e-gov_client)_shared_mem"
#define MEM_SIZE 4096

SingleInstanceEmitter globSingleInstanceEmitter;

SingleInstance::SingleInstance(QObject *parent)
    : QObject(parent),
    m_memoryExisted(false),
    m_shMem()
{
	/*
	 * On UNIX the semaphore is not removed on crash.
	 * For now lets assume that the code does not crash here.
	 */
	QSystemSemaphore sema(UNIQUE_SEM_ID, 1);
	logInfo("Trying to acquire semaphore '%s'.\n", UNIQUE_SEM_ID);
	sema.acquire();
	logInfo("Semaphore '%s' acquired.\n", UNIQUE_SEM_ID);

#if !defined(Q_OS_WIN)
	/*
	 * On UNIX systems the memory is not freed opon crash.
	 * If there is any previous instance, clean it.
	 */
	{
		QSharedMemory shMem(UNIQUE_MEM_ID);
		if (shMem.attach()) {
			shMem.detach();
		}
	}
#endif /* !defined(Q_OS_WIN) */

	m_shMem.setKey(UNIQUE_MEM_ID);

	if (m_shMem.create(MEM_SIZE)) {
		m_shMem.lock();
		memset(m_shMem.data(), 0, MEM_SIZE);
		m_shMem.unlock();

		/* Start reading messages. */
		QTimer *timer = new QTimer(this);
		connect(timer, SIGNAL(timeout()), this, SLOT(checkMessage()));
		timer->start(200);
	} else if (m_shMem.attach()) {
		m_memoryExisted = true;
	} else {
		logErrorNL("%s", "Cannot access shared memory.");
	}

	sema.release();
	logInfo("Semaphore '%s' released.\n", UNIQUE_SEM_ID);
}

bool SingleInstance::existsInSystem(void) const
{
	return m_memoryExisted;
}

bool SingleInstance::sendMessage(const QString &message)
{
	/* Master process cannot send messages. */
	if (!m_memoryExisted){
		return false;
	}

	/* Only short messages can be sent. */
	if (message.size() > 255) {
		return false;
	}

	m_shMem.lock();

	/* Find the end of the buffer. */
	char *begin = (char *) m_shMem.data();
	char *to = begin;
	while(((to - begin) < MEM_SIZE) && (*to != '\0')) {
		int sizeToRead = *to;
		to += sizeToRead + 1;
	}

	/* Compute the remaining space. */
	if ((MEM_SIZE - (to - begin)) < (message.size() + 2)) {
		return false;
	}

	QByteArray byteArray;
	byteArray.append((char) message.size());
	byteArray.append(message.toUtf8());
	byteArray.append('\0');

	memcpy(to, byteArray.data(), byteArray.size());

	m_shMem.unlock();

	return true;
}

void SingleInstance::checkMessage(void)
{
	m_shMem.lock();

	char *begin = (char *) m_shMem.data();
	char *from = begin;

	while(((from - begin) < MEM_SIZE) && (*from != '\0')){
		int sizeToRead = (int) *from;

		++from;
		QByteArray byteArray(from, sizeToRead);
		from += sizeToRead;

		emit globSingleInstanceEmitter.messageReceived(
		    QString::fromUtf8((byteArray + '\0').constData()));
	}

	memset(m_shMem.data(), 0, MEM_SIZE);

	m_shMem.unlock();
}
