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


#include "src/gui/dlg_contacts.h"
#include "src/views/table_home_end_filter.h"
#include "src/views/table_space_selection_filter.h"


DlgContacts::DlgContacts(const MessageDbSet &dbSet, const QString &dbId,
    QStringList *dbIdList, QWidget *parent)
    : QDialog(parent),
    m_dbSet(dbSet),
    m_dbId(dbId),
    m_contactListProxyModel(this),
    m_contactTableModel(this),
    m_dbIdList(dbIdList)
{
	setupUi(this);
	/* Set default line height for table views/widgets. */
	this->contactTableView->setNarrowedLineHeight();
	this->contactTableView->setSelectionBehavior(
	    QAbstractItemView::SelectRows);

	m_contactTableModel.setHeader();
	m_contactListProxyModel.setSortRole(ROLE_MSGS_DB_PROXYSORT);
	m_contactListProxyModel.setSourceModel(&m_contactTableModel);
	{
		QList<int> columnList;
		columnList.append(BoxContactsModel::BOX_ID_COL);
		columnList.append(BoxContactsModel::BOX_NAME_COL);
		columnList.append(BoxContactsModel::ADDRESS_COL);
		m_contactListProxyModel.setFilterKeyColumns(columnList);
	}
	this->contactTableView->setModel(&m_contactListProxyModel);

	this->contactTableView->setColumnWidth(BoxContactsModel::CHECKBOX_COL, 20);
	this->contactTableView->setColumnWidth(BoxContactsModel::BOX_ID_COL, 70);
	this->contactTableView->setColumnWidth(BoxContactsModel::BOX_NAME_COL, 150);

	this->contactTableView->setColumnHidden(BoxContactsModel::BOX_TYPE_COL,
	    true);
	this->contactTableView->setColumnHidden(BoxContactsModel::POST_CODE_COL,
	    true);
	this->contactTableView->setColumnHidden(BoxContactsModel::PDZ_COL,
	    true);

	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	this->clearPushButton->setEnabled(false);

	fillContactsFromMessageDb();

	connect(this->filterLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(filterContact(QString)));
	connect(&m_contactTableModel,
	    SIGNAL(dataChanged(QModelIndex, QModelIndex)),
	    this, SLOT(enableOkButton()));
	connect(this->contactTableView->selectionModel(),
	    SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
	    this, SLOT(setFirstColumnActive(QItemSelection, QItemSelection)));
	connect(this->contactTableView, SIGNAL(doubleClicked(QModelIndex)),
	    this, SLOT(contactItemDoubleClicked(QModelIndex)));
	connect(this->clearPushButton, SIGNAL(clicked()),
	    this, SLOT(clearContactText()));
	connect(this->buttonBox, SIGNAL(accepted()),
	    this, SLOT(addSelectedDbIDs()));

	this->contactTableView->
	    setEditTriggers(QAbstractItemView::NoEditTriggers);

	this->contactTableView->installEventFilter(
	    new TableHomeEndFilter(this));
	this->contactTableView->installEventFilter(
	    new TableSpaceSelectionFilter(this));
}

void DlgContacts::enableOkButton(void)
{
	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
	    m_contactTableModel.somethingChecked());
}

void DlgContacts::setFirstColumnActive(const QItemSelection &selected,
    const QItemSelection &deselected)
{
	Q_UNUSED(deselected);

	if (selected.isEmpty()) {
		return;
	}
	this->contactTableView->selectColumn(BoxContactsModel::CHECKBOX_COL);
	this->contactTableView->selectRow(selected.first().top());
}

void DlgContacts::contactItemDoubleClicked(const QModelIndex &index)
{
	if (index.isValid() && (m_dbIdList != Q_NULLPTR)) {
		m_dbIdList->append(index.sibling(index.row(),
		    BoxContactsModel::BOX_ID_COL).data(
		        Qt::DisplayRole).toString());
	}
	this->close();
}

void DlgContacts::addSelectedDbIDs(void) const
{
	if (m_dbIdList != Q_NULLPTR) {
		m_dbIdList->append(m_contactTableModel.checkedBoxIds());
	}
}

void DlgContacts::filterContact(const QString &text)
{
	this->clearPushButton->setEnabled(!text.isEmpty());

	m_contactListProxyModel.setFilterRegExp(QRegExp(text,
	    Qt::CaseInsensitive, QRegExp::FixedString));
	/* Set filter field background colour. */
	if (text.isEmpty()) {
		this->filterLineEdit->setStyleSheet(
		    SortFilterProxyModel::blankFilterEditStyle);
	} else if (m_contactListProxyModel.rowCount() != 0) {
		this->filterLineEdit->setStyleSheet(
		    SortFilterProxyModel::foundFilterEditStyle);
	} else {
		this->filterLineEdit->setStyleSheet(
		    SortFilterProxyModel::notFoundFilterEditStyle);
	}
}

void DlgContacts::clearContactText(void)
{
	this->filterLineEdit->clear();
	this->clearPushButton->setEnabled(false);
}

void DlgContacts::fillContactsFromMessageDb(void)
{
	this->contactTableView->setEnabled(false);
	m_contactTableModel.removeRows(0, m_contactTableModel.rowCount());

	QList<MessageDb::ContactEntry> foundBoxes(m_dbSet.uniqueContacts());

	this->contactTableView->setEnabled(true);
	m_contactTableModel.appendData(foundBoxes);

	if (m_contactTableModel.rowCount() > 0) {
		this->contactTableView->selectColumn(
		    BoxContactsModel::CHECKBOX_COL);
		this->contactTableView->selectRow(0);
	}

	//this->contactTableView->resizeColumnsToContents();
}
