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

#include "src/delegates/tag_item.h"
#include "src/models/sort_filter_proxy_model.h"

SortFilterProxyModel::SortFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

void SortFilterProxyModel::setFilterKeyColumn(int column)
{
	m_filterColumns.clear();
	m_filterColumns.append(column);
}

void SortFilterProxyModel::setFilterKeyColumns(const QList<int> &columns)
{
	m_filterColumns = columns;
}

QList<int> SortFilterProxyModel::filterKeyColumns(void)
{
	return m_filterColumns;
}

bool SortFilterProxyModel::filterAcceptsRow(int sourceRow,
    const QModelIndex &sourceParent) const
{
	/*
	 * Adapted from
	 * qtbase/src/corelib/itemmodels/qsortfilterproxymodel.cpp .
	 */

	if (filterRegExp().isEmpty()) {
		return true;
	}

	if (m_filterColumns.isEmpty() || m_filterColumns.contains(-1)) {
		int columnCnt = columnCount();
		for (int column = 0; column < columnCnt; ++column) {
			QModelIndex sourceIndex(sourceModel()->index(sourceRow,
			    column, sourceParent));
			if (filterAcceptsItem(sourceIndex)) {
				return true;
			}
		}
		return false;
	}

	foreach (int column, m_filterColumns) {
		QModelIndex sourceIndex(
		    sourceModel()->index(sourceRow, column, sourceParent));
		/* The column may not exist. */
		if (filterAcceptsItem(sourceIndex)) {
			return true;
		}
	}
	return false;
}

bool SortFilterProxyModel::filterAcceptsItem(const QModelIndex &sourceIdx) const
{
	if (!sourceIdx.isValid()) {
		return false;
	}

	const QVariant data(sourceModel()->data(sourceIdx, filterRole()));

	if (data.canConvert<TagItem>()) {
		TagItem tagItem = qvariant_cast<TagItem>(data);
		return tagItem.name.contains(filterRegExp());
	} else if (data.canConvert<TagItemList>()) {
		TagItemList tagList = qvariant_cast<TagItemList>(data);
		foreach (const TagItem &tagItem, tagList) {
			if (tagItem.name.contains(filterRegExp())) {
				return true;
			}
		}
		return false;
	} else {
		QString key(data.toString());
		return key.contains(filterRegExp());
	}
}
