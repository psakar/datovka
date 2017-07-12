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

#include "src/records_management/models/upload_hierarchy_model.h"

UploadHierarchyModel::UploadHierarchyModel(QObject *parent)
    : QAbstractItemModel(parent),
    m_hierarchy()
{
}

QModelIndex UploadHierarchyModel::index(int row, int column,
    const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent)) {
		return QModelIndex();
	}

	quintptr internalId = 0;

	if (!parent.isValid()) {
		if (showRootName()) {
			/* Root is shown. */
			internalId = (quintptr)m_hierarchy.root();
		} else {
			/* Root is not shown. */
			internalId = (quintptr)m_hierarchy.root()->sub().at(row);
		}
	} else {
		const UploadHierarchyResp::NodeEntry *entry =
		    (UploadHierarchyResp::NodeEntry *)parent.internalId();
		internalId = (quintptr)entry->sub().at(row);
	}

	return createIndex(row, column, internalId);
}

QModelIndex UploadHierarchyModel::parent(const QModelIndex &index) const
{
	if (!index.isValid()) {
		return QModelIndex();
	}

	const UploadHierarchyResp::NodeEntry *iEntry =
	    (UploadHierarchyResp::NodeEntry *)index.internalId();
	const UploadHierarchyResp::NodeEntry *pEntry = iEntry->super();

	if ((pEntry != Q_NULLPTR) &&
	    ((pEntry != m_hierarchy.root()) || showRootName())) {
		if (pEntry == m_hierarchy.root()) {
			/* Root node is shown and parent is root. */
			return createIndex(0, 0, (quintptr)pEntry);
		} else {
			const UploadHierarchyResp::NodeEntry *ppEntry =
			    pEntry->super();
			int row = 0;
			/* Find position of parent. */
			for ( ; row < ppEntry->sub().size(); ++row) {
				if (pEntry == ppEntry->sub().at(row)) {
					break;
				}
			}
			Q_ASSERT(row < ppEntry->sub().size());
			return createIndex(row, 0, (quintptr)pEntry);
		}
	} else {
		return QModelIndex();
	}
}

int UploadHierarchyModel::rowCount(const QModelIndex &parent) const
{
	if (parent.column() > 0) {
		return 0;
	}

	if (!parent.isValid()) {
		if (!m_hierarchy.isValid()) {
			/* Invalid hierarchy. */
			return 0;
		}

		/* Root. */
		if (showRootName()) {
			return 1;
		} else {
			return m_hierarchy.root()->sub().size();
		}
	} else {
		const UploadHierarchyResp::NodeEntry *entry =
		    (UploadHierarchyResp::NodeEntry *)parent.internalId();
		return entry->sub().size();
	}
}

int UploadHierarchyModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);

	return 1;
}

QVariant UploadHierarchyModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid()) {
		return QVariant();
	}

	const UploadHierarchyResp::NodeEntry *entry =
	    (UploadHierarchyResp::NodeEntry *)index.internalId();
	if (entry == Q_NULLPTR) {
		Q_ASSERT(0);
		return QVariant();
	}

	switch (role) {
	case Qt::DisplayRole:
		return entry->name();
		break;
	case Qt::ToolTipRole:
		return filterData(entry).join(QStringLiteral("\n"));
		break;
	case ROLE_FILTER:
		return filterDataRecursive(entry, true);
		break;
	case ROLE_ID:
		return entry->id();
		break;
	default:
		return QVariant();
		break;
	}
}

QVariant UploadHierarchyModel::headerData(int section,
    Qt::Orientation orientation, int role) const
{
	if ((Qt::Horizontal == orientation) && (Qt::DisplayRole == role) &&
	    (0 == section)) {
		return tr("Records Management Hierarchy");
	}

	return QVariant();
}

Qt::ItemFlags UploadHierarchyModel::flags(const QModelIndex &index) const
{
	if (!index.isValid()) {
		return Qt::NoItemFlags;
	}

	Qt::ItemFlags flags =
	    QAbstractItemModel::flags(index) & ~Qt::ItemIsEditable;

	const UploadHierarchyResp::NodeEntry *entry =
	    (UploadHierarchyResp::NodeEntry *)index.internalId();
	if (entry->id().isEmpty()) {
		flags &= ~Qt::ItemIsSelectable;
	}

	return flags;
}

void UploadHierarchyModel::setHierarchy(const UploadHierarchyResp &uhr)
{
	beginResetModel();
	m_hierarchy = uhr;
	endResetModel();
}

bool UploadHierarchyModel::showRootName(void) const
{
	return m_hierarchy.isValid() && !m_hierarchy.root()->name().isEmpty();
}

QStringList UploadHierarchyModel::filterData(
    const UploadHierarchyResp::NodeEntry *entry)
{
	if (Q_UNLIKELY(entry == Q_NULLPTR)) {
		Q_ASSERT(0);
		return QStringList();
	}

	return QStringList(entry->name()) + entry->metadata();
}

QStringList UploadHierarchyModel::filterDataRecursive(
    const UploadHierarchyResp::NodeEntry *entry, bool takeSuper)
{
	if (Q_UNLIKELY(entry == Q_NULLPTR)) {
		Q_ASSERT(0);
		return QStringList();
	}

	QStringList res(filterData(entry));
	foreach (const UploadHierarchyResp::NodeEntry *sub, entry->sub()) {
		res += filterDataRecursive(sub, false);
	}
	if (takeSuper) {
		/*
		 * Add also filter data from superordinate node. This has the
		 * effect that all sub-nodes (including those not matching the
		 * filter) of a node which matches the entered filter are
		 * going to be also displayed.
		 */
		const UploadHierarchyResp::NodeEntry *sup = entry->super();
		if (Q_UNLIKELY(sup == Q_NULLPTR)) {
			return res;
		}
		res += filterData(sup);
	}

	return res;
}
