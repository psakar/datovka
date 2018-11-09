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

#include "src/gov_services/models/gov_service_list_model.h"
#include "src/datovka_shared/gov_services/service/gov_service.h"

GovServiceListModel::Entry::Entry(const Entry &sme)
    : m_srvcInternId(sme.m_srvcInternId),
    m_srvcFullName(sme.m_srvcFullName),
    m_instName(sme.m_instName),
    m_srvcBoxId(sme.m_srvcBoxId)
{
}

GovServiceListModel::Entry::Entry(const QString &srvcInternId,
    const QString &srvcFullName, const QString &instName,
    const QString &srvcBoxId)
    : m_srvcInternId(srvcInternId),
    m_srvcFullName(srvcFullName),
    m_instName(instName),
    m_srvcBoxId(srvcBoxId)
{
}

const QString &GovServiceListModel::Entry::srvcInternId(void) const
{
	return m_srvcInternId;
}

const QString &GovServiceListModel::Entry::srvcFullName(void) const
{
	return m_srvcFullName;
}

const QString &GovServiceListModel::Entry::instName(void) const
{
	return m_instName;
}

const QString &GovServiceListModel::Entry::srvcBoxId(void) const
{
	return m_srvcBoxId;
}

GovServiceListModel::GovServiceListModel(QObject *parent)
    : QAbstractListModel(parent),
    m_services()
{
}

int GovServiceListModel::rowCount(const QModelIndex &parent) const
{
	return !parent.isValid() ? m_services.size() : 0;
}

QVariant GovServiceListModel::data(const QModelIndex &index, int role) const
{
	int row = index.row();
	if (Q_UNLIKELY((row < 0) || (row >= m_services.size()))) {
		Q_ASSERT(0);
		return QVariant();
	}

	const Entry &e(m_services[row]);

	switch (role) {
	case Qt::DisplayRole:
	case Qt::AccessibleTextRole:
		return QString("%1\nDS: %2 -- %3").arg(e.srvcFullName()).arg(e.srvcBoxId()).arg(e.instName());
		break;
	case ROLE_INTERN_ID:
		return e.srvcInternId();
		break;
	default:
		/* Do nothing. */
		break;
	}

	return QVariant();
}

Qt::ItemFlags GovServiceListModel::flags(const QModelIndex &index) const
{
	return QAbstractListModel::flags(index);
}

void GovServiceListModel::appendService(const Gov::Service *gs)
{
	if (Q_UNLIKELY(gs == Q_NULLPTR)) {
		Q_ASSERT(0);
		return;
	}

	beginInsertRows(QModelIndex(), rowCount(), rowCount());
	m_services.append(Entry(gs->internalId(), gs->fullName(),
	    gs->instituteName(), gs->boxId()));
	endInsertRows();
}

void GovServiceListModel::clearAll(void)
{
	beginResetModel();
	m_services.clear();
	endResetModel();
}
