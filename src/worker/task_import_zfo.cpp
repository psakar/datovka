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

#include <QByteArray>
#include <QFile>
#include <QThread>

#include "src/common.h"
#include "src/global.h"
#include "src/gui/dlg_import_zfo.h" /* TODO -- Remove this dependency. */
#include "src/io/account_db.h"
#include "src/io/dbs.h"
#include "src/io/isds_sessions.h"
#include "src/isds/message_conversion.h"
#include "src/log/log.h"
#include "src/settings/accounts.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_authenticate_message.h"
#include "src/worker/task_import_zfo.h"

TaskImportZfo::TaskImportZfo(const QList<Task::AccountDescr> &accounts,
    const QString &fileName, enum ZfoType type, bool authenticate)
    : m_fileName(fileName),
    m_result(IMP_ERR),
//    m_isdsError(),
//    m_isdsLongError(),
    m_resultDesc(),
    m_accounts(accounts),
    m_zfoType(type),
    m_auth(authenticate)
{
	Q_ASSERT(!m_fileName.isEmpty());
	Q_ASSERT(!m_accounts.isEmpty());

	if (ZT_UKNOWN == m_zfoType) {
		m_zfoType = determineFileType(m_fileName);
	}
}

void TaskImportZfo::run(void)
{
	if (m_fileName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	if (m_accounts.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	if (ZT_UKNOWN == m_zfoType) {
		m_result = IMP_DATA_ERROR;
		return;
	}

	logDebugLv0NL("Starting import ZFO task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	if (ZT_MESSAGE == m_zfoType) {
		m_result = importMessageZfo(m_accounts, m_fileName, m_auth,
		    m_resultDesc);
	} else if (ZT_DELIVERY_INFO == m_zfoType) {
		m_result = importDeliveryZfo(m_accounts, m_fileName, m_auth,
		    m_resultDesc);
	}

	emit GlobInstcs::msgProcEmitterPtr->importZfoFinished(m_fileName,
	    m_result, m_resultDesc);

	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Import ZFO task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

enum TaskImportZfo::ZfoType TaskImportZfo::determineFileType(
    const QString &fileName)
{
	debugFuncCall();

	ZfoType zfoType = ZT_UKNOWN;

	if (fileName.isEmpty()) {
		Q_ASSERT(0);
		return zfoType;
	}

	struct isds_ctx *dummySession = isds_ctx_create();
	if (NULL == dummySession) {
		logErrorNL("%s", "Cannot create dummy ISDS session.");
		return zfoType;
	}

	struct isds_message *message = loadZfoFile(dummySession, fileName,
	    Imports::IMPORT_MESSAGE);
	if (NULL != message) {
		zfoType = ZT_MESSAGE;
	} else {
		message = loadZfoFile(dummySession, fileName,
		    Imports::IMPORT_DELIVERY);
		if(NULL != message) {
			zfoType = ZT_DELIVERY_INFO;
		} else {
			zfoType = ZT_UKNOWN;
		}
	}

	isds_message_free(&message);
	isds_ctx_free(&dummySession);

	return zfoType;
}

/*!
 * @brief Loads ZFO file content into stuct isds_message.
 *
 * @param[in]  fileName Full path to ZFO file.
 * @param[in]  zfoType  Message of delivery info type.
 * @param[out] msgPtr   Pointer to null pointer to libisds message structure.
 * @return Error code.
 */
static
enum TaskImportZfo::Result loadZfo(const QString &fileName,
    enum Imports::Type zfoType, struct isds_message **msgPtr)
{
	Q_ASSERT((NULL != msgPtr) && (NULL == *msgPtr));
	Q_ASSERT((Imports::IMPORT_MESSAGE == zfoType) ||
	    (Imports::IMPORT_DELIVERY == zfoType));

	struct isds_message *message = NULL;

	{
		struct isds_ctx *dummy_session = isds_ctx_create();
		if (NULL == dummy_session) {
			logErrorNL("%s\n", "Cannot create dummy ISDS session.");
			return TaskImportZfo::IMP_ERR;
		}

		message = loadZfoFile(dummy_session, fileName, zfoType);
		isds_ctx_free(&dummy_session);
	}

	if ((NULL == message) || (NULL == message->envelope)) {
		logErrorNL("Wrong format of file '%s', expected delivery info.",
		    fileName.toUtf8().constData());
		isds_message_free(&message);
		return TaskImportZfo::IMP_DATA_ERROR;
	}

	*msgPtr = message;
	return TaskImportZfo::IMP_SUCCESS;
}

/*!
 * @brief Performs message file authentication.
 *
 * @param[in]  userName      Account identifier (user login name).
 * @param[in]  fileName      Full name of message file.
 * @param[out] isdsError     Error description.
 * @param[out] isdsLongError Long error description.
 * @return Authentication result.
 */
static
TaskAuthenticateMessage::Result authenticateMessageFile(const QString &userName,
    const QString &fileName, QString &isdsError, QString &isdsLongError)
{
	Q_ASSERT(!userName.isEmpty());
	Q_ASSERT(!fileName.isEmpty());

	QFile file(fileName);
	if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
		return TaskAuthenticateMessage::AUTH_DATA_ERROR;
	}

	QByteArray data(file.readAll());
	file.close();

	return TaskAuthenticateMessage::authenticateMessage(userName, data,
	    isdsError, isdsLongError);
}

enum TaskImportZfo::Result TaskImportZfo::importMessageZfoSingle(
    const Task::AccountDescr &acnt, const struct isds_message *message,
    qint64 dmId, const QDateTime &deliveryTime, enum MessageDirection direct,
    const QString &fileName, QString &isdsError, QString &isdsLongError,
    QString &resultDesc)
{
	Q_ASSERT(acnt.isValid());
	Q_ASSERT((NULL != message) && (NULL != message->envelope));

	MessageDb *messageDb = acnt.messageDbSet->accessMessageDb(deliveryTime,
	    true);
	if (NULL == messageDb) {
		logErrorNL("%s", "Cannot create or access database file.");
		resultDesc = QObject::tr("This file (message) has not been "
		    "inserted into the database because the corresponding "
		    "database file could not be accessed or created.");
		return TaskImportZfo::IMP_ERR;
	}
	const QString accountName(
	    (*GlobInstcs::acntMapPtr)[acnt.userName].accountName());
	if (-1 != messageDb->getMessageStatus(dmId)) {
		resultDesc = QObject::tr("Message '%1' already exists in "
		    "the local database, account '%2'.").
		    arg(dmId).arg(accountName);
		return TaskImportZfo::IMP_DB_EXISTS;
	}

	if (!fileName.isEmpty()) {
		TaskAuthenticateMessage::Result ret = authenticateMessageFile(
		    acnt.userName, fileName, isdsError, isdsLongError);
		switch (ret) {
		case TaskAuthenticateMessage::AUTH_SUCCESS:
			/* Continue with execution. */
			break;
		case TaskAuthenticateMessage::AUTH_DATA_ERROR:
			resultDesc = QObject::tr("Couldn't read data from "
			    "file for authentication on the ISDS server.");
			return TaskImportZfo::IMP_DATA_ERROR;
			break;
		case TaskAuthenticateMessage::AUTH_ISDS_ERROR:
			resultDesc = QObject::tr(
			    "Error contacting ISDS server.");
			return TaskImportZfo::IMP_ISDS_ERROR;
			break;
		default:
			resultDesc = QObject::tr("Message '%1' could not be "
			    "authenticated by ISDS server.").arg(dmId);
			return TaskImportZfo::IMP_AUTH_ERR;
			break;
		}
	}

	/* Store envelope and message. */
	bool ok = false;
	Isds::Envelope envel = Isds::libisds2envelope(message->envelope, &ok);
	if (!ok) {
		logErrorNL("%s", "Cannot convert libisds envelope to envelope.");
		return TaskImportZfo::IMP_ERR;
	}

	if ((Q_SUCCESS != Task::storeMessageEnvelope(direct,
	        *(acnt.messageDbSet), envel)) ||
	    (Q_SUCCESS != Task::storeMessage(true, direct, *(acnt.messageDbSet),
	        message, ""))) {
		resultDesc = QObject::tr("File has not been imported because "
		    "an error was detected during insertion process.");
		return TaskImportZfo::IMP_DB_INS_ERR;
	}

	resultDesc += QObject::tr("Imported message '%1', account '%2'.").
	    arg(dmId).arg(accountName);
	return TaskImportZfo::IMP_SUCCESS;
}

enum TaskImportZfo::Result TaskImportZfo::importMessageZfo(
    const QList<Task::AccountDescr> &accounts,
    const QString &fileName, bool authenticate, QString &resultDesc)
{
	struct isds_message *message = NULL;

	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_IMPORT_ZFO_MSG,
	    0);

	{
		enum TaskImportZfo::Result res = loadZfo(fileName,
		    Imports::IMPORT_MESSAGE, &message);
		if (IMP_SUCCESS != res) {
			Q_ASSERT(NULL == message);
			resultDesc = QObject::tr("Wrong ZFO format. This file "
			    "does not contain correct data for import.");
			return res;
		}

		Q_ASSERT(NULL != message);
	}

	const QString dbIDSender(message->envelope->dbIDSender);
	const QString dbIDRecipient(message->envelope->dbIDRecipient);

	qint64 dmId = QString(message->envelope->dmID).toLongLong();
	QDateTime deliveryTime =
	    timevalToDateTime(message->envelope->dmDeliveryTime);
	if (!deliveryTime.isValid()) {
		isds_message_free(&message);
		return IMP_DATA_ERROR;
	}

	QString importedDescr;
	QString existingDescr;
	QString errorDescr;
	QString isdsError, isdsLongError;
	bool imported = false;
	bool exists = false;

	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_IMPORT_ZFO_MSG,
	    10);

	float increment = 90.0 / accounts.size();

	foreach (const Task::AccountDescr &acnt, accounts) {
		isdsError.clear();
		isdsLongError.clear();
		resultDesc.clear();

		const QString databoxID(GlobInstcs::accntDbPtr->dbId(
		    AccountDb::keyFromLogin(acnt.userName)));
		if ((databoxID != dbIDSender) && (databoxID != dbIDRecipient)) {
			continue;
		}

		enum Result res = importMessageZfoSingle(acnt, message, dmId,
		    deliveryTime,
		    (databoxID == dbIDSender) ? MSG_SENT : MSG_RECEIVED,
		    authenticate ? fileName : QString(),
		    isdsError, isdsLongError, resultDesc);

		emit GlobInstcs::msgProcEmitterPtr->progressChange(
		    PL_IMPORT_ZFO_MSG, (int)(10 + increment));

		if (IMP_SUCCESS == res) {
			imported = true;
			importedDescr += resultDesc + "<br/>";
		} else if (IMP_DB_EXISTS == res) {
			exists = true;
			existingDescr += resultDesc + "<br/>";
		} else {
			errorDescr += resultDesc + "<br/>";
		}
	}

	isds_message_free(&message);

	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_IMPORT_ZFO_MSG,
	    100);

	if (imported) {
		resultDesc = importedDescr;
		return IMP_SUCCESS;
	} else if (exists) {
		resultDesc = existingDescr;
		return IMP_DB_EXISTS;
	} else {
		resultDesc = errorDescr;
		return IMP_ERR;
	}
}

enum TaskImportZfo::Result TaskImportZfo::importDeliveryZfoSingle(
    const Task::AccountDescr &acnt, const struct isds_message *message,
    qint64 dmId, const QDateTime &deliveryTime, const QString &fileName,
    QString &isdsError, QString &isdsLongError, QString &resultDesc)
{
	Q_ASSERT(acnt.isValid());
	Q_ASSERT((NULL != message) && (NULL != message->envelope));

	MessageDb *messageDb = acnt.messageDbSet->accessMessageDb(deliveryTime,
	    false);
	const QString accountName(
	    (*GlobInstcs::acntMapPtr)[acnt.userName].accountName());
	if ((NULL == messageDb) ||
	    (-1 == messageDb->getMessageStatus(dmId))) {
		/* Corresponding message does not exist in database. */
		resultDesc = QObject::tr("This file (acceptance info) has not "
		    "been inserted into database because there isn't any "
		    "related message with id '%1' in the databases.").arg(dmId);
		return TaskImportZfo::IMP_DB_MISSING_MSG;
	}

	if (!messageDb->getDeliveryInfoBase64(dmId).isEmpty()) {
		resultDesc = QObject::tr("Acceptance info for message '%1' "
		    "already exists in the local database, account '%2'.").
		    arg(dmId).arg(accountName);
		return TaskImportZfo::IMP_DB_EXISTS;
	}

	if (!fileName.isEmpty()) {
		TaskAuthenticateMessage::Result ret = authenticateMessageFile(
		    acnt.userName, fileName, isdsError, isdsLongError);
		switch (ret) {
		case TaskAuthenticateMessage::AUTH_SUCCESS:
			/* Continue with execution. */
			break;
		case TaskAuthenticateMessage::AUTH_DATA_ERROR:
			resultDesc = QObject::tr("Couldn't read data from "
			    "file for authentication on the ISDS server.");
			return TaskImportZfo::IMP_DATA_ERROR;
			break;
		case TaskAuthenticateMessage::AUTH_ISDS_ERROR:
			resultDesc = QObject::tr(
			    "Error contacting ISDS server.");
			return TaskImportZfo::IMP_ISDS_ERROR;
			break;
		default:
			resultDesc = QObject::tr("Acceptance info for message "
			    "'%1' could not be authenticated by ISDS server.").
			    arg(dmId);
			return TaskImportZfo::IMP_AUTH_ERR;
			break;
		}
	}

	bool ok = false;
	Isds::Message msg = Isds::libisds2message(message, &ok);
	if (!ok) {
		logErrorNL("%s", "Cannot convert libisds message to message.");
		return IMP_ERR;
	}

	if (Q_SUCCESS !=
	    Task::storeDeliveryInfo(true, *(acnt.messageDbSet), msg)) {
		resultDesc = QObject::tr("File has not been imported because "
		    "an error was detected during insertion process.");
		return TaskImportZfo::IMP_DB_INS_ERR;
	}

	resultDesc += QObject::tr(
	    "Imported acceptance info for message '%1', account '%2'.").
	    arg(dmId).arg(accountName);
	return TaskImportZfo::IMP_SUCCESS;
}

enum TaskImportZfo::Result TaskImportZfo::importDeliveryZfo(
    const QList<Task::AccountDescr> &accounts, const QString &fileName,
    bool authenticate, QString &resultDesc)
{
	struct isds_message *message = NULL;

	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_IMPORT_ZFO_DINFO,
	    0);

	{
		enum TaskImportZfo::Result res = loadZfo(fileName,
		    Imports::IMPORT_DELIVERY, &message);
		if (IMP_SUCCESS != res) {
			Q_ASSERT(NULL == message);
			resultDesc = QObject::tr("Wrong ZFO format. This file "
			    "does not contain correct data for import.");
			return res;
		}

		Q_ASSERT(NULL != message);
	}

	qint64 dmId = QString(message->envelope->dmID).toLongLong();
	QDateTime deliveryTime =
	    timevalToDateTime(message->envelope->dmDeliveryTime);
	if (!deliveryTime.isValid()) {
		isds_message_free(&message);
		return IMP_DATA_ERROR;
	}

	QString importedDescr;
	QString existingDescr;
	QString errorDescr;
	QString isdsError, isdsLongError;
	bool imported = false;
	bool exists = false;

	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_IMPORT_ZFO_DINFO,
	    10);

	float increment = 90.0 / accounts.size();

	foreach (const Task::AccountDescr &acnt, accounts) {
		isdsError.clear();
		isdsLongError.clear();
		resultDesc.clear();

		enum Result res = importDeliveryZfoSingle(acnt, message, dmId,
		    deliveryTime, authenticate ? fileName : QString(),
		    isdsError, isdsLongError, resultDesc);

		emit GlobInstcs::msgProcEmitterPtr->progressChange(
		    PL_IMPORT_ZFO_DINFO, (int)(10 + increment));

		if (IMP_SUCCESS == res) {
			imported = true;
			importedDescr += resultDesc + "<br/>";
		} else if (IMP_DB_EXISTS == res) {
			exists = true;
			existingDescr += resultDesc + "<br/>";
		} else {
			errorDescr += resultDesc + "<br/>";
		}
	}

	isds_message_free(&message);

	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_IMPORT_ZFO_DINFO,
	    100);

	if (imported) {
		resultDesc = importedDescr;
		return IMP_SUCCESS;
	} else if (exists) {
		resultDesc = existingDescr;
		return IMP_DB_EXISTS;
	} else {
		resultDesc = errorDescr;
		return IMP_ERR;
	}
}
