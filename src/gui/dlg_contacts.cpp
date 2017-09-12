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

#include <QPushButton>

#include "src/gui/dlg_contacts.h"
#include "src/views/table_home_end_filter.h"
#include "src/views/table_space_selection_filter.h"
#include "ui_dlg_contacts.h"


DlgContacts::DlgContacts(const MessageDbSet &dbSet, const QString &dbId,
    QStringList *dbIdList, QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgContacts),
    m_dbId(dbId),
    m_contactListProxyModel(this),
    m_contactTableModel(this),
    m_dbIdList(dbIdList)
{
	m_ui->setupUi(this);
	/* Set default line height for table views/widgets. */
	m_ui->contactTableView->setNarrowedLineHeight();
	m_ui->contactTableView->setSelectionBehavior(
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
	m_ui->contactTableView->setModel(&m_contactListProxyModel);

	m_ui->contactTableView->setColumnWidth(BoxContactsModel::CHECKBOX_COL, 20);
	m_ui->contactTableView->setColumnWidth(BoxContactsModel::BOX_ID_COL, 70);
	m_ui->contactTableView->setColumnWidth(BoxContactsModel::BOX_NAME_COL, 150);

	m_ui->contactTableView->setColumnHidden(BoxContactsModel::BOX_TYPE_COL,
	    true);
	m_ui->contactTableView->setColumnHidden(BoxContactsModel::POST_CODE_COL,
	    true);
	m_ui->contactTableView->setColumnHidden(BoxContactsModel::PDZ_COL,
	    true);

	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

	fillContactsFromMessageDb(dbSet);

	m_ui->filterLine->setToolTip(tr("Enter sought expression"));
	m_ui->filterLine->setClearButtonEnabled(true);

	connect(m_ui->filterLine, SIGNAL(textChanged(QString)),
	    this, SLOT(filterContact(QString)));
	connect(&m_contactTableModel,
	    SIGNAL(dataChanged(QModelIndex, QModelIndex)),
	    this, SLOT(enableOkButton()));
	connect(m_ui->contactTableView->selectionModel(),
	    SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
	    this, SLOT(setFirstColumnActive(QItemSelection, QItemSelection)));
	connect(m_ui->contactTableView, SIGNAL(doubleClicked(QModelIndex)),
	    this, SLOT(contactItemDoubleClicked(QModelIndex)));
	connect(m_ui->buttonBox, SIGNAL(accepted()),
	    this, SLOT(addSelectedDbIDs()));

	m_ui->contactTableView->setEditTriggers(
	    QAbstractItemView::NoEditTriggers);

	m_ui->contactTableView->installEventFilter(
	    new TableHomeEndFilter(this));
	m_ui->contactTableView->installEventFilter(
	    new TableSpaceSelectionFilter(this));
}

DlgContacts::~DlgContacts(void)
{
	delete m_ui;
}

void DlgContacts::enableOkButton(void)
{
	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
	    m_contactTableModel.somethingChecked());
}

void DlgContacts::setFirstColumnActive(const QItemSelection &selected,
    const QItemSelection &deselected)
{
	Q_UNUSED(deselected);

	if (selected.isEmpty()) {
		return;
	}
	m_ui->contactTableView->selectColumn(BoxContactsModel::CHECKBOX_COL);
	m_ui->contactTableView->selectRow(selected.first().top());
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
		m_dbIdList->append(m_contactTableModel.boxIdentifiers(
		    BoxContactsModel::CHECKED));
	}
}

void DlgContacts::filterContact(const QString &text)
{
	m_contactListProxyModel.setFilterRegExp(QRegExp(text,
	    Qt::CaseInsensitive, QRegExp::FixedString));
	/* Set filter field background colour. */
	if (text.isEmpty()) {
		m_ui->filterLine->setStyleSheet(
		    SortFilterProxyModel::blankFilterEditStyle);
	} else if (m_contactListProxyModel.rowCount() != 0) {
		m_ui->filterLine->setStyleSheet(
		    SortFilterProxyModel::foundFilterEditStyle);
	} else {
		m_ui->filterLine->setStyleSheet(
		    SortFilterProxyModel::notFoundFilterEditStyle);
	}
}

void DlgContacts::fillContactsFromMessageDb(const MessageDbSet &dbSet)
{
	m_ui->contactTableView->setEnabled(false);
	m_contactTableModel.removeRows(0, m_contactTableModel.rowCount());

	QList<MessageDb::ContactEntry> foundBoxes(dbSet.uniqueContacts());

	m_ui->contactTableView->setEnabled(true);
	m_contactTableModel.appendData(foundBoxes);

	if (m_contactTableModel.rowCount() > 0) {
		m_ui->contactTableView->selectColumn(
		    BoxContactsModel::CHECKBOX_COL);
		m_ui->contactTableView->selectRow(0);
	}

	//m_ui->contactTableView->resizeColumnsToContents();
}
