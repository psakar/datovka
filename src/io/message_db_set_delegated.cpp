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

#include "message_db_set.h"
#include "src/log/log.h"

DbMsgsTblModel *MessageDbSet::_sf_msgsRcvdModel(void)
{
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
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsRcvdWithin90DaysModel();
}

DbMsgsTblModel *MessageDbSet::_yrly_msgsRcvdWithin90DaysModel(void)
{
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
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsRcvdYears(sorting);
}

QStringList MessageDbSet::_yrly_msgsRcvdYears(enum Sorting sorting) const
{
	(void) sorting;
	Q_ASSERT(0);
	return QStringList();
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
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsRcvdUnreadWithin90Days();
}

int MessageDbSet::_yrly_msgsRcvdUnreadWithin90Days(void) const
{
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
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsRcvdUnreadInYear(year);
}

int MessageDbSet::_yrly_msgsRcvdUnreadInYear(const QString &year) const
{
	(void) year;
	Q_ASSERT(0);
	return -1;
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
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsSntYears(sorting);
}

QStringList MessageDbSet::_yrly_msgsSntYears(enum Sorting sorting) const
{
	(void) sorting;
	Q_ASSERT(0);
	return QStringList();
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
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsSntUnreadWithin90Days();
}

int MessageDbSet::_yrly_msgsSntUnreadWithin90Days(void) const
{
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
	Q_ASSERT(this->size() == 1);
	return this->first()->msgsSntUnreadInYear(year);
}

int MessageDbSet::_yrly_msgsSntUnreadInYear(const QString &year) const
{
	(void) year;
	Q_ASSERT(0);
	return -1;
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
