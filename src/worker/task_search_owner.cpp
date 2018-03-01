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

#include <cstdlib>
#include <cstring>
#include <QThread>

#include "src/global.h"
#include "src/io/isds_sessions.h"
#include "src/log/log.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_search_owner.h"

TaskSearchOwner::SoughtOwnerInfo::SoughtOwnerInfo(const QString &_id,
    enum BoxType _type, const QString &_ic, const QString &_firstName,
    const QString &_lastName, const QString &_firmName,
    const QString &_zipCode)
    : id(_id),
    type(_type),
    ic(_ic),
    firstName(_firstName),
    lastName(_lastName),
    firmName(_firmName),
    zipCode(_zipCode)
{
}

TaskSearchOwner::BoxEntry::BoxEntry(const QString &i, int t, const QString &n,
    const QString &ad, const QString &zc, bool &ovm)
    : id(i),
    type(t),
    name(n),
    address(ad),
    zipCode(zc),
    effectiveOVM(ovm)
{
}

TaskSearchOwner::TaskSearchOwner(const QString &userName,
    const SoughtOwnerInfo &soughtInfo)
    : m_result(SO_ERROR),
    m_isdsError(),
    m_isdsLongError(),
    m_foundBoxes(),
    m_userName(userName),
    m_soughtInfo(soughtInfo)
{
	Q_ASSERT(!m_userName.isEmpty());
}

void TaskSearchOwner::run(void)
{
	if (m_userName.isEmpty()) {
		Q_ASSERT(0);
		return;
	}

	logDebugLv0NL("Starting search owner task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	m_result = isdsSearch(m_userName, m_soughtInfo, m_foundBoxes,
	    m_isdsError, m_isdsLongError);

	emit GlobInstcs::msgProcEmitterPtr->progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Search owner task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

/*!
 * @brief Convert box type from task to libisds enum.
 *
 * @param[in] type Task enum type representation.
 * @return Libisds enum representation.
 */
static
isds_DbType convertType(enum TaskSearchOwner::BoxType type)
{
	switch (type) {
	case TaskSearchOwner::BT_OVM:
		return DBTYPE_OVM;
		break;
	case TaskSearchOwner::BT_PO:
		return DBTYPE_PO;
		break;
	case TaskSearchOwner::BT_PFO:
		return DBTYPE_PFO;
		break;
	case TaskSearchOwner::BT_FO:
		return DBTYPE_FO;
		break;
	default:
		Q_ASSERT(0);
		return DBTYPE_OVM;
		break;
	}
}

/*!
 * @brief Creates a new owner info structure used for data box searching.
 *
 * @param[in] soughtInfo Sought data.
 * @return Newly allocated structure set according to input.
 */
static
struct isds_DbOwnerInfo *ownerInfoFromSoughtInfo(
    const TaskSearchOwner::SoughtOwnerInfo &soughtInfo)
{
	struct isds_PersonName *personName = NULL;
	struct isds_Address *address = NULL;
	struct isds_DbOwnerInfo *ownerInfo = NULL;

	personName = isds_PersonName_create(soughtInfo.firstName, QString(),
	    soughtInfo.lastName, soughtInfo.lastName);
	if (NULL == personName) {
		goto fail;
	}

	address = isds_Address_create(QString(), QString(), QString(),
	    QString(), soughtInfo.zipCode, QString());
	if (NULL == address) {
		goto fail;
	}

	ownerInfo = isds_DbOwnerInfo_createConsume(soughtInfo.id,
	    convertType(soughtInfo.type), soughtInfo.ic, personName,
	    soughtInfo.firmName, NULL, address, QString(), QString(),
	    QString(), QString(), QString(), 0, false, false);
	if (NULL != ownerInfo) {
		personName = NULL;
		address = NULL;
	} else {
		goto fail;
	}

	return ownerInfo;

fail:
	isds_PersonName_free(&personName);
	isds_Address_free(&address);
	isds_DbOwnerInfo_free(&ownerInfo);
	return NULL;
}

/*!
 * @brief Adds entries from libisds list into antry list.
 *
 * @param[out] boxList List of boxes to append data to.
 * @param[in]  boxes List of boxes to copy data from.
 */
static
void appendBoxEntries(QList<TaskSearchOwner::BoxEntry> &foundBoxes,
    const struct isds_list *boxes)
{
	while (boxes != NULL) {
		const struct isds_DbOwnerInfo *found =
		    (struct isds_DbOwnerInfo *)boxes->data;

		if ((found->dbID == NULL) || (found->dbType == NULL)) {
			continue;
		}

		QString name;
		switch (*found->dbType) {
		case DBTYPE_FO:
			if (found->personName != NULL) {
				name = QString(found->personName->pnFirstName) +
				    " " + QString(found->personName->pnLastName);
			}
			break;
		case DBTYPE_PFO:
			if (found->personName != NULL) {
				QString firmName(found->firmName);
				if (firmName.isEmpty() || (firmName == " ")) {
					if (found->personName != NULL) {
						name = QString(found->personName->pnFirstName) +
						    " " + QString(found->personName->pnLastName);
					}
				} else {
					name = QString(found->firmName);
				}
			}
			break;
		default:
			break;
		}
		if (name.isEmpty()) { /* Defaults. */
			name = QString(found->firmName);
		}

		QString address, zipCode;
		if (found->address != NULL) {
			QString street(found->address->adStreet);
			QString adNumberInStreet(found->address->adNumberInStreet);
			QString adNumberInMunicipality(found->address->adNumberInMunicipality);

			if (street.isEmpty() || (street == " ")) {
				address = QString(found->address->adCity);
			} else {
				address = street;
				if (adNumberInStreet.isEmpty() ||
				    (adNumberInStreet == " ")) {
					address += + " " +
					    adNumberInMunicipality +
					    ", " + QString(found->address->adCity);
				} else if (adNumberInMunicipality.isEmpty() ||
				           (adNumberInMunicipality == " ")) {
					address += + " " +
					    adNumberInStreet +
					    ", " + QString(found->address->adCity);
				} else {
					address += + " "+
					    adNumberInMunicipality
					    + "/" +
					    adNumberInStreet +
					", " + QString(found->address->adCity);
				}
			}

			zipCode = QString(found->address->adZipCode);
		}

		bool ovm = (found->dbEffectiveOVM != NULL) ? (*found->dbEffectiveOVM) : false;

		foundBoxes.append(TaskSearchOwner::BoxEntry(
		    QString(found->dbID), *found->dbType, name, address,
		    zipCode, ovm));

		boxes = boxes->next;
	}
}

/*!
 * @brief Converts libisds error code into task error code.
 *
 * @param[in] status Libisds error status.
 * @return Task error code.
 */
static
enum TaskSearchOwner::Result convertError(int status)
{
	switch (status) {
	case IE_SUCCESS:
		return TaskSearchOwner::SO_SUCCESS;
		break;
	case IE_2BIG:
	case IE_NOEXIST:
	case IE_INVAL:
	case IE_INVALID_CONTEXT:
		return TaskSearchOwner::SO_BAD_DATA;
		break;
	case IE_ISDS:
	case IE_NOT_LOGGED_IN:
	case IE_CONNECTION_CLOSED:
	case IE_TIMED_OUT:
	case IE_NETWORK:
	case IE_HTTP:
	case IE_SOAP:
	case IE_XML:
		return TaskSearchOwner::SO_COM_ERROR;
		break;
	default:
		return TaskSearchOwner::SO_ERROR;
		break;
	}
}

enum TaskSearchOwner::Result TaskSearchOwner::isdsSearch(
    const QString &userName, const SoughtOwnerInfo &soughtInfo,
    QList<BoxEntry> &foundBoxes, QString &error, QString &longError)
{
	isds_error status = IE_ERROR;

	struct isds_ctx *session = GlobInstcs::isdsSessionsPtr->session(userName);
	if (NULL == session) {
		Q_ASSERT(0);
		return SO_ERROR;
	}

	struct isds_list *iFoundBoxes = NULL;

	struct isds_DbOwnerInfo *info = ownerInfoFromSoughtInfo(soughtInfo);
	if (info == NULL) {
		return SO_ERROR;
	}

	status = isds_FindDataBox(session, info, &iFoundBoxes);

	isds_DbOwnerInfo_free(&info);

	if (iFoundBoxes != NULL) {
		appendBoxEntries(foundBoxes, iFoundBoxes);
		isds_list_free(&iFoundBoxes);
	}

	if (IE_SUCCESS != status) {
		logErrorNL(
		    "Searching for data box returned status %d: '%s'.",
		    status, isdsStrError(status).toUtf8().constData());
		error = isds_error(status);
		longError = isdsLongMessage(session);
	} else {
		logDebugLv1NL("Find databox returned '%d': '%s'.",
		    status, isdsStrError(status).toUtf8().constData());
	}

	return convertError(status);
}
