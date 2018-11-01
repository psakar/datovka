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

#include <QRegExp>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>

#include "src/io/message_db_set.h"

bool MessageDbSet::vacuum(void)
{
	for (QMap<QString, MessageDb *>::const_iterator i = this->begin();
	     i != this->end(); ++i) {
		MessageDb *db = i.value();
		if (Q_UNLIKELY(Q_NULLPTR == db)) {
			Q_ASSERT(0);
			return false;
		}
		if (!db->vacuum()) {
			return false;
		}
	}

	return true;
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

/*!
 * @brief Return list of years that match the span from (now-90days) to now.
 */
static
QStringList yearsInPast90Days(void)
{
	const QDate currentDate(QDate::currentDate());
	const QDate dateBefore90Days(currentDate.addDays(-90));

	QStringList years;
	years.append(currentDate.toString("yyyy"));
	if (currentDate.year() != dateBefore90Days.year()) {
		years.append(dateBefore90Days.toString("yyyy"));
	}
	return years;
}

QStringList MessageDbSet::_yrly_secKeysIn90Days(void) const
{
	static const QRegExp re("^" YEARLY_SEC_KEY_RE "$");

	QStringList keys = this->keys();
	if (keys.isEmpty()) {
		return keys;
	}

	keys = keys.filter(re);
	/* Intersection of found and expected years. */
	keys = keys.toSet().intersect(yearsInPast90Days().toSet()).toList();
	keys.sort();

	Q_ASSERT(keys.size() <= 2);
	return keys;
}

QList<MessageDb::RcvdEntry> MessageDbSet::_sf_msgsRcvdEntries(void) const
{
	if (this->size() == 0) {
		return QList<MessageDb::RcvdEntry>();
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsRcvdEntries();
}

QList<MessageDb::RcvdEntry> MessageDbSet::_yrly_msgsRcvdEntries(void) const
{
	/* TODO -- Implementation missing and will probably be missing. */
	Q_ASSERT(0);
	return QList<MessageDb::RcvdEntry>();
}

QList<MessageDb::RcvdEntry> MessageDbSet::msgsRcvdEntries(void) const
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgsRcvdEntries();
		break;
	case DO_YEARLY:
		return _yrly_msgsRcvdEntries();
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return QList<MessageDb::RcvdEntry>();
}

QList<MessageDb::RcvdEntry> MessageDbSet::_sf_msgsRcvdEntriesWithin90Days(void) const
{
	if (this->size() == 0) {
		return QList<MessageDb::RcvdEntry>();
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsRcvdEntriesWithin90Days();
}

QList<MessageDb::RcvdEntry> MessageDbSet::_yrly_2dbs_msgsRcvdEntriesWithin90Days(
    MessageDb &db0, MessageDb &db1)
{
	QList<MessageDb::RcvdEntry> entryList;

	{
		QSqlQuery query(db0.accessDb());

		if (!db0.msgsRcvdWithin90DaysQuery(query)) {
			goto fail;
		}

		MessageDb::appendRcvdEntryList(entryList, query);
	}

	{
		QSqlQuery query(db1.accessDb());

		if (!db1.msgsRcvdWithin90DaysQuery(query)) {
			goto fail;
		}

		MessageDb::appendRcvdEntryList(entryList, query);
	}

fail:
	return entryList;
}

QList<MessageDb::RcvdEntry> MessageDbSet::_yrly_msgsRcvdEntriesWithin90Days(void) const
{
	QList<MessageDb::RcvdEntry> entryList;
	QStringList secKeys = _yrly_secKeysIn90Days();

	if (secKeys.size() == 0) {
		return QList<MessageDb::RcvdEntry>();
	} else if (secKeys.size() == 1) {
		/* Query only one database. */
		MessageDb *db = this->value(secKeys[0], Q_NULLPTR);
		if (Q_NULLPTR == db) {
			Q_ASSERT(0);
			return QList<MessageDb::RcvdEntry>();
		}
		return db->msgsRcvdEntriesWithin90Days();
	} else {
		Q_ASSERT(secKeys.size() == 2);
		/* The models need to be attached. */

		MessageDb *db0 = this->value(secKeys[0], Q_NULLPTR);
		MessageDb *db1 = this->value(secKeys[1], Q_NULLPTR);
		if ((Q_NULLPTR == db0) || (Q_NULLPTR == db1)) {
			Q_ASSERT(0);
			return QList<MessageDb::RcvdEntry>();
		}

		return _yrly_2dbs_msgsRcvdEntriesWithin90Days(*db0, *db1);
	}
}

QList<MessageDb::RcvdEntry> MessageDbSet::msgsRcvdEntriesWithin90Days(void) const
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgsRcvdEntriesWithin90Days();
		break;
	case DO_YEARLY:
		return _yrly_msgsRcvdEntriesWithin90Days();
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return QList<MessageDb::RcvdEntry>();
}

QList<MessageDb::RcvdEntry> MessageDbSet::_sf_msgsRcvdEntriesInYear(
    const QString &year) const
{
	if (this->size() == 0) {
		return QList<MessageDb::RcvdEntry>();
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsRcvdEntriesInYear(year);
}

QList<MessageDb::RcvdEntry> MessageDbSet::_yrly_msgsRcvdEntriesInYear(
    const QString &year) const
{
	QString secondaryKey = _yrly_YearToSecondaryKey(year);

	MessageDb *db = this->value(secondaryKey, Q_NULLPTR);
	if (Q_NULLPTR == db) {
		return QList<MessageDb::RcvdEntry>();
	}

	return db->msgsRcvdEntriesInYear(year);
}

QList<MessageDb::RcvdEntry> MessageDbSet::msgsRcvdEntriesInYear(
    const QString &year) const
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgsRcvdEntriesInYear(year);
		break;
	case DO_YEARLY:
		return _yrly_msgsRcvdEntriesInYear(year);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return QList<MessageDb::RcvdEntry>();
}

QStringList MessageDbSet::_sf_msgsYears(enum MessageDb::MessageType type,
    enum Sorting sorting) const
{
	if (this->size() == 0) {
		return QStringList();
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsYears(type, sorting);
}

QStringList MessageDbSet::_yrly_msgsYears(enum MessageDb::MessageType type,
    enum Sorting sorting) const
{
	if (this->size() == 0) {
		return QStringList();
	}

	QSet<QString> years;

	for (QMap<QString, MessageDb *>::const_iterator i = this->begin();
	     i != this->end(); ++i) {
		MessageDb *db = i.value();
		if (Q_UNLIKELY(Q_NULLPTR == db)) {
			Q_ASSERT(0);
			return QStringList();
		}

		if (db->isOpen()) {
			years.unite(db->msgsYears(type, sorting).toSet());
		} else {
			years.insert(i.key());
		}
	}

	QStringList list(years.toList());

	if (sorting == ASCENDING) {
		list.sort();
		return list;
	} else if (sorting == DESCENDING) {
		list.sort();
		QStringList reversed;
		foreach (const QString &str, list) {
			reversed.prepend(str);
		}
		/* Keep invalid year always last. */
		if ((reversed.size() > 1) &&
		    (reversed.first() == INVALID_YEAR)) {
			reversed.removeFirst();
			reversed.append(INVALID_YEAR);
		}
		return reversed;
	}

	return list;
}

QStringList MessageDbSet::msgsYears(enum MessageDb::MessageType type,
    enum Sorting sorting) const
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgsYears(type, sorting);
		break;
	case DO_YEARLY:
		return _yrly_msgsYears(type, sorting);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return QStringList();
}

QList< QPair<QString, int> > MessageDbSet::_sf_msgsYearlyCounts(
    enum MessageDb::MessageType type, enum Sorting sorting) const
{
	if (this->size() == 0) {
		return QList< QPair<QString, int> >();
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsYearlyCounts(type, sorting);
}

QList< QPair<QString, int> > MessageDbSet::_yrly_msgsYearlyCounts(
    enum MessageDb::MessageType type, enum Sorting sorting) const
{
	if (this->size() == 0) {
		return QList< QPair<QString, int> >();
	}

	QSet<QString> years;
	QMap<QString, int> counts;

	for (QMap<QString, MessageDb *>::const_iterator i = this->begin();
	     i != this->end(); ++i) {
		MessageDb *db = i.value();
		if (Q_UNLIKELY(Q_NULLPTR == db)) {
			Q_ASSERT(0);
			return QList< QPair<QString, int> >();
		}

		typedef QPair<QString, int> StoredPair;
		QList< StoredPair > obtained(
		    db->msgsYearlyCounts(type, sorting));

		foreach (const StoredPair &pair, obtained) {
			years.insert(pair.first);
			if (counts.find(pair.first) != counts.end()) {
				counts[pair.first] += pair.second;
			} else {
				counts.insert(pair.first, pair.second);
			}
		}
	}

	QStringList list(years.toList());

	if (sorting == ASCENDING) {
		list.sort();
	} else if (sorting == DESCENDING) {
		list.sort();
		QStringList reversed;
		foreach (const QString &str, list) {
			reversed.prepend(str);
		}
		/* Keep invalid year always last. */
		if ((reversed.size() > 1) &&
		    (reversed.first() == INVALID_YEAR)) {
			reversed.removeFirst();
			reversed.append(INVALID_YEAR);
		}
		list = reversed;
	}

	QList< QPair<QString, int> > yearlyCounts;
	foreach (const QString &year, list) {
		yearlyCounts.append(QPair<QString, int>(year, counts[year]));
	}

	return yearlyCounts;
}

QList< QPair<QString, int> > MessageDbSet::msgsYearlyCounts(
    enum MessageDb::MessageType type, enum Sorting sorting) const
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgsYearlyCounts(type, sorting);
		break;
	case DO_YEARLY:
		return _yrly_msgsYearlyCounts(type, sorting);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return QList< QPair<QString, int> >();
}

int MessageDbSet::_sf_msgsUnreadWithin90Days(
    enum MessageDb::MessageType type) const
{
	if (this->size() == 0) {
		return 0;
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsUnreadWithin90Days(type);
}

int MessageDbSet::_yrly_msgsUnreadWithin90Days(
    enum MessageDb::MessageType type) const
{
	QStringList secKeys = _yrly_secKeysIn90Days();

	if (secKeys.size() == 0) {
		return 0;
	} else if (secKeys.size() == 1) {
		/* Query only one database. */
		MessageDb *db = this->value(secKeys[0], Q_NULLPTR);
		if (Q_UNLIKELY(Q_NULLPTR == db)) {
			Q_ASSERT(0);
			return -1;
		}
		return db->msgsUnreadWithin90Days(type);
	} else {
		Q_ASSERT(secKeys.size() == 2);
		/* The models need to be attached. */

		MessageDb *db0 = this->value(secKeys[0], Q_NULLPTR);
		MessageDb *db1 = this->value(secKeys[1], Q_NULLPTR);
		if (Q_UNLIKELY((Q_NULLPTR == db0) || (Q_NULLPTR == db1))) {
			Q_ASSERT(0);
			return -1;
		}

		int ret0 = db0->msgsUnreadWithin90Days(type);
		int ret1 = db1->msgsUnreadWithin90Days(type);
		if ((ret0 < 0) || (ret1 < 0)) {
			return -1;
		}

		return ret0 + ret1;
	}

	Q_ASSERT(0);
	return -1;
}

int MessageDbSet::msgsUnreadWithin90Days(enum MessageDb::MessageType type) const
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgsUnreadWithin90Days(type);
		break;
	case DO_YEARLY:
		return _yrly_msgsUnreadWithin90Days(type);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return -1;
}

int MessageDbSet::_sf_msgsUnreadInYear(enum MessageDb::MessageType type,
    const QString &year) const
{
	if (this->size() == 0) {
		return 0;
	}
	Q_ASSERT(this->size() == 1);
	if (type != MessageDb::TYPE_SENT) {
		return this->first()->msgsUnreadInYear(type, year);
	} else {
		return 0; /* The read state on sent messages is ignored. */
	}
}

int MessageDbSet::_yrly_msgsUnreadInYear(enum MessageDb::MessageType type,
    const QString &year) const
{
	QString secondaryKey = _yrly_YearToSecondaryKey(year);

	MessageDb *db = this->value(secondaryKey, Q_NULLPTR);
	if (Q_UNLIKELY(Q_NULLPTR == db)) {
		return 0;
	}

	if (db->isOpen()) {
		return db->msgsUnreadInYear(type, year);
	} else {
		return -2;
	}
}

int MessageDbSet::msgsUnreadInYear(enum MessageDb::MessageType type,
    const QString &year) const
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgsUnreadInYear(type, year);
		break;
	case DO_YEARLY:
		return _yrly_msgsUnreadInYear(type, year);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return -1;
}

QList<MessageDb::SntEntry> MessageDbSet::_sf_msgsSntEntries(void) const
{
	if (this->size() == 0) {
		return QList<MessageDb::SntEntry>();
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsSntEntries();
}

QList<MessageDb::SntEntry> MessageDbSet::_yrly_msgsSntEntries(void) const
{
	/* TODO -- Implementation missing and will probably be missing. */
	Q_ASSERT(0);
	return QList<MessageDb::SntEntry>();
}

QList<MessageDb::SntEntry> MessageDbSet::msgsSntEntries(void) const
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgsSntEntries();
		break;
	case DO_YEARLY:
		return _yrly_msgsSntEntries();
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return QList<MessageDb::SntEntry>();
}

QList<MessageDb::SntEntry> MessageDbSet::_sf_msgsSntEntriesWithin90Days(void) const
{
	if (this->size() == 0) {
		return QList<MessageDb::SntEntry>();
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsSntEntriesWithin90Days();
}

QList<MessageDb::SntEntry> MessageDbSet::_yrly_2dbs_msgsSntEntriesWithin90Days(
    MessageDb &db0, MessageDb &db1)
{
	QList<MessageDb::SntEntry> entryList;

	{
		QSqlQuery query(db0.accessDb());

		if (!db0.msgsSntWithin90DaysQuery(query)) {
			goto fail;
		}

		MessageDb::appendSntEntryList(entryList, query);
	}

	{
		QSqlQuery query(db1.accessDb());

		if (!db1.msgsSntWithin90DaysQuery(query)) {
			goto fail;
		}

		MessageDb::appendSntEntryList(entryList, query);
	}

fail:
	return entryList;
}

QList<MessageDb::SntEntry> MessageDbSet::_yrly_msgsSntEntriesWithin90Days(void) const
{
	QStringList secKeys = _yrly_secKeysIn90Days();

	if (secKeys.size() == 0) {
		return QList<MessageDb::SntEntry>();
	} else if (secKeys.size() == 1) {
		/* Query only one database. */
		MessageDb *db = this->value(secKeys[0], Q_NULLPTR);
		if (Q_NULLPTR == db) {
			Q_ASSERT(0);
			return QList<MessageDb::SntEntry>();
		}
		return db->msgsSntEntriesWithin90Days();
	} else {
		Q_ASSERT(secKeys.size() == 2);
		/* The models need to be attached. */

		MessageDb *db0 = this->value(secKeys[0], Q_NULLPTR);
		MessageDb *db1 = this->value(secKeys[1], Q_NULLPTR);
		if ((Q_NULLPTR == db0) || (Q_NULLPTR == db1)) {
			Q_ASSERT(0);
			return QList<MessageDb::SntEntry>();
		}

		return _yrly_2dbs_msgsSntEntriesWithin90Days(*db0, *db1);
	}
}

QList<MessageDb::SntEntry> MessageDbSet::msgsSntEntriesWithin90Days(void) const
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgsSntEntriesWithin90Days();
		break;
	case DO_YEARLY:
		return _yrly_msgsSntEntriesWithin90Days();
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return QList<MessageDb::SntEntry>();
}

QList<MessageDb::SntEntry> MessageDbSet::_sf_msgsSntEntriesInYear(
    const QString &year) const
{
	if (this->size() == 0) {
		return QList<MessageDb::SntEntry>();
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsSntEntriesInYear(year);
}

QList<MessageDb::SntEntry> MessageDbSet::_yrly_msgsSntEntriesInYear(
    const QString &year) const
{
	QString secondaryKey = _yrly_YearToSecondaryKey(year);

	MessageDb *db = this->value(secondaryKey, Q_NULLPTR);
	if (Q_NULLPTR == db) {
		return QList<MessageDb::SntEntry>();
	}

	return db->msgsSntEntriesInYear(year);
}

QList<MessageDb::SntEntry> MessageDbSet::msgsSntEntriesInYear(
    const QString &year) const
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgsSntEntriesInYear(year);
		break;
	case DO_YEARLY:
		return _yrly_msgsSntEntriesInYear(year);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return QList<MessageDb::SntEntry>();
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
	bool ret = true;

	for (QMap<QString, MessageDb *>::iterator i = this->begin();
	     i != this->end(); ++i)
	{
		MessageDb *db = i.value();
		if (Q_UNLIKELY(Q_NULLPTR == db)) {
			Q_ASSERT(0);
			return false;
		}

		ret = ret && db->smsgdtSetAllReceivedLocallyRead(read);
	}

	return ret;
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
	QString secondaryKey = _yrly_YearToSecondaryKey(year);

	MessageDb *db = this->value(secondaryKey, Q_NULLPTR);
	if (Q_UNLIKELY(Q_NULLPTR == db)) {
		return false;
	}

	return db->smsgdtSetReceivedYearLocallyRead(year, read);
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
	QStringList secKeys = _yrly_secKeysIn90Days();

	if (secKeys.size() == 0) {
		return false;
	} else if (secKeys.size() == 1) {
		/* Query only one database. */
		MessageDb *db = this->value(secKeys[0], Q_NULLPTR);
		if (Q_UNLIKELY(Q_NULLPTR == db)) {
			Q_ASSERT(0);
			return false;
		}
		return db->smsgdtSetWithin90DaysReceivedLocallyRead(read);
	} else {
		Q_ASSERT(secKeys.size() == 2);
		/* The models need to be attached. */

		bool ret = true;
		foreach (const QString &secKey, secKeys) {
			MessageDb *db = this->value(secKey, Q_NULLPTR);
			if (Q_UNLIKELY(Q_NULLPTR == db)) {
				Q_ASSERT(0);
				return false;
			}

			ret = ret &&
			    db->smsgdtSetWithin90DaysReceivedLocallyRead(read);
		}

		return ret;
	}

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
	return this->first()->setReceivedMessagesProcessState(state);
}

bool MessageDbSet::_yrly_msgSetAllReceivedProcessState(
    enum MessageProcessState state)
{
	bool ret = true;

	for (QMap<QString, MessageDb *>::iterator i = this->begin();
	     i != this->end(); ++i)
	{
		MessageDb *db = i.value();
		if (Q_UNLIKELY(Q_NULLPTR == db)) {
			Q_ASSERT(0);
			return false;
		}

		ret = ret && db->setReceivedMessagesProcessState(state);
	}

	return ret;
}

bool MessageDbSet::setReceivedMessagesProcessState(enum MessageProcessState state)
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
	QString secondaryKey = _yrly_YearToSecondaryKey(year);

	MessageDb *db = this->value(secondaryKey, Q_NULLPTR);
	if (Q_UNLIKELY(Q_NULLPTR == db)) {
		return false;
	}

	return db->smsgdtSetReceivedYearProcessState(year, state);
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
	QStringList secKeys = _yrly_secKeysIn90Days();

	if (secKeys.size() == 0) {
		return false;
	} else if (secKeys.size() == 1) {
		/* Query only one database. */
		MessageDb *db = this->value(secKeys[0], Q_NULLPTR);
		if (Q_UNLIKELY(Q_NULLPTR == db)) {
			Q_ASSERT(0);
			return false;
		}
		return db->smsgdtSetWithin90DaysReceivedProcessState(state);
	} else {
		Q_ASSERT(secKeys.size() == 2);
		/* The models need to be attached. */

		bool ret = true;
		foreach (const QString &secKey, secKeys) {
			MessageDb *db = this->value(secKey, Q_NULLPTR);
			if (Q_UNLIKELY(Q_NULLPTR == db)) {
				Q_ASSERT(0);
				return false;
			}

			ret = ret &&
			    db->smsgdtSetWithin90DaysReceivedProcessState(
			        state);
		}

		return ret;
	}

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

MessageDb::MsgId MessageDbSet::_sf_msgsMsgId(qint64 dmId) const
{
	if (this->size() == 0) {
		return MessageDb::MsgId();
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsMsgId(dmId);
}

MessageDb::MsgId MessageDbSet::_yrly_msgsMsgId(qint64 dmId) const
{
	MessageDb::MsgId soFar;

	for (QMap<QString, MessageDb *>::const_iterator i = this->begin();
	     i != this->end(); ++i) {
		MessageDb *db = i.value();
		MessageDb::MsgId found = db->msgsMsgId(dmId);

		if (found.dmId >= 0) {
			if (soFar.dmId >= 0) {
				Q_ASSERT(0);
			}
			soFar = found;
		}
	}

	return soFar;
}

MessageDb::MsgId MessageDbSet::msgsMsgId(qint64 dmId) const
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgsMsgId(dmId);
		break;
	case DO_YEARLY:
		return _yrly_msgsMsgId(dmId);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return MessageDb::MsgId();
}

QList<MessageDb::ContactEntry> MessageDbSet::_sf_uniqueContacts(void) const
{
	if (this->size() == 0) {
		return QList<MessageDb::ContactEntry>();
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->uniqueContacts();
}

/*!
 * @brief Merge unique contacts, prefer larger message ids.
 *
 * @param
 */
void mergeUniqueContacts(QMap<QString, MessageDb::ContactEntry> &tgt,
    const QList<MessageDb::ContactEntry> &src)
{
	QMap<QString, MessageDb::ContactEntry>::iterator found;

	foreach (const MessageDb::ContactEntry &entry, src) {
		found = tgt.find(entry.boxId);
		if (tgt.end() == found) {
			tgt.insert(entry.boxId, entry);
		} else if (found->dmId < entry.dmId) {
			/* Prefer entries with larger ids. */
			*found = entry;
		}
	}
}

QList<MessageDb::ContactEntry> MessageDbSet::_yrly_uniqueContacts(void) const
{
	QMap<QString, MessageDb::ContactEntry> unique;

	for (QMap<QString, MessageDb *>::const_iterator i = this->begin();
	     i != this->end(); ++i) {
		MessageDb *db = i.value();
		if (Q_UNLIKELY(Q_NULLPTR == db)) {
			Q_ASSERT(0);
			return QList<MessageDb::ContactEntry>();
		}

		mergeUniqueContacts(unique, db->uniqueContacts());
	}

	QList<MessageDb::ContactEntry> list;

	foreach (const MessageDb::ContactEntry &entry, unique) {
		list.append(entry);
	}

	return list;
}

QList<MessageDb::ContactEntry> MessageDbSet::uniqueContacts(void) const
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

	return QList<MessageDb::ContactEntry>();
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
	QList<MessageDb::MsgId> msgIds;

	for (QMap<QString, MessageDb *>::const_iterator i = this->begin();
	     i != this->end(); ++i) {
		MessageDb *db = i.value();
		if (Q_UNLIKELY(Q_NULLPTR == db)) {
			Q_ASSERT(0);
			return QList<MessageDb::MsgId>();
		}

		msgIds.append(db->getAllMessageIDsFromDB());
	}

	return msgIds;
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

QList<qint64> MessageDbSet::_sf_getAllMessageIDsWithoutAttach(void) const
{
	if (this->size() == 0) {
		return QList<qint64>();
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->getAllMessageIDsWithoutAttach();
}

QList<qint64> MessageDbSet::_yrly_getAllMessageIDsWithoutAttach(void) const
{
	QList<qint64> msgIds;

	for (QMap<QString, MessageDb *>::const_iterator i = this->begin();
	     i != this->end(); ++i) {
		MessageDb *db = i.value();
		if (Q_NULLPTR == db) {
			Q_ASSERT(0);
			return QList<qint64>();
		}

		msgIds.append(db->getAllMessageIDsWithoutAttach());
	}

	return msgIds;
}

QList<qint64> MessageDbSet::getAllMessageIDsWithoutAttach(void) const
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_getAllMessageIDsWithoutAttach();
		break;
	case DO_YEARLY:
		return _yrly_getAllMessageIDsWithoutAttach();
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return QList<qint64>();
}

QList<qint64> MessageDbSet::_sf_getAllMessageIDs(
    enum MessageDb::MessageType messageType) const
{
	if (this->size() == 0) {
		return QList<qint64>();
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->getAllMessageIDs(messageType);
}

QList<qint64> MessageDbSet::_yrly_getAllMessageIDs(
    enum MessageDb::MessageType messageType) const
{
	QList<qint64> msgIds;

	for (QMap<QString, MessageDb *>::const_iterator i = this->begin();
	     i != this->end(); ++i) {
		MessageDb *db = i.value();
		if (Q_NULLPTR == db) {
			Q_ASSERT(0);
			return QList<qint64>();
		}

		msgIds.append(db->getAllMessageIDs(messageType));
	}

	return msgIds;
}

QList<qint64> MessageDbSet::getAllMessageIDs(
    enum MessageDb::MessageType messageType) const
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_getAllMessageIDs(messageType);
		break;
	case DO_YEARLY:
		return _yrly_getAllMessageIDs(messageType);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return QList<qint64>();
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
	QList<MessageDb::MsgId> msgIds;

	for (QMap<QString, MessageDb *>::const_iterator i = this->begin();
	     i != this->end(); ++i) {
		MessageDb *db = i.value();
		if (Q_UNLIKELY(Q_NULLPTR == db)) {
			Q_ASSERT(0);
			return QList<MessageDb::MsgId>();
		}

		msgIds.append(db->msgsDateInterval(fromDate, toDate,
		    msgDirect));
	}

	return msgIds;
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

QList<MessageDb::SoughtMsg> MessageDbSet::_sf_msgsSearch(
    const Isds::Envelope &envel, enum MessageDirection msgDirect,
    const QString &attachPhrase, bool logicalAnd) const
{
	if (this->size() == 0) {
		return QList<MessageDb::SoughtMsg>();
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsSearch(envel, msgDirect, attachPhrase,
	    logicalAnd);
}

QList<MessageDb::SoughtMsg> MessageDbSet::_yrly_msgsSearch(
    const Isds::Envelope &envel, enum MessageDirection msgDirect,
    const QString &attachPhrase, bool logicalAnd) const
{
	QList<MessageDb::SoughtMsg> msgs;

	for (QMap<QString, MessageDb *>::const_iterator i = this->begin();
	     i != this->end(); ++i) {
		MessageDb *db = i.value();
		if (Q_UNLIKELY(Q_NULLPTR == db)) {
			Q_ASSERT(0);
			return QList<MessageDb::SoughtMsg>();
		}

		msgs.append(db->msgsSearch(envel, msgDirect, attachPhrase,
		    logicalAnd));
	}

	return msgs;
}

QList<MessageDb::SoughtMsg> MessageDbSet::msgsSearch(
    const Isds::Envelope &envel, enum MessageDirection msgDirect,
    const QString &attachPhrase, bool logicalAnd) const
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgsSearch(envel, msgDirect, attachPhrase,
		    logicalAnd);
		break;
	case DO_YEARLY:
		return _yrly_msgsSearch(envel, msgDirect, attachPhrase,
		    logicalAnd);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return QList<MessageDb::SoughtMsg>();
}

MessageDb::SoughtMsg MessageDbSet::_sf_msgsGetMsgDataFromId(
    const qint64 msgId) const
{
	if (this->size() == 0) {
		return MessageDb::SoughtMsg();
	}
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsGetMsgDataFromId(msgId);
}

MessageDb::SoughtMsg MessageDbSet::_yrly_msgsGetMsgDataFromId(
    const qint64 msgId) const
{
	MessageDb::SoughtMsg msgData;

	for (QMap<QString, MessageDb *>::const_iterator i = this->begin();
	     i != this->end(); ++i) {
		MessageDb *db = i.value();
		if (Q_UNLIKELY(Q_NULLPTR == db)) {
			Q_ASSERT(0);
			return MessageDb::SoughtMsg();
		}
		msgData = db->msgsGetMsgDataFromId(msgId);
		if (msgData.mId.dmId != -1) {
			return msgData;
		}
	}

	return MessageDb::SoughtMsg();
}

MessageDb::SoughtMsg MessageDbSet::msgsGetMsgDataFromId(
    const qint64 msgId) const
{
	switch (m_organisation) {
	case DO_SINGLE_FILE:
		return _sf_msgsGetMsgDataFromId(msgId);
		break;
	case DO_YEARLY:
		return _yrly_msgsGetMsgDataFromId(msgId);
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return MessageDb::SoughtMsg();
}
