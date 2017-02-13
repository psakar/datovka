/*
 * Copyright (C) 2014-2015 CZ.NIC
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
    QStringList &dbIdList, QWidget *parent)
    : QDialog(parent),
    m_dbSet(dbSet),
    m_dbId(dbId),
    m_dbIdList(dbIdList)
{
	setupUi(this);
	/* Set default line height for table views/widgets. */
	contactTableWidget->setNarrowedLineHeight();

	this->contactTableWidget->setColumnWidth(0,20);
	this->contactTableWidget->setColumnWidth(1,70);
	this->contactTableWidget->setColumnWidth(2,150);

	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	this->clearPushButton->setEnabled(false);

	fillContactsFromMessageDb();

	if (this->contactTableWidget->rowCount() > 0) {
		this->contactTableWidget->selectColumn(0);
		this->contactTableWidget->selectRow(0);
	}

	connect(this->contactTableWidget,
	    SIGNAL(itemSelectionChanged()), this,
	    SLOT(setFirtsColumnActive()));
	connect(this->filterLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(filterContact(QString)));
	connect(this->contactTableWidget,
	    SIGNAL(itemClicked(QTableWidgetItem*)), this,
	    SLOT(enableOkButton()));
	connect(this->contactTableWidget,
	    SIGNAL(itemChanged(QTableWidgetItem*)), this,
	    SLOT(enableOkButton()));
	connect(this->clearPushButton, SIGNAL(clicked()), this,
	    SLOT(clearContactText()));
	connect(this->buttonBox, SIGNAL(accepted()), this,
	    SLOT(addSelectedDbIDs()));
	connect(this->contactTableWidget, SIGNAL(doubleClicked(QModelIndex)),
	    this, SLOT(contactItemDoubleClicked(QModelIndex)));

	this->contactTableWidget->
	    setEditTriggers(QAbstractItemView::NoEditTriggers);

	this->contactTableWidget->installEventFilter(
	    new TableHomeEndFilter(this));
	this->contactTableWidget->installEventFilter(
	    new TableSpaceSelectionFilter(this));
}


/* ========================================================================= */
/*
 * Set first column with checkbox active if item was changed
 */
void DlgContacts::setFirtsColumnActive(void)
/* ========================================================================= */
{
	this->contactTableWidget->selectColumn(0);
	this->contactTableWidget->selectRow(
	    this->contactTableWidget->currentRow());
}


/* ========================================================================= */
/*
 * Apply filter text on the tablewidget
 */
void DlgContacts::filterContact(const QString &text)
/* ========================================================================= */
{
	this->clearPushButton->setEnabled(true);
	if (!text.isEmpty()) {
		for (int i = 0; i < this->contactTableWidget->rowCount(); i++) {
			contactTableWidget->hideRow(i);
		}
		QList<QTableWidgetItem *> items =
		    contactTableWidget->findItems(text, Qt::MatchContains);
		for (int i = 0; i < items.count(); i++) {
				contactTableWidget->showRow(items.at(i)->row());
		}
	} else {
		this->clearPushButton->setEnabled(false);
		for (int i = 0; i < this->contactTableWidget->rowCount(); i++) {
			contactTableWidget->showRow(i);
		}
	}
}


/* ========================================================================= */
/*
 * Clear search text in the filterLineEdit
 */
void DlgContacts::clearContactText(void)
/* ========================================================================= */
{
	this->filterLineEdit->clear();
	this->clearPushButton->setEnabled(false);
}


/* ========================================================================= */
/*
 * Get contacts from message db and fill wablewidget
 */
void DlgContacts::fillContactsFromMessageDb()
/* ========================================================================= */
{
	QList<MessageDb::ContactEntry> contactList(m_dbSet.uniqueContacts());

	foreach (const MessageDb::ContactEntry &entry, contactList) {
		if (m_dbId != entry.boxId) {
			int row = this->contactTableWidget->rowCount();
			this->contactTableWidget->insertRow(row);
			QTableWidgetItem *item = new QTableWidgetItem;
			item->setCheckState(Qt::Unchecked);
			this->contactTableWidget->setItem(row, 0, item);
			item = new QTableWidgetItem;
			item->setText(entry.boxId);
			this->contactTableWidget->setItem(row, 1, item);
			item = new QTableWidgetItem;
			item->setText(entry.name);
			this->contactTableWidget->setItem(row, 2, item);
			item = new QTableWidgetItem;
			item->setText(entry.address);
			this->contactTableWidget->setItem(row, 3, item);
		};
	}
	this->contactTableWidget->resizeColumnsToContents();
}


/* ========================================================================= */
/*
 * Enable ok (add) button if some contact was selected
 */
void DlgContacts::enableOkButton(void)
/* ========================================================================= */
{
	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	for (int i = 0; i < this->contactTableWidget->rowCount(); i++) {
		if (this->contactTableWidget->item(i,0)->checkState()) {
			this->buttonBox->button(QDialogButtonBox::Ok)->
			    setEnabled(true);
		}
	}
}


/* ========================================================================= */
/*
 * Add selected recipient databox IDs into recipient list
 */
void DlgContacts::addSelectedDbIDs(void)
/* ========================================================================= */
{
	for (int i = 0; i < this->contactTableWidget->rowCount(); i++) {
		if (this->contactTableWidget->item(i,0)->checkState()) {
			m_dbIdList.append(this->contactTableWidget->
			    item(i, 1)->text());
		}
	}
}


/* ========================================================================= */
/*
 * Add selected recipient databox ID into recipient list
 */
void DlgContacts::contactItemDoubleClicked(const QModelIndex &index)
/* ========================================================================= */
{
	if (!index.isValid()) {
		this->close();
		return;
	}

	m_dbIdList.append(this->contactTableWidget->
	    item(index.row(), 1)->text());
	this->close();
}
