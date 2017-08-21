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

Qt::ItemFlags TagsModel::flags(const QModelIndex &index) const
{
	if (Q_UNLIKELY(!index.isValid())) {
		return Qt::NoItemFlags;
	}

	Qt::ItemFlags defaultFlags = QAbstractListModel::flags(index);

	if (!m_enabled.at(index.row())) {
		defaultFlags &= ~Qt::ItemIsSelectable;
		defaultFlags &= ~Qt::ItemIsEnabled;
	}

	return defaultFlags;
}

void TagsModel::setTagList(const TagItemList &tagList)
{
	beginResetModel();

	m_tagList = tagList;
	m_enabled = QVector<bool>(tagList.size(), true);

	endResetModel();
}

void  TagsModel::setDisabledTagList(const TagItemList &disabledTagList)
{
	foreach (const TagItem &disabledItem, disabledTagList) {
		int row = m_tagList.indexOf(disabledItem);
		if (row == -1) {
			continue;
		}
		m_enabled[row] = false;
		emit dataChanged(QAbstractListModel::index(row, 0),
		    QAbstractListModel::index(row, 0));
	}
}
