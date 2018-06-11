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

#include "src/datovka_shared/isds/type_conversion.h"
#include "src/isds/to_text_conversion.h"
#include "src/isds/type_description.h"
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
		switch (index.column()) {
		case CHECKBOX_COL:
			return QVariant();
			break;
		case BOX_TYPE_COL:
			{
				QVariant entry(_data(index, role));
				if (!entry.isNull()) {
					return Isds::dbType2Str(Isds::intVariant2DbType(entry));
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
		break;
	case Qt::ToolTipRole:
		if (index.column() == BOX_TYPE_COL) {
			QVariant entry(_data(index, Qt::DisplayRole));

			if (!entry.isNull()) {
				return Isds::Description::descrDbType(Isds::intVariant2DbType(entry));
			} else {
				return entry;
			}
		} else {
			return QVariant();
		}
		break;
	case Qt::CheckStateRole:
		if (index.column() == CHECKBOX_COL) {
			return _data(index, Qt::DisplayRole).toBool() ?
			    Qt::Checked : Qt::Unchecked;
		} else {
			return QVariant();
		}
		break;
	case Qt::AccessibleTextRole:
		/*
		 * Most of the accessibility data are constructed using
		 * the header data followed by the cell content.
		 */
		switch (index.column()) {
		case CHECKBOX_COL:
			return _data(index, Qt::DisplayRole).toBool() ?
			    tr("selected") : tr("not selected");
			break;
		case BOX_ID_COL:
			return tr("box identifier") + QLatin1String(" ") +
			    data(index).toString();
			break;
		case BOX_TYPE_COL:
			return headerData(index.column(), Qt::Horizontal).toString() +
			    QLatin1String(" ") +
			    data(index, Qt::ToolTipRole).toString();
			break;
		case BOX_NAME_COL:
		case ADDRESS_COL:
		case POST_CODE_COL:
		case PDZ_COL:
			return headerData(index.column(), Qt::Horizontal).toString() +
			    QLatin1String(" ") +
			    data(index).toString();
			break;
		default:
			return QVariant();
			break;
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
    const QList<Isds::DbOwnerInfo> &entryList)
{
	if (entryList.isEmpty()) {
		return;
	}

	beginInsertRows(QModelIndex(), rowCount(),
	    rowCount() + entryList.size() - 1);

	foreach (const Isds::DbOwnerInfo &entry, entryList) {

		reserveSpace();

		QVector<QVariant> row(m_columnCount);

		row[CHECKBOX_COL] = false;
		row[BOX_ID_COL] = entry.dbID();
		row[BOX_TYPE_COL] = Isds::dbType2IntVariant(entry.dbType());
		row[BOX_NAME_COL] = Isds::textOwnerName(entry);
		row[ADDRESS_COL] = Isds::textAddressWithoutIc(entry.address());
		row[POST_CODE_COL] = entry.address().zipCode();
		row[PDZ_COL] = (entry.dbEffectiveOVM() != Isds::Type::BOOL_TRUE);

		m_data[m_rowCount++] = row;
	}

	endInsertRows();
}

void BoxContactsModel::appendData(const QList<Isds::FulltextResult> &entryList)
{
	if (entryList.isEmpty()) {
		return;
	}

	beginInsertRows(QModelIndex(), rowCount(),
	    rowCount() + entryList.size() - 1);

	foreach (const Isds::FulltextResult &entry, entryList) {

		reserveSpace();

		QVector<QVariant> row(m_columnCount);

		row[CHECKBOX_COL] = false;
		row[BOX_ID_COL] = entry.dbID();
		row[BOX_TYPE_COL] = Isds::dbType2IntVariant(entry.dbType());
		row[BOX_NAME_COL] = entry.dbName();
		row[ADDRESS_COL] = entry.dbAddress();
		//row[POST_CODE_COL];
		row[PDZ_COL] = !entry.publicSending() && entry.commercialSending();

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

void BoxContactsModel::appendData(const QString &id,
    enum Isds::Type::DbType type, const QString &name, const QString &addr,
    const QString &postCode, const QVariant &pdz)
{
	if (id.isEmpty() || name.isEmpty() || addr.isEmpty()) {
		return;
	}

	beginInsertRows(QModelIndex(), rowCount(), rowCount());

	{
		reserveSpace();

		QVector<QVariant> row(m_columnCount);

		row[CHECKBOX_COL] = false;
		row[BOX_ID_COL] = id;
		if (type != Isds::Type::BT_NULL) {
			row[BOX_TYPE_COL] = Isds::dbType2IntVariant(type);
		}
		row[BOX_NAME_COL] = name;
		row[ADDRESS_COL] = addr;
		row[POST_CODE_COL] = postCode;
		row[PDZ_COL] = pdz;

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

bool BoxContactsModel::containsBoxId(const QString &boxId) const
{
	for (int row = 0; row < m_rowCount; ++row) {
		if (m_data[row][BOX_ID_COL].toString() == boxId) {
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

QList<BoxContactsModel::PartialEntry> BoxContactsModel::partialBoxEntries(
    enum EntryState entryState) const
{
	QList<PartialEntry> entries;

	for (int row = 0; row < m_rowCount; ++row) {
		if (checkStateMatch(m_data[row][CHECKBOX_COL].toBool(),
		        entryState)) {
			PartialEntry entry;

			entry.id = m_data[row][BOX_ID_COL].toString();
			entry.name = m_data[row][BOX_NAME_COL].toString();
			entry.address = m_data[row][ADDRESS_COL].toString();
			entry.pdz = m_data[row][PDZ_COL].toBool();

			entries.append(entry);
		}
	}

	return entries;
}
