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

#include <cstring>
#include <QByteArray>
#include <QStringBuilder>
#include <QSystemSemaphore>
#include <QTimer>

#include "src/datovka_shared/log/log.h"
#include "src/global.h"
#include "src/single/single_instance.h"

#define UNIQUE_SEM_ID "CZ.NIC_Datovka_(e-gov_client)_semaphore"
#define UNIQUE_MEM_ID "CZ.NIC_Datovka_(e-gov_client)_shared_mem"
#define MEM_SIZE (1024 * 1024)
#define MAX_MSG_SIZE 1024

static const QString msgRaiseMainWindow("RaiseMainWindow");
static const QString msgCompose("Compose");

static const QChar msgTypeSep('>'); /*!< Separator used to divide message type name and the value. */

SingleInstance::SingleInstance(const QString &shMemKey, QObject *parent)
    : QObject(parent),
    m_memoryExisted(false),
    m_shMem()
{
	/*
	 * On UNIX the semaphore is not removed on crash.
	 * For now let's assume that the code does not crash here.
	 */
	QSystemSemaphore sema(UNIQUE_SEM_ID, 1);
	logInfoNL("Trying to acquire semaphore '%s'.", UNIQUE_SEM_ID);
	sema.acquire();
	logInfoNL("Semaphore '%s' acquired.", UNIQUE_SEM_ID);

	QString memKey(UNIQUE_MEM_ID);
	if (!shMemKey.isEmpty()) {
		memKey += "[" + shMemKey + "]";
	}
	logInfoNL("Shared memory key is '%s'.", memKey.toUtf8().constData());

#if !defined(Q_OS_WIN)
	/*
	 * On UNIX systems the memory is not freed upon crash.
	 * If there is any previous instance, clean it.
	 */
	{
		QSharedMemory shMem(memKey);
		if (shMem.attach()) {
			shMem.detach();
		}
	}
#endif /* !defined(Q_OS_WIN) */

	m_shMem.setKey(memKey);

	if (m_shMem.create(MEM_SIZE)) {
		m_shMem.lock();
		memset(m_shMem.data(), 0, MEM_SIZE);
		m_shMem.unlock();

		/* Start reading messages. */
		QTimer *timer = new QTimer(this);
		connect(timer, SIGNAL(timeout()), this, SLOT(checkMessage()));
		timer->start(200);

		logInfoNL("Application owns shared memory under key '%s'.",
		    memKey.toUtf8().constData());
	} else if (m_shMem.attach()) {
		m_memoryExisted = true;

		logInfoNL(
		    "Another application owns shared memory under key '%s'.",
		    memKey.toUtf8().constData());
	} else {
		logErrorNL("Cannot access shared memory under key '%s'.",
		    memKey.toUtf8().constData());
	}

	sema.release();
	logInfoNL("Semaphore '%s' released.", UNIQUE_SEM_ID);
}

bool SingleInstance::existsInSystem(void) const
{
	return m_memoryExisted;
}

/*!
 * @brief Converts message type to string description.
 *
 * @param[in] msgType Message type identifier.
 * @return String description.
 */
static
const QString &msgTypeToStr(int msgType)
{
	switch (msgType) {
	case SingleInstance::MTYPE_RAISE_MAIN_WIN:
		return msgRaiseMainWindow;
		break;
	case SingleInstance::MTYPE_COMPOSE:
		return msgCompose;
		break;
	default:
		/* Default to raise main window. */
		Q_ASSERT(0);
		return msgRaiseMainWindow;
		break;
	}
}

/*!
 * @brief Composes a message.
 *
 * @param[in] msgType Message type identifier.
 * @param[in] msgVal Message value (serialised data).
 * @return Message string.
 */
static
QString composeMessage(int msgType, const QString &msgVal)
{
	return msgTypeToStr(msgType) % msgTypeSep % msgVal;
}

bool SingleInstance::sendMessage(int msgType, const QString &msgVal)
{
#if 0
	/* Master process cannot send messages. */
	if (!m_memoryExisted){
		return false;
	}
#endif

	const QString message(composeMessage(msgType, msgVal));

	/* Only short messages can be sent. */
	if (message.size() > MAX_MSG_SIZE) {
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

/*!
 * @brief Converts message description string to numerical value.
 *
 * @param[in] typeStrRef String containing operation name.
 * @return Message type.
 */
static
enum SingleInstance::MsgType strToMsgType(const QStringRef &typeStrRef)
{
	if (typeStrRef == msgRaiseMainWindow) {
		return SingleInstance::MTYPE_RAISE_MAIN_WIN;
	} else if (typeStrRef == msgCompose) {
		return SingleInstance::MTYPE_COMPOSE;
	} else {
		Q_ASSERT(0);
		return SingleInstance::MTYPE_UNKNOWN;
	}
}

/*!
 * @brief Decomposes a message.
 *
 * @param[in]  message Received message.
 * @param[out] msgType Message type.
 * @param[out] msgValRef Message value (serialised data).
 * @return True if message type has been recognised.
 */
static
bool decomposeMessage(const QString &message,
    enum SingleInstance::MsgType &msgType, QStringRef &msgValRef)
{
	if (Q_UNLIKELY(message.isEmpty())) {
		Q_ASSERT(0);
		msgType = SingleInstance::MTYPE_UNKNOWN;
		msgValRef.clear();
		return false;
	}

	int sepIdx = message.indexOf(msgTypeSep);
	if (Q_UNLIKELY(sepIdx < 0)) {
		msgType = SingleInstance::MTYPE_UNKNOWN;
		msgValRef.clear();
		return false;
	}

	msgType = strToMsgType(message.leftRef(sepIdx));
	if ((sepIdx + 1) < message.size()) {
		/* Separator is not the last character in the message. */
		msgValRef = message.midRef(sepIdx + 1);
	} else {
		msgValRef.clear();
	}
	return msgType != SingleInstance::MTYPE_UNKNOWN;
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

		const QString message(
		    QString::fromUtf8((byteArray + '\0').constData()));

		enum MsgType msgType = MTYPE_UNKNOWN;
		QStringRef msgValRef;
		if (decomposeMessage(message, msgType, msgValRef)) {
			emit GlobInstcs::snglInstEmitterPtr->messageReceived(
			    msgType, msgValRef.toString());
		}
	}

	memset(m_shMem.data(), 0, MEM_SIZE);

	m_shMem.unlock();
}
