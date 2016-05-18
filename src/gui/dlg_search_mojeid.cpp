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


#include <cstddef>
#include <QMessageBox>

#include "src/gui/dlg_search_mojeid.h"
#include "src/views/table_home_end_filter.h"
#include "src/views/table_space_selection_filter.h"
#include "src/web/json.h"

DlgDsSearchMojeId::DlgDsSearchMojeId(Action action,
    QTableWidget *recipientTableWidget, const QString &dbType,
    bool dbEffectiveOVM, QWidget *parent, const QString &userName)
    : QDialog(parent),
    m_action(action),
    m_recipientTableWidget(recipientTableWidget),
    m_dbType(dbType),
    m_dbEffectiveOVM(dbEffectiveOVM),
    m_userName(userName),
    m_limit(0)
{
	setupUi(this);
	initSearchWindow();
}


/* ========================================================================= */
/*
 * Init ISDS search dialog
 */
void DlgDsSearchMojeId::initSearchWindow(void)
/* ========================================================================= */
{
	this->resultsTableWidget->setColumnHidden(5, true);

	this->accountInfo->setText("<strong>" + m_userName +
	    "</strong>" + " - " + m_dbType);

	this->resultsTableWidget->setColumnWidth(0,20);
	this->resultsTableWidget->setColumnWidth(1,60);
	this->resultsTableWidget->setColumnWidth(2,120);

	connect(this->resultsTableWidget,
	    SIGNAL(itemSelectionChanged()), this,
	    SLOT(setFirtsColumnActive()));

	connect(this->keywordLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->resultsTableWidget,SIGNAL(itemClicked(QTableWidgetItem*)),
	    this, SLOT(enableOkButton()));
	connect(this->resultsTableWidget,
	    SIGNAL(itemChanged(QTableWidgetItem*)), this,
	    SLOT(enableOkButton()));
	connect(this->buttonBox, SIGNAL(accepted()), this,
	    SLOT(insertDsItems()));
	connect(this->searchPushButton, SIGNAL(clicked()), this,
	    SLOT(searchDataBox()));
	connect(this->resultsTableWidget, SIGNAL(doubleClicked(QModelIndex)),
	    this, SLOT(contactItemDoubleClicked(QModelIndex)));

	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	this->resultsTableWidget->
	    setEditTriggers(QAbstractItemView::NoEditTriggers);

	this->resultsTableWidget->installEventFilter(
	    new TableHomeEndFilter(this));
	this->resultsTableWidget->installEventFilter(
	    new TableSpaceSelectionFilter(this));

	checkInputFields();
}


/* ========================================================================= */
/*
 * Set first column with checkbox active if item was changed
 */
void DlgDsSearchMojeId::setFirtsColumnActive(void)
/* ========================================================================= */
{
	this->resultsTableWidget->selectColumn(0);
	this->resultsTableWidget->selectRow(
	    this->resultsTableWidget->currentRow());
}


/* ========================================================================= */
/*
 * Check input fields in the dialog
 */
void DlgDsSearchMojeId::checkInputFields(void)
/* ========================================================================= */
{
	this->searchPushButton->setEnabled(false);
	if (this->keywordLineEdit->text().length() > 0) {
		this->searchPushButton->setEnabled(true);
	}
	this->searchPushButton->setText(tr("Search"));
	m_limit = 0;
}


/* ========================================================================= */
/*
 * Call ISDS and find data boxes with given criteria
 */
void DlgDsSearchMojeId::searchDataBox(void)
/* ========================================================================= */
{
	if (m_limit == 0) {
		this->resultsTableWidget->setRowCount(0);
	}
	this->resultsTableWidget->setEnabled(false);

	QList<JsonLayer::Recipient> rList;
	bool hasMore = false;
	QString errStr;

	QString aID  = m_userName.split("-").at(1);
	int accoutID = aID.toInt();

	JsonLayer jsonLayer;
	jsonLayer.searchRecipient(accoutID, this->keywordLineEdit->text(),
	    m_limit, rList, hasMore, errStr);

	if (rList.isEmpty()) {
		return;
	}

	this->resultsTableWidget->setEnabled(true);
	foreach (const JsonLayer::Recipient &recipient, rList) {
		int row = this->resultsTableWidget->rowCount();
		this->resultsTableWidget->insertRow(row);
		QTableWidgetItem *item = new QTableWidgetItem;
		item->setCheckState(Qt::Unchecked);
		this->resultsTableWidget->setItem(row,0,item);
		item = new QTableWidgetItem;
		item->setText(recipient.recipientDbId);
		this->resultsTableWidget->setItem(row,1,item);
		item = new QTableWidgetItem;
		item->setText(recipient.recipientName);
		this->resultsTableWidget->setItem(row,2,item);
		item = new QTableWidgetItem;
		item->setText(recipient.recipientAddress);
		this->resultsTableWidget->setItem(row,3,item);
	}

	if (this->resultsTableWidget->rowCount() > 0) {
		this->resultsTableWidget->selectColumn(0);
		this->resultsTableWidget->selectRow(0);
	}

	if (hasMore) {
		this->searchPushButton->setText(tr("Search next"));
		m_limit++;
	}
}


/* ========================================================================= */
/*
 * Enable action button
 */
void DlgDsSearchMojeId::enableOkButton(void)
/* ========================================================================= */
{
	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	for (int i = 0; i < this->resultsTableWidget->rowCount(); i++) {
		if (this->resultsTableWidget->item(i,0)->checkState()) {
			this->buttonBox->button(QDialogButtonBox::Ok)->
			    setEnabled(true);
		}
	}
}


/* ========================================================================= */
/*
 * Test if the selected item is not in recipient list
 */
bool DlgDsSearchMojeId::isInRecipientTable(const QString &idDs) const
/* ========================================================================= */
{
	Q_ASSERT(0 != m_recipientTableWidget);

	for (int i = 0; i < this->m_recipientTableWidget->rowCount(); i++) {
		if (this->m_recipientTableWidget->item(i,0)->text() == idDs) {
			return true;
		}
	}
	return false;
}


/* ========================================================================= */
/*
 * Insert selected contacts into recipient list of the sent message dialog.
 */
void DlgDsSearchMojeId::insertDsItems(void)
/* ========================================================================= */
{
	if (ACT_ADDNEW == m_action) {
		Q_ASSERT(0 != m_recipientTableWidget);
		for (int i = 0; i < this->resultsTableWidget->rowCount(); i++) {
			if (this->resultsTableWidget->item(i,0)->checkState()) {
				insertContactToRecipentTable(i);
			}
		}
	}
}


/* ========================================================================= */
/*
 * Doubleclick of selected contact.
 */
void DlgDsSearchMojeId::contactItemDoubleClicked(const QModelIndex &index)
/* ========================================================================= */
{
	if (ACT_ADDNEW == m_action) {

		if (!index.isValid()) {
			this->close();
			return;
		}

		insertContactToRecipentTable(index.row());

		this->close();
	}
}


/* ========================================================================= */
/*
 * Insert contact into recipient list of the sent message dialog.
 */
void DlgDsSearchMojeId::insertContactToRecipentTable(int selRow)
/* ========================================================================= */
{
	if (!isInRecipientTable(
	    this->resultsTableWidget->item(selRow, 1)->text())) {

		int row = m_recipientTableWidget->rowCount();
		m_recipientTableWidget->insertRow(row);
		QTableWidgetItem *item = new QTableWidgetItem;
		item->setText(this->resultsTableWidget->
		    item(selRow, 1)->text());
		this->m_recipientTableWidget->setItem(row,0,item);
		item = new QTableWidgetItem;
		item->setText(this->resultsTableWidget->
		    item(selRow, 2)->text());
		this->m_recipientTableWidget->setItem(row,1,item);
		item = new QTableWidgetItem;
		item->setText(this->resultsTableWidget->
		    item(selRow, 3)->text() + " " +
		    this->resultsTableWidget->item(selRow, 4)->text());
		this->m_recipientTableWidget->setItem(row, 2, item);
		item = new QTableWidgetItem;
		item->setText(m_dbEffectiveOVM ? tr("no") :
		    this->resultsTableWidget->item(selRow, 5)->text());
		item->setTextAlignment(Qt::AlignCenter);
		this->m_recipientTableWidget->setItem(row, 3, item);
	}
}
