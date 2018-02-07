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

#include "src/localisation/localisation.h"
#include "src/records_management/models/upload_hierarchy_proxy_model.h"

UploadHierarchyProxyModel::UploadHierarchyProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

bool UploadHierarchyProxyModel::filterAcceptsRow(int sourceRow,
    const QModelIndex &sourceParent) const
{
	/*
	 * Adapted from
	 * qtbase/src/corelib/itemmodels/qsortfilterproxymodel.cpp .
	 */

	if (filterRegExp().isEmpty()) {
		return true;
	}

	QModelIndex sourceIndex(sourceModel()->index(sourceRow,
	    filterKeyColumn(), sourceParent));
	return filterAcceptsItem(sourceIndex);
}

bool UploadHierarchyProxyModel::lessThan(const QModelIndex &sourceLeft,
    const QModelIndex &sourceRight) const
{
	QVariant leftData(sourceModel()->data(sourceLeft, sortRole()));
	QVariant rightData(sourceModel()->data(sourceRight, sortRole()));

	if (Q_LIKELY(leftData.canConvert<QString>())) {
		Q_ASSERT(rightData.canConvert<QString>());
		return Localisation::stringCollator.compare(leftData.toString(),
		    rightData.toString()) < 0;
	} else {
		return QSortFilterProxyModel::lessThan(sourceLeft, sourceRight);
	}
}

bool UploadHierarchyProxyModel::filterAcceptsItem(
    const QModelIndex &sourceIdx) const
{
	if (!sourceIdx.isValid()) {
		return false;
	}

	QStringList filterData;
	{
		const QVariant data(
		    sourceModel()->data(sourceIdx, filterRole()));
		if (data.canConvert<QString>()) {
			filterData += data.toString();
		} else if (data.canConvert<QStringList>()) {
			filterData += data.toStringList();
		} else {
			return false;
		}
	}

	foreach (const QString &str, filterData) {
		if (str.contains(filterRegExp())) {
			return true;
		}
	}
	return false;
}
