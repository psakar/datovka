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

#include "src/models/tags_model.h"

TagsModel::TagsModel(QObject *parent)
    : QAbstractListModel(parent),
    m_tagList()
{
}

int TagsModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid()) {
		return 0;
	} else {
		return m_tagList.size();
	}
}

QVariant TagsModel::data(const QModelIndex &index, int role) const
{
	if (role != Qt::DisplayRole) {
		return QVariant();
	}

	if ((index.row() < m_tagList.size()) && (index.column() == 0)) {
		return QVariant::fromValue(m_tagList.at(index.row()));
	} else {
		return QVariant();
	}
}

QVariant TagsModel::headerData(int section, Qt::Orientation orientation,
    int role) const
{
	Q_UNUSED(orientation);

	if (role != Qt::DisplayRole) {
		return QVariant();
	}

	if (0 == section) {
		return tr("Tags");
	} else {
		return QVariant();
	}
}
