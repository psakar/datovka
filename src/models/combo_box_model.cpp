/*
 * Copyright (C) 2014-2017 CZ.NIC
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

#include "src/models/combo_box_model.h"

const int CBoxModel::valueRole = Qt::UserRole;

CBoxModel::CBoxModel(QObject *parent)
    : QAbstractListModel(parent),
    m_entries()
{
}

int CBoxModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid()) {
		return 0;
	} else {
		return m_entries.size();
	}
}

QVariant CBoxModel::data(const QModelIndex &index, int role) const
{
	int row = index.row();
	if ((row < 0) || (row >= m_entries.size())) {
		return QVariant();
	}

	switch (role) {
	case Qt::DisplayRole:
	case Qt::AccessibleTextRole:
		return m_entries.at(row).label;
		break;
	case valueRole:
		return m_entries.at(row).value;
		break;
	default:
		return QVariant();
		break;
	}
}

Qt::ItemFlags CBoxModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);

	if (index.isValid()) {
		int row = index.row();
		if ((row >= 0) && (row < m_entries.size()) &&
		    !m_entries.at(row).enabled) {
			defaultFlags &= ~Qt::ItemIsEnabled;
		}
	}

	return defaultFlags;
}

void CBoxModel::appendRow(const QString &label, int val, bool enabled)
{
	beginInsertRows(QModelIndex(), rowCount(), rowCount());
	m_entries.append(Entry(label, val, enabled));
	endInsertRows();
}

void CBoxModel::setEnabled(int val, bool enabled)
{
	for (int row = 0; row < m_entries.size(); ++row) {
		Entry &entry(m_entries[row]);
		if ((entry.value == val) && (entry.enabled != enabled)) {
			entry.enabled = enabled;

			emit dataChanged(QAbstractListModel::index(row, 0),
			    QAbstractListModel::index(row, 0));
		}
	}
}

int CBoxModel::findRow(int val) const
{
	for (int row = 0; row < m_entries.size(); ++row) {
		const Entry &entry(m_entries.at(row));
		if (entry.enabled && (entry.value == val)) {
			return row;
		}
	}

	return -1;
}
