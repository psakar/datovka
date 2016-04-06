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


#include "dlg_contacts.h"
#include "src/io/isds_sessions.h"
#include "src/views/table_home_end_filter.h"
#include "src/views/table_space_selection_filter.h"
#include "src/worker/pool.h"
#include "src/worker/task_search_owner.h"


DlgContacts::DlgContacts(const MessageDbSet &dbSet, const QString &dbId,
    QTableWidget &recipientTableWidget,
    QString dbType, bool dbEffectiveOVM, bool dbOpenAddressing,
    QWidget *parent, const QString &userName)
    : QDialog(parent),
    m_recipientTableWidget(recipientTableWidget),
    m_dbSet(dbSet),
    m_dbId(dbId),
    m_dbType(dbType),
    m_dbEffectiveOVM(dbEffectiveOVM),
    m_dbOpenAddressing(dbOpenAddressing),
    m_userName(userName)
{
	setupUi(this);

	this->contactTableWidget->setColumnWidth(0,20);
	this->contactTableWidget->setColumnWidth(1,60);
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
	    SLOT(insertDsItems()));
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
 * Test if recipient exists in recipient tablewidget
 */
bool DlgContacts::isInRecipientTable(const QString &idDs) const
/* ========================================================================= */
{
	for (int i = 0; i < this->m_recipientTableWidget.rowCount(); i++) {
		if (this->m_recipientTableWidget.item(i,0)->text() == idDs)
		return true;
	}
	return false;
}


/* ========================================================================= */
/*
 * Insert selected recipients into send message dialog tablewidget
 */
void DlgContacts::insertDsItems(void)
/* ========================================================================= */
{
	for (int i = 0; i < this->contactTableWidget->rowCount(); i++) {
		if (this->contactTableWidget->item(i,0)->checkState()) {
			insertContactToRecipentTable(i);
		}
	}
}


/* ========================================================================= */
/*
 * Get and return dbEffectiveOVM info for recipient
 */
QString DlgContacts::getUserInfoFromIsds(const QString &userName,
    const QString &idDbox)
/* ========================================================================= */
{
	QString str = tr("no");
	struct isds_DbOwnerInfo *doi = NULL;
	struct isds_list *box = NULL;
	isds_DbType dbType = DBTYPE_FO;

	doi = isds_DbOwnerInfo_createConsume(idDbox, dbType, QString(),
	    NULL, QString(), NULL, NULL, QString(), QString(), QString(),
	    QString(), QString(), 0, false, false);
	if (NULL == doi) {
		return str;
	}

	TaskSearchOwner *task;

	task = new (std::nothrow) TaskSearchOwner(userName, doi);
	task->setAutoDelete(false);
	globWorkPool.runSingle(task);

	box = task->m_results; task->m_results = NULL;

	delete task;
	isds_DbOwnerInfo_free(&doi);

	if (NULL != box) {
		const struct isds_DbOwnerInfo *item = (isds_DbOwnerInfo *)
		    box->data;
		Q_ASSERT(NULL != item);
		str = *item->dbEffectiveOVM ? tr("no") : tr("yes");
	}

	isds_list_free(&box);

	return str;
}

void DlgContacts::contactItemDoubleClicked(const QModelIndex &index)
{
	if (!index.isValid()) {
		this->close();
		return;
	}

	insertContactToRecipentTable(index.row());

	this->close();
}


void DlgContacts::insertContactToRecipentTable(int selRow)
{
	if (!isInRecipientTable(
	    this->contactTableWidget->item(selRow, 1)->text())) {
		int row = m_recipientTableWidget.rowCount();
		m_recipientTableWidget.insertRow(row);
		QTableWidgetItem *item = new QTableWidgetItem;
		item->setText(this->contactTableWidget->
		    item(selRow, 1)->text());
		this->m_recipientTableWidget.setItem(row, 0,
		    item);
		item = new QTableWidgetItem;
		item->setText(this->contactTableWidget->
		    item(selRow, 2)->text());
		this->m_recipientTableWidget.setItem(row, 1,
		    item);
		item = new QTableWidgetItem;
		item->setText(this->contactTableWidget->
		    item(selRow, 3)->text());
		this->m_recipientTableWidget.setItem(row, 2,
		    item);
		item = new QTableWidgetItem;
		if (!m_dbEffectiveOVM) {
			item->setText(getUserInfoFromIsds(
			    m_userName,
			    this->contactTableWidget->item(selRow ,1)
			    ->text()));
		} else {
			item->setText(tr("no"));
		}
		item->setTextAlignment(Qt::AlignCenter);
		this->m_recipientTableWidget.setItem(row, 3, item);
	}
}
