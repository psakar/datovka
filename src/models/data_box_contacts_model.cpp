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

#include "src/common.h"
#include "src/models/data_box_contacts_model.h"

BoxContactsModel::BoxContactsModel(QObject *parent)
    : TblModel(parent)
{
	/* Fixed column count. */
	m_columnCount = MAX_COL;
}

QVariant BoxContactsModel::data(const QModelIndex &index, int role) const
{
	switch (role) {
	case Qt::DisplayRole:
		/* Continue with code. */
		break;
	case Qt::CheckStateRole:
		if (index.column() == CHECKBOX_COL) {
			return _data(index, Qt::DisplayRole).toBool() ?
			    Qt::Checked : Qt::Unchecked;
		} else {
			return QVariant();
		}
		break;
	default:
		return _data(index, role);
		break;
	}

	switch (index.column()) {
	case CHECKBOX_COL:
		return QVariant();
		break;
	case BOX_TYPE_COL:
		{
			QVariant entry(_data(index, role));

			if (!entry.isNull()) {
				return convertDbTypeToString(entry.toInt());
			} else {
				return entry;
			}
		}
		break;
	case PDZ_COL:
		{
			QVariant entry(_data(index, role));

			if (!entry.isNull()) {
				return entry.toBool() ? tr("yes") : tr("no");
			} else {
				return entry;
			}
		}
		break;
	default:
		return _data(index, role);
		break;
	}
}

Qt::ItemFlags BoxContactsModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags defaultFlags = TblModel::flags(index);

	if (index.column() == CHECKBOX_COL) {
		defaultFlags |= Qt::ItemIsUserCheckable;
	}

	return defaultFlags;
}

bool BoxContactsModel::setData(const QModelIndex &index, const QVariant &value,
    int role)
{
	if (!index.isValid()) {
		return false;
	}

	int row = index.row();
	if ((row < 0) || (row >= m_rowCount)) {
		return 0;
	}

	if ((index.column() != CHECKBOX_COL) || (role != Qt::CheckStateRole)) {
		return TblModel::setData(index, value, role);
	}

	bool newVal = (value == Qt::Checked);
	if (newVal != _data(index, Qt::DisplayRole).toBool()) {
		m_data[row][CHECKBOX_COL] = newVal;
		emit dataChanged(index, index);
		return true;
	}

	return false;
}

void BoxContactsModel::setHeader(void)
{
	setHeaderData(BOX_ID_COL, Qt::Horizontal, tr("ID"), Qt::DisplayRole);
	setHeaderData(BOX_TYPE_COL, Qt::Horizontal, tr("Type"),
	    Qt::DisplayRole);
	setHeaderData(BOX_NAME_COL, Qt::Horizontal, tr("Name"),
	    Qt::DisplayRole);
	setHeaderData(ADDRESS_COL, Qt::Horizontal, tr("Address"),
	    Qt::DisplayRole);
	setHeaderData(POST_CODE_COL, Qt::Horizontal, tr("Postal Code"),
	    Qt::DisplayRole);
	setHeaderData(PDZ_COL, Qt::Horizontal, tr("PDZ"), Qt::DisplayRole);
}

void BoxContactsModel::appendData(
    const QList<TaskSearchOwner::BoxEntry> &entryList)
{
	if (entryList.isEmpty()) {
		return;
	}

	beginInsertRows(QModelIndex(), rowCount(),
	    rowCount() + entryList.size() - 1);

	foreach (const TaskSearchOwner::BoxEntry &entry, entryList) {

		reserveSpace();

		QVector<QVariant> row(m_columnCount);

		row[CHECKBOX_COL] = false;
		row[BOX_ID_COL] = entry.id;
		row[BOX_TYPE_COL] = entry.type;
		row[BOX_NAME_COL] = entry.name;
		row[ADDRESS_COL] = entry.address;
		row[POST_CODE_COL] = entry.zipCode;
		row[PDZ_COL] = !entry.effectiveOVM;

		m_data[m_rowCount++] = row;
	}

	endInsertRows();
}

void BoxContactsModel::appendData(
    const QList<TaskSearchOwnerFulltext::BoxEntry> &entryList)
{
	if (entryList.isEmpty()) {
		return;
	}

	beginInsertRows(QModelIndex(), rowCount(),
	    rowCount() + entryList.size() - 1);

	foreach (const TaskSearchOwnerFulltext::BoxEntry &entry, entryList) {

		reserveSpace();

		QVector<QVariant> row(m_columnCount);

		row[CHECKBOX_COL] = false;
		row[BOX_ID_COL] = entry.id;
		row[BOX_TYPE_COL] = entry.type;
		row[BOX_NAME_COL] = entry.name;
		row[ADDRESS_COL] = entry.address;
		//row[POST_CODE_COL];
		row[PDZ_COL] = !entry.publicSending && entry.commercialSending;

		m_data[m_rowCount++] = row;
	}

	endInsertRows();
}

void BoxContactsModel::appendData(
    const QList<MessageDb::ContactEntry> &entryList)
{
	if (entryList.isEmpty()) {
		return;
	}

	beginInsertRows(QModelIndex(), rowCount(),
	    rowCount() + entryList.size() - 1);

	foreach (const MessageDb::ContactEntry &entry, entryList) {

		reserveSpace();

		QVector<QVariant> row(m_columnCount);

		row[CHECKBOX_COL] = false;
		row[BOX_ID_COL] = entry.boxId;
		//row[BOX_TYPE_COL];
		row[BOX_NAME_COL] = entry.name;
		row[ADDRESS_COL] = entry.address;
		//row[POST_CODE_COL];
		//row[PDZ_COL];

		m_data[m_rowCount++] = row;
	}

	endInsertRows();
}

bool BoxContactsModel::somethingChecked(void) const
{
	for (int row = 0; row < m_rowCount; ++row) {
		if (m_data[row][CHECKBOX_COL].toBool()) {
			return true;
		}
	}

	return false;
}

/*!
 * @brief Compares the check state according to required criteria.
 *
 * @param[in] checked Check state from the model.
 * @param[in] entryState User required check state.
 * @return True if state matches the criteria.
 */
static inline
bool checkStateMatch(bool checked, enum BoxContactsModel::EntryState entryState)
{
	return (!checked && (entryState == BoxContactsModel::UNCHECKED)) ||
	    (checked && (entryState == BoxContactsModel::CHECKED)) ||
	    (entryState == BoxContactsModel::ANY);
}

QStringList BoxContactsModel::boxIdentifiers(enum EntryState entryState) const
{
	QStringList ids;

	for (int row = 0; row < m_rowCount; ++row) {
		if (checkStateMatch(m_data[row][CHECKBOX_COL].toBool(),
		        entryState)) {
			ids.append(m_data[row][BOX_ID_COL].toString());
		}
	}

	return ids;
}
