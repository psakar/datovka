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

#include <QSqlRecord>

#include "src/models/table_model.h"

const int TblModel::m_rowAllocationIncrement(128);

TblModel::TblModel(QObject *parent)
    : QAbstractTableModel(parent),
    m_data(),
    m_rowsAllocated(0),
    m_rowCount(0),
    m_columnCount(0),
    m_headerData()
{
}

int TblModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid()) {
		return 0;
	} else {
		return m_rowCount;
	}
}

int TblModel::columnCount(const QModelIndex &parent) const
{
	if (parent.isValid()) {
		return 0;
	} else {
		return m_columnCount;
	}
}

bool TblModel::setHeaderData(int section, Qt::Orientation orientation,
    const QVariant &value, int role)
{
	/* Orientation is ignored. */

	m_headerData[section][role] = value;

	emit headerDataChanged(orientation, section, section);

	return true;
}

QVariant TblModel::headerData(int section, Qt::Orientation orientation,
    int role) const
{
	return _headerData(section, orientation, role);
}

void TblModel::setQuery(QSqlQuery &query)
{
	m_data.clear();
	m_rowsAllocated = 0;
	m_rowCount = 0;

	/* Looks like empty results have column count set. */
	m_columnCount = query.record().count();

	appendQueryData(query);
}

bool TblModel::appendQueryData(QSqlQuery &query)
{
	if (query.record().count() != m_columnCount) {
		return false;
	}

	beginResetModel();

	query.first();
	while (query.isActive() && query.isValid()) {

		if (m_rowCount == m_rowsAllocated) {
			m_rowsAllocated += m_rowAllocationIncrement;
			m_data.resize(m_rowsAllocated);
		}

		QVector<QVariant> row(m_columnCount);

		for (int i = 0; i < m_columnCount; ++i) {
			row[i] = query.value(i);
		}

		m_data[m_rowCount++] = row;

		query.next();
	}

	endResetModel();

	return true;
}

QVariant TblModel::_data(const QModelIndex &index, int role) const
{
	if (role != Qt::DisplayRole) {
		return QVariant();
	}

	if ((index.row() < m_rowCount) && (index.column() < m_columnCount)) {
		return m_data.at(index.row()).at(index.column());
	} else {
		return QVariant();
	}
}

QVariant TblModel::_data(int row, int col, int role) const
{
	if (role != Qt::DisplayRole) {
		return QVariant();
	}

	if ((row < m_rowCount) && (col < m_columnCount)) {
		return m_data.at(row).at(col);
	} else {
		return QVariant();
	}
}

QVariant TblModel::_headerData(int section, Qt::Orientation orientation,
    int role) const
{
	Q_UNUSED(orientation);

	return m_headerData[section][role];
}
