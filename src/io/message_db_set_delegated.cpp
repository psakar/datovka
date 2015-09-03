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

#include <QRegExp>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>

#include "message_db_set.h"
#include "src/log/log.h"

#define DB2 "db2"

/*!
 * @brief Attaches a database file to opened database.
 */
static
bool attachDb2(QSqlDatabase &db, const QString &attachFileName)
{
	QSqlQuery query(db);

	QString queryStr("ATTACH DATABASE :fileName AS " DB2);
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":fileName", attachFileName);
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	return true;

fail:
	return false;
}

/*!
 * @brief Detaches database file from opened database.
 */
static
bool detachDb2(QSqlDatabase &db)
{
	QSqlQuery query(db);

	QString queryStr = "DETACH DATABASE " DB2;
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	return true;

fail:
	return false;
}

/*!
 * @brief Converts year to secondary key string.
 */
static
QString _yrly_YearToSecondaryKey(const QString &year)
{
	static const QRegExp re(YEARLY_SEC_KEY_RE);

	if (re.exactMatch(year)) {
		return year;
	} else {
		return YEARLY_SEC_KEY_INVALID;
	}
}

QStringList MessageDbSet::_yrly_secKeysIn90Days(void) const
{
	static const QRegExp re("^" YEARLY_SEC_KEY_RE "$");

	QStringList keys = this->keys();
	if (keys.isEmpty()) {
		return keys;
	}

	keys = keys.filter(re);
	keys.sort();
	if (keys.size() > 1) {
		keys = keys.mid(keys.size() - 2, 2);
	}

	return keys;
}

DbMsgsTblModel *MessageDbSet::_sf_msgsRcvdModel(void)
{
	if (this->size() == 0) {
		MessageDb::dummyModel.setType(DbMsgsTblModel::DUMMY_RECEIVED);
		return &MessageDb::dummyModel;
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsRcvdModel();
}

DbMsgsTblModel *MessageDbSet::_yrly_msgsRcvdModel(void)
{
	Q_ASSERT(0);
	return NULL;
}

DbMsgsTblModel *MessageDbSet::msgsRcvdModel(void)
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgsRcvdModel();
		break;
	case DO_YEARLY:
		return _yrly_msgsRcvdModel();
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return NULL;
}

DbMsgsTblModel *MessageDbSet::_sf_msgsRcvdWithin90DaysModel(void)
{
	if (this->size() == 0) {
		MessageDb::dummyModel.setType(DbMsgsTblModel::DUMMY_RECEIVED);
		return &MessageDb::dummyModel;
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsRcvdWithin90DaysModel();
}

DbMsgsTblModel *MessageDbSet::_yrly_2dbs_msgsRcvdWithin90DaysModel(
    MessageDb &db, const QString &attachFileName)
{
	QSqlQuery query(db.m_db);
	DbMsgsTblModel *ret = 0;
	bool attached = false;
	QString queryStr;

	attached = attachDb2(db.m_db, attachFileName);
	if (!attached) {
		goto fail;
	}

	queryStr = "SELECT ";
	for (int i = 0; i < (MessageDb::receivedItemIds.size() - 2); ++i) {
		queryStr += MessageDb::receivedItemIds[i] + ", ";
	}
	queryStr += "(ifnull(r.message_id, 0) != 0) AS is_downloaded" ", ";
	queryStr += "ifnull(p.state, 0) AS process_status";
	queryStr += " FROM messages AS m "
	    "LEFT JOIN supplementary_message_data AS s "
	    "ON (m.dmID = s.message_id) "
	    "LEFT JOIN raw_message_data AS r "
	    "ON (m.dmId = r.message_id) "
	    "LEFT JOIN process_state AS p "
	    "ON (m.dmId = p.message_id) "
	    "WHERE "
	    "(s.message_type = :message_type)"
	    " and "
	    "(m.dmDeliveryTime >= date('now','-90 day'))"
	    " UNION "
	    "SELECT ";
	for (int i = 0; i < (MessageDb::receivedItemIds.size() - 2); ++i) {
		queryStr += MessageDb::receivedItemIds[i] + ", ";
	}
	queryStr += "(ifnull(r.message_id, 0) != 0) AS is_downloaded" ", ";
	queryStr += "ifnull(p.state, 0) AS process_status";
	queryStr += " FROM " DB2 ".messages AS m "
	    "LEFT JOIN " DB2 ".supplementary_message_data AS s "
	    "ON (m.dmID = s.message_id) "
	    "LEFT JOIN " DB2 ".raw_message_data AS r "
	    "ON (m.dmId = r.message_id) "
	    "LEFT JOIN " DB2 ".process_state AS p "
	    "ON (m.dmId = p.message_id) "
	    "WHERE "
	    "(s.message_type = :message_type)"
	    " and "
	    "(m.dmDeliveryTime >= date('now','-90 day'))";
	if (!query.prepare(queryStr)) {
		logErrorNL("Cannot prepare SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}
	query.bindValue(":message_type", MessageDb::TYPE_RECEIVED);
	if (!query.exec()) {
		logErrorNL("Cannot execute SQL query: %s.",
		    query.lastError().text().toUtf8().constData());
		goto fail;
	}

	db.m_sqlMsgsModel.clearOverridingData();
	db.m_sqlMsgsModel.setQuery(query);
	if (!db.m_sqlMsgsModel.setRcvdHeader()) {
		Q_ASSERT(0);
		goto fail;
	}

	ret = &db.m_sqlMsgsModel;

fail:
	if (attached) {
		detachDb2(db.m_db);
	}
	return ret;
}

DbMsgsTblModel *MessageDbSet::_yrly_msgsRcvdWithin90DaysModel(void)
{
	QStringList secKeys = _yrly_secKeysIn90Days();

	if (secKeys.size() == 0) {
		MessageDb::dummyModel.setType(DbMsgsTblModel::DUMMY_RECEIVED);
		return &MessageDb::dummyModel;
	} else if (secKeys.size() == 1) {
		/* Query only one database. */
		MessageDb *db = this->value(secKeys[0], NULL);
		if (NULL == db) {
			Q_ASSERT(0);
			return NULL;
		}
		return db->msgsRcvdWithin90DaysModel();
	} else {
		Q_ASSERT(secKeys.size() == 2);
		/* The models need to be attached. */

		MessageDb *db0 = this->value(secKeys[0], NULL);
		MessageDb *db1 = this->value(secKeys[1], NULL);
		if ((NULL == db0) || (NULL == db1)) {
			Q_ASSERT(0);
			return NULL;
		}

//		return db0->msgsRcvdWithin90DaysModel();
//		return db1->msgsRcvdWithin90DaysModel();
		/* TODO -- The following code does not work as expected. */
		return _yrly_2dbs_msgsRcvdWithin90DaysModel(*db0, db1->fileName());
	}

	Q_ASSERT(0);
	return NULL;
}

DbMsgsTblModel *MessageDbSet::msgsRcvdWithin90DaysModel(void)
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgsRcvdWithin90DaysModel();
		break;
	case DO_YEARLY:
		return _yrly_msgsRcvdWithin90DaysModel();
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return NULL;
}

DbMsgsTblModel *MessageDbSet::_sf_msgsRcvdInYearModel(const QString &year)
{
	if (this->size() == 0) {
		MessageDb::dummyModel.setType(DbMsgsTblModel::DUMMY_RECEIVED);
		return &MessageDb::dummyModel;
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsRcvdInYearModel(year);
}

DbMsgsTblModel *MessageDbSet::_yrly_msgsRcvdInYearModel(const QString &year)
{
	(void) year;
	Q_ASSERT(0);
	return NULL;
}

DbMsgsTblModel *MessageDbSet::msgsRcvdInYearModel(const QString &year)
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgsRcvdInYearModel(year);
		break;
	case DO_YEARLY:
		return _yrly_msgsRcvdInYearModel(year);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return NULL;
}

QStringList MessageDbSet::_sf_msgsRcvdYears(enum Sorting sorting) const
{
	if (this->size() == 0) {
		return QStringList();
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsRcvdYears(sorting);
}

QStringList MessageDbSet::_yrly_msgsRcvdYears(enum Sorting sorting) const
{
	if (this->size() == 0) {
		return QStringList();
	}

	QSet<QString> years;

	for (QMap<QString, MessageDb *>::const_iterator i = this->begin();
	     i != this->end(); ++i) {
		MessageDb *db = i.value();
		if (NULL == db) {
			Q_ASSERT(0);
			return QStringList();
		}
		years.unite(db->msgsRcvdYears(sorting).toSet());
	}

	QStringList list(years.toList());

	if (ASCENDING) {
		list.sort();
		return list;
	} else if (DESCENDING) {
		list.sort();
		QStringList reversed;
		foreach (const QString &str, list) {
			reversed.prepend(str);
		}
		return reversed;
	}

	return list;
}

QStringList MessageDbSet::msgsRcvdYears(enum Sorting sorting) const
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgsRcvdYears(sorting);
		break;
	case DO_YEARLY:
		return _yrly_msgsRcvdYears(sorting);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return QStringList();
}

QList< QPair<QString, int> > MessageDbSet::_sf_msgsRcvdYearlyCounts(
    enum Sorting sorting) const
{
	if (this->size() == 0) {
		return QList< QPair<QString, int> >();
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsRcvdYearlyCounts(sorting);
}

QList< QPair<QString, int> > MessageDbSet::_yrly_msgsRcvdYearlyCounts(
    enum Sorting sorting) const
{
	(void) sorting;
	Q_ASSERT(0);
	return QList< QPair<QString, int> >();
}

QList< QPair<QString, int> > MessageDbSet::msgsRcvdYearlyCounts(
    enum Sorting sorting) const
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgsRcvdYearlyCounts(sorting);
		break;
	case DO_YEARLY:
		return _yrly_msgsRcvdYearlyCounts(sorting);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return QList< QPair<QString, int> >();
}

int MessageDbSet::_sf_msgsRcvdUnreadWithin90Days(void) const
{
	if (this->size() == 0) {
		return 0;
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsRcvdUnreadWithin90Days();
}

int MessageDbSet::_yrly_msgsRcvdUnreadWithin90Days(void) const
{
	QStringList secKeys = _yrly_secKeysIn90Days();

	if (secKeys.size() == 0) {
		return 0;
	} else if (secKeys.size() == 1) {
		/* Query only one database. */
		MessageDb *db = this->value(secKeys[0], NULL);
		if (NULL == db) {
			Q_ASSERT(0);
			return -1;
		}
		return db->msgsRcvdUnreadWithin90Days();
	} else {
		Q_ASSERT(secKeys.size() == 2);
		/* The models need to be attached. */

		MessageDb *db0 = this->value(secKeys[0], NULL);
		MessageDb *db1 = this->value(secKeys[1], NULL);
		if ((NULL == db0) || (NULL == db1)) {
			Q_ASSERT(0);
			return -1;
		}

		int ret0 = db0->msgsRcvdUnreadWithin90Days();
		int ret1 = db1->msgsRcvdUnreadWithin90Days();
		if ((ret0 < 0) || (ret1 < 0)) {
			return -1;
		}

		return ret0 + ret1;
	}

	Q_ASSERT(0);
	return -1;
}

int MessageDbSet::msgsRcvdUnreadWithin90Days(void) const
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgsRcvdUnreadWithin90Days();
		break;
	case DO_YEARLY:
		return _yrly_msgsRcvdUnreadWithin90Days();
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return -1;
}

int MessageDbSet::_sf_msgsRcvdUnreadInYear(const QString &year) const
{
	if (this->size() == 0) {
		return 0;
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsRcvdUnreadInYear(year);
}

int MessageDbSet::_yrly_msgsRcvdUnreadInYear(const QString &year) const
{
	QString secondaryKey = _yrly_YearToSecondaryKey(year);

	MessageDb *db = this->value(secondaryKey, NULL);
	if (NULL == db) {
		return 0;
	}

	return db->msgsRcvdUnreadInYear(year);
}

int MessageDbSet::msgsRcvdUnreadInYear(const QString &year) const
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgsRcvdUnreadInYear(year);
		break;
	case DO_YEARLY:
		return _yrly_msgsRcvdUnreadInYear(year);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return -1;
}

DbMsgsTblModel *MessageDbSet::_sf_msgsSntModel(void)
{
	if (this->size() == 0) {
		MessageDb::dummyModel.setType(DbMsgsTblModel::DUMMY_SENT);
		return &MessageDb::dummyModel;
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsSntModel();
}

DbMsgsTblModel *MessageDbSet::_yrly_msgsSntModel(void)
{
	Q_ASSERT(0);
	return NULL;
}

DbMsgsTblModel *MessageDbSet::msgsSntModel(void)
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgsSntModel();
		break;
	case DO_YEARLY:
		return _yrly_msgsSntModel();
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return NULL;
}

DbMsgsTblModel *MessageDbSet::_sf_msgsSntWithin90DaysModel(void)
{
	if (this->size() == 0) {
		MessageDb::dummyModel.setType(DbMsgsTblModel::DUMMY_SENT);
		return &MessageDb::dummyModel;
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsSntWithin90DaysModel();
}

DbMsgsTblModel *MessageDbSet::_yrly_msgsSntWithin90DaysModel(void)
{
	Q_ASSERT(0);
	return NULL;
}

DbMsgsTblModel *MessageDbSet::msgsSntWithin90DaysModel(void)
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgsSntWithin90DaysModel();
		break;
	case DO_YEARLY:
		return _yrly_msgsSntWithin90DaysModel();
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return NULL;
}

DbMsgsTblModel *MessageDbSet::_sf_msgsSntInYearModel(const QString &year)
{
	if (this->size() == 0) {
		MessageDb::dummyModel.setType(DbMsgsTblModel::DUMMY_SENT);
		return &MessageDb::dummyModel;
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsSntInYearModel(year);
}

DbMsgsTblModel *MessageDbSet::_yrly_msgsSntInYearModel(const QString &year)
{
	(void) year;
	Q_ASSERT(0);
	return NULL;
}

DbMsgsTblModel *MessageDbSet::msgsSntInYearModel(const QString &year)
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgsSntInYearModel(year);
		break;
	case DO_YEARLY:
		return _yrly_msgsSntInYearModel(year);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return NULL;
}

QStringList MessageDbSet::_sf_msgsSntYears(enum Sorting sorting) const
{
	if (this->size() == 0) {
		return QStringList();
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsSntYears(sorting);
}

QStringList MessageDbSet::_yrly_msgsSntYears(enum Sorting sorting) const
{
	if (this->size() == 0) {
		return QStringList();
	}

	QSet<QString> years;

	for (QMap<QString, MessageDb *>::const_iterator i = this->begin();
	     i != this->end(); ++i) {
		MessageDb *db = i.value();
		if (NULL == db) {
			Q_ASSERT(0);
			return QStringList();
		}
		years.unite(db->msgsSntYears(sorting).toSet());
	}

	QStringList list(years.toList());

	if (ASCENDING) {
		list.sort();
		return list;
	} else if (DESCENDING) {
		list.sort();
		QStringList reversed;
		foreach (const QString &str, list) {
			reversed.prepend(str);
		}
		return reversed;
	}

	return list;
}

QStringList MessageDbSet::msgsSntYears(enum Sorting sorting) const
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgsSntYears(sorting);
		break;
	case DO_YEARLY:
		return _yrly_msgsSntYears(sorting);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return QStringList();
}

QList< QPair<QString, int> > MessageDbSet::_sf_msgsSntYearlyCounts(
    enum Sorting sorting) const
{
	if (this->size() == 0) {
		return QList< QPair<QString, int> >();
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsSntYearlyCounts(sorting);
}

QList< QPair<QString, int> > MessageDbSet::_yrly_msgsSntYearlyCounts(
    enum Sorting sorting) const
{
	(void) sorting;
	Q_ASSERT(0);
	return QList< QPair<QString, int> >();
}

QList< QPair<QString, int> > MessageDbSet::msgsSntYearlyCounts(
    enum Sorting sorting) const
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgsSntYearlyCounts(sorting);
		break;
	case DO_YEARLY:
		return _yrly_msgsSntYearlyCounts(sorting);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return QList< QPair<QString, int> >();
}

int MessageDbSet::_sf_msgsSntUnreadWithin90Days(void) const
{
	if (this->size() == 0) {
		return 0;
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsSntUnreadWithin90Days();
}

int MessageDbSet::_yrly_msgsSntUnreadWithin90Days(void) const
{
	if (this->size() == 0) {
		return 0;
	}
	Q_ASSERT(0);
	return -1;
}

int MessageDbSet::msgsSntUnreadWithin90Days(void) const
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgsSntUnreadWithin90Days();
		break;
	case DO_YEARLY:
		return _yrly_msgsSntUnreadWithin90Days();
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return -1;
}

int MessageDbSet::_sf_msgsSntUnreadInYear(const QString &year) const
{
	if (this->size() == 0) {
		return 0;
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsSntUnreadInYear(year);
}

int MessageDbSet::_yrly_msgsSntUnreadInYear(const QString &year) const
{
	QString secondaryKey = _yrly_YearToSecondaryKey(year);

	MessageDb *db = this->value(secondaryKey, NULL);
	if (NULL == db) {
		return 0;
	}

	return db->msgsSntUnreadInYear(year);
}

int MessageDbSet::msgsSntUnreadInYear(const QString &year) const
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgsSntUnreadInYear(year);
		break;
	case DO_YEARLY:
		return _yrly_msgsSntUnreadInYear(year);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return -1;
}

bool MessageDbSet::_sf_smsgdtSetAllReceivedLocallyRead(bool read)
{
	if (this->size() == 0) {
		return false;
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->smsgdtSetAllReceivedLocallyRead(read);
}

bool MessageDbSet::_yrly_smsgdtSetAllReceivedLocallyRead(bool read)
{
	(void) read;
	Q_ASSERT(0);
	return false;
}

bool MessageDbSet::smsgdtSetAllReceivedLocallyRead(bool read)
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_smsgdtSetAllReceivedLocallyRead(read);
		break;
	case DO_YEARLY:
		return _yrly_smsgdtSetAllReceivedLocallyRead(read);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return false;
}

bool MessageDbSet::_sf_smsgdtSetReceivedYearLocallyRead(const QString &year,
    bool read)
{
	if (this->size() == 0) {
		return false;
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->smsgdtSetReceivedYearLocallyRead(year, read);
}

bool MessageDbSet::_yrly_smsgdtSetReceivedYearLocallyRead(const QString &year,
    bool read)
{
	(void) year; (void) read;
	Q_ASSERT(0);
	return false;
}

bool MessageDbSet::smsgdtSetReceivedYearLocallyRead(const QString &year,
    bool read)
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_smsgdtSetReceivedYearLocallyRead(year, read);
		break;
	case DO_YEARLY:
		return _yrly_smsgdtSetReceivedYearLocallyRead(year, read);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return false;
}

bool MessageDbSet::_sf_smsgdtSetWithin90DaysReceivedLocallyRead(bool read)
{
	if (this->size() == 0) {
		return false;
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->smsgdtSetWithin90DaysReceivedLocallyRead(read);
}

bool MessageDbSet::_yrly_smsgdtSetWithin90DaysReceivedLocallyRead(bool read)
{
	(void) read;
	Q_ASSERT(0);
	return false;
}

bool MessageDbSet::smsgdtSetWithin90DaysReceivedLocallyRead(bool read)
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_smsgdtSetWithin90DaysReceivedLocallyRead(read);
		break;
	case DO_YEARLY:
		return _yrly_smsgdtSetWithin90DaysReceivedLocallyRead(read);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return false;
}

bool MessageDbSet::_sf_msgSetAllReceivedProcessState(
    enum MessageProcessState state)
{
	if (this->size() == 0) {
		return false;
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->msgSetAllReceivedProcessState(state);
}

bool MessageDbSet::_yrly_msgSetAllReceivedProcessState(
    enum MessageProcessState state)
{
	(void) state;
	Q_ASSERT(0);
	return false;
}

bool MessageDbSet::msgSetAllReceivedProcessState(enum MessageProcessState state)
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgSetAllReceivedProcessState(state);
		break;
	case DO_YEARLY:
		return _yrly_msgSetAllReceivedProcessState(state);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return false;
}

bool MessageDbSet::_sf_smsgdtSetReceivedYearProcessState(const QString &year,
    enum MessageProcessState state)
{
	if (this->size() == 0) {
		return false;
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->smsgdtSetReceivedYearProcessState(year, state);
}

bool MessageDbSet::_yrly_smsgdtSetReceivedYearProcessState(const QString &year,
    enum MessageProcessState state)
{
	(void) year; (void) state;
	Q_ASSERT(0);
	return false;
}

bool MessageDbSet::smsgdtSetReceivedYearProcessState(const QString &year,
    enum MessageProcessState state)
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_smsgdtSetReceivedYearProcessState(year, state);
		break;
	case DO_YEARLY:
		return _yrly_smsgdtSetReceivedYearProcessState(year, state);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return false;
}

bool MessageDbSet::_sf_smsgdtSetWithin90DaysReceivedProcessState(
    enum MessageProcessState state)
{
	if (this->size() == 0) {
		return false;
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->smsgdtSetWithin90DaysReceivedProcessState(state);
}

bool MessageDbSet::_yrly_smsgdtSetWithin90DaysReceivedProcessState(
    enum MessageProcessState state)
{
	(void) state;
	Q_ASSERT(0);
	return false;
}

bool MessageDbSet::smsgdtSetWithin90DaysReceivedProcessState(
    enum MessageProcessState state)
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_smsgdtSetWithin90DaysReceivedProcessState(state);
		break;
	case DO_YEARLY:
		return _yrly_smsgdtSetWithin90DaysReceivedProcessState(state);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return false;
}

QList< QVector<QString> > MessageDbSet::_sf_uniqueContacts(void) const
{
	if (this->size() == 0) {
		return QList< QVector<QString> >();
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->uniqueContacts();
}

QList< QVector<QString> > MessageDbSet::_yrly_uniqueContacts(void) const
{
	Q_ASSERT(0);
	return QList< QVector<QString> >();
}

QList< QVector<QString> > MessageDbSet::uniqueContacts(void) const
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_uniqueContacts();
		break;
	case DO_YEARLY:
		return _yrly_uniqueContacts();
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return QList< QVector<QString> >();
}

QList<MessageDb::MsgId> MessageDbSet::_sf_getAllMessageIDsFromDB(void) const
{
	if (this->size() == 0) {
		return QList<MessageDb::MsgId>();
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->getAllMessageIDsFromDB();
}

QList<MessageDb::MsgId> MessageDbSet::_yrly_getAllMessageIDsFromDB(void) const
{
	Q_ASSERT(0);
	return QList<MessageDb::MsgId>();
}

QList<MessageDb::MsgId> MessageDbSet::getAllMessageIDsFromDB(void) const
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_getAllMessageIDsFromDB();
		break;
	case DO_YEARLY:
		return _yrly_getAllMessageIDsFromDB();
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return QList<MessageDb::MsgId>();
}

QList<MessageDb::MsgId> MessageDbSet::_sf_msgsDateInterval(
    const QDate &fromDate, const QDate &toDate,
    enum MessageDirection msgDirect) const
{
	if (this->size() == 0) {
		return QList<MessageDb::MsgId>();
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsDateInterval(fromDate, toDate, msgDirect);
}

QList<MessageDb::MsgId> MessageDbSet::_yrly_msgsDateInterval(
    const QDate &fromDate, const QDate &toDate,
    enum MessageDirection msgDirect) const
{
	(void) fromDate; (void) toDate; (void) msgDirect;
	Q_ASSERT(0);
	return QList<MessageDb::MsgId>();
}

QList<MessageDb::MsgId> MessageDbSet::msgsDateInterval(const QDate &fromDate,
    const QDate &toDate, enum MessageDirection msgDirect) const
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgsDateInterval(fromDate, toDate, msgDirect);
		break;
	case DO_YEARLY:
		return _yrly_msgsDateInterval(fromDate, toDate, msgDirect);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return QList<MessageDb::MsgId>();
}

QList<MessageDb::SoughtMsg> MessageDbSet::_sf_msgsAdvancedSearchMessageEnvelope(
    qint64 dmId, const QString &dmAnnotation,
    const QString &dbIDSender, const QString &dmSender,
    const QString &dmAddress, const QString &dbIDRecipient,
    const QString &dmRecipient, const QString &dmSenderRefNumber,
    const QString &dmSenderIdent, const QString &dmRecipientRefNumber,
    const QString &dmRecipientIdent, const QString &dmToHands,
    const QString &dmDeliveryTime, const QString &dmAcceptanceTime,
    enum MessageDirection msgDirect) const
{
	if (this->size() == 0) {
		return QList<MessageDb::SoughtMsg>();
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsAdvancedSearchMessageEnvelope(dmId,
	    dmAnnotation, dbIDSender, dmSender, dmAddress, dbIDRecipient,
	    dmRecipient, dmSenderRefNumber, dmSenderIdent, dmRecipientRefNumber,
	    dmRecipientIdent, dmToHands, dmDeliveryTime, dmAcceptanceTime,
	    msgDirect);
}

QList<MessageDb::SoughtMsg> MessageDbSet::_yrly_msgsAdvancedSearchMessageEnvelope(
    qint64 dmId, const QString &dmAnnotation,
    const QString &dbIDSender, const QString &dmSender,
    const QString &dmAddress, const QString &dbIDRecipient,
    const QString &dmRecipient, const QString &dmSenderRefNumber,
    const QString &dmSenderIdent, const QString &dmRecipientRefNumber,
    const QString &dmRecipientIdent, const QString &dmToHands,
    const QString &dmDeliveryTime, const QString &dmAcceptanceTime,
    enum MessageDirection msgDirect) const
{
	(void) dmId; (void) dmAnnotation; (void) dbIDSender; (void) dmSender;
	(void) dmAddress; (void) dbIDRecipient; (void) dmRecipient;
	(void) dmSenderRefNumber; (void) dmSenderIdent;
	(void) dmRecipientRefNumber; (void) dmRecipientIdent; (void) dmToHands;
	(void) dmDeliveryTime; (void) dmAcceptanceTime; (void) msgDirect;
	Q_ASSERT(0);
	return QList<MessageDb::SoughtMsg>();
}

QList<MessageDb::SoughtMsg> MessageDbSet::msgsAdvancedSearchMessageEnvelope(
    qint64 dmId, const QString &dmAnnotation,
    const QString &dbIDSender, const QString &dmSender,
    const QString &dmAddress, const QString &dbIDRecipient,
    const QString &dmRecipient, const QString &dmSenderRefNumber,
    const QString &dmSenderIdent, const QString &dmRecipientRefNumber,
    const QString &dmRecipientIdent, const QString &dmToHands,
    const QString &dmDeliveryTime, const QString &dmAcceptanceTime,
    enum MessageDirection msgDirect) const
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgsAdvancedSearchMessageEnvelope(dmId, dmAnnotation,
		    dbIDSender, dmSender, dmAddress, dbIDRecipient, dmRecipient,
		    dmSenderRefNumber, dmSenderIdent, dmRecipientRefNumber,
		    dmRecipientIdent, dmToHands, dmDeliveryTime,
		    dmAcceptanceTime, msgDirect);
		break;
	case DO_YEARLY:
		return _yrly_msgsAdvancedSearchMessageEnvelope(dmId,
		    dmAnnotation, dbIDSender, dmSender, dmAddress,
		    dbIDRecipient, dmRecipient, dmSenderRefNumber,
		    dmSenderIdent, dmRecipientRefNumber, dmRecipientIdent,
		    dmToHands, dmDeliveryTime, dmAcceptanceTime, msgDirect);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return QList<MessageDb::SoughtMsg>();
}
