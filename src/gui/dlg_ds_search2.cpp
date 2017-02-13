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

#include "src/gui/dlg_ds_search2.h"
#include "src/io/isds_sessions.h"
#include "src/views/table_home_end_filter.h"
#include "src/views/table_space_selection_filter.h"
#include "src/worker/pool.h"
#include "src/worker/task_search_owner_fulltext.h"

/*
 * Column indexes into recipient table widget.
 */
#define RTW_CHECK 0
#define RTW_ID 1
#define RTW_TYPE 2
#define RTW_NAME 3
#define RTW_ADDR 4
// Max items per page
#define MAXSIZE 100


DlgSearch2::DlgSearch2(Action action, QStringList &dbIdList, QWidget *parent,
    const QString &userName)
    : QDialog(parent),
    m_action(action),
    m_dbIdList(dbIdList),
    m_userName(userName)
{
	setupUi(this);
	/* Set default line height for table views/widgets. */
	this->contactTableWidget->setEnabled(false);
	this->contactTableWidget->setNarrowedLineHeight();
	this->contactTableWidget->setColumnWidth(RTW_CHECK,20);
	this->contactTableWidget->setColumnWidth(RTW_ID,70);
	this->contactTableWidget->setColumnWidth(RTW_TYPE,70);
	this->contactTableWidget->setColumnWidth(RTW_NAME,200);

	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	this->searchPushButton->setEnabled(false);

	if (this->contactTableWidget->rowCount() > 0) {
		this->contactTableWidget->selectColumn(0);
		this->contactTableWidget->selectRow(0);
	}

	connect(this->contactTableWidget,
	    SIGNAL(itemSelectionChanged()), this,
	    SLOT(setFirtsColumnActive()));
	connect(this->textLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(enableSearchButton(QString)));
	connect(this->contactTableWidget,
	    SIGNAL(itemClicked(QTableWidgetItem*)), this,
	    SLOT(enableOkButton()));
	connect(this->contactTableWidget,
	    SIGNAL(itemChanged(QTableWidgetItem*)), this,
	    SLOT(enableOkButton()));
	connect(this->searchPushButton, SIGNAL(clicked()), this,
	    SLOT(findDataboxes()));
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
void DlgSearch2::setFirtsColumnActive(void)
/* ========================================================================= */
{
	this->contactTableWidget->selectColumn(0);
	this->contactTableWidget->selectRow(
	    this->contactTableWidget->currentRow());
}


/* ========================================================================= */
/*
 * Get contacts from message db and fill wablewidget
 */
void DlgSearch2::findDataboxes()
/* ========================================================================= */
{
	this->contactTableWidget->setRowCount(0);
	this->contactTableWidget->setEnabled(false);

	long max = MAXSIZE;
	struct isds_list *boxes = NULL;
	TaskSearchOwnerFulltext *task;
	isds_fulltext_target target = FULLTEXT_ALL;
	isds_DbType box_type;

	if (this->dbIDRadioButton->isChecked()) {
		target = FULLTEXT_BOX_ID;
	} else if (this->icRadioButton->isChecked()) {
		target = FULLTEXT_IC;
	} else if (this->addressRadioButton->isChecked()) {
		target = FULLTEXT_ADDRESS;
	}

	if (this->ovmRadioButton->isChecked()) {
		box_type = DBTYPE_OVM;
	} else if (this->poRadioButton->isChecked()) {
		box_type = DBTYPE_PO;
	} else if (this->pfoRadioButton->isChecked()) {
		box_type = DBTYPE_PFO;
	} else if (this->foRradioButton->isChecked()) {
		box_type = DBTYPE_FO;
	}

	task = new (std::nothrow) TaskSearchOwnerFulltext(m_userName,
	    this->textLineEdit->text(), &target, &box_type, NULL, &max);
	task->setAutoDelete(false);
	globWorkPool.runSingle(task);

	boxes = task->m_results; task->m_results = NULL;
	delete task;

	struct isds_list *box;
	box = boxes;

	QString name = tr("Unknown");
	QString address = tr("Unknown");
	QString dbID = "n/a";
	isds_DbType dbType;

	while (0 != box) {

		this->contactTableWidget->setEnabled(true);

		isds_fulltext_result *boxdata =
		    (isds_fulltext_result *) box->data;
		Q_ASSERT(0 != boxdata);
		if (NULL != boxdata->dbID) {
			dbID = boxdata->dbID;
		}
		dbType = boxdata->dbType;
		if (NULL != boxdata->name) {
			name = boxdata->name;
		}
		if (NULL != boxdata->address) {
			address = boxdata->address;
		}

		int row = this->contactTableWidget->rowCount();
		this->contactTableWidget->insertRow(row);
		QTableWidgetItem *item = new QTableWidgetItem;
		item->setCheckState(Qt::Unchecked);
		this->contactTableWidget->setItem(row, RTW_CHECK, item);
		item = new QTableWidgetItem;
		item->setText(dbID);
		this->contactTableWidget->setItem(row, RTW_ID, item);
		item = new QTableWidgetItem;
		item->setText(convertDbTypeToString(dbType));
		this->contactTableWidget->setItem(row, RTW_TYPE, item);
		item = new QTableWidgetItem;
		item->setText(name);
		this->contactTableWidget->setItem(row, RTW_NAME, item);
		item = new QTableWidgetItem;
		item->setText(address);
		this->contactTableWidget->setItem(row, RTW_ADDR, item);
		//this->contactTableWidget->resizeColumnsToContents();

		box = box->next;
	}

	isds_list_free(&boxes);

}


/* ========================================================================= */
/*
 * Enable ok (add) button if some contact was selected
 */
void DlgSearch2::enableSearchButton(const QString &text)
/* ========================================================================= */
{
	this->searchPushButton->setEnabled(text.count() > 2);
}



/* ========================================================================= */
/*
 * Enable ok (add) button if some contact was selected
 */
void DlgSearch2::enableOkButton(void)
/* ========================================================================= */
{
	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	for (int i = 0; i < this->contactTableWidget->rowCount(); i++) {
		if (this->contactTableWidget->item(i,RTW_CHECK)->checkState()) {
			this->buttonBox->button(QDialogButtonBox::Ok)->
			    setEnabled(true);
		}
	}
}



/* ========================================================================= */
/*
 * Add selected recipient databox IDs into recipient list
 */
void DlgSearch2::addSelectedDbIDs(void)
/* ========================================================================= */
{
	for (int i = 0; i < this->contactTableWidget->rowCount(); i++) {
		if (this->contactTableWidget->item(i,RTW_CHECK)->checkState()) {
			m_dbIdList.append(this->contactTableWidget->
			    item(i, RTW_ID)->text());
		}
	}
}


/* ========================================================================= */
/*
 * Add selected recipient databox ID into recipient list
 */
void DlgSearch2::contactItemDoubleClicked(const QModelIndex &index)
/* ========================================================================= */
{
	if (!index.isValid()) {
		this->close();
		return;
	}

	m_dbIdList.append(this->contactTableWidget->
	    item(index.row(), RTW_ID)->text());
	this->close();
}
