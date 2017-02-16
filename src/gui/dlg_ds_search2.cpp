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

#include "src/gui/dlg_ds_search2.h"
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

DlgSearch2::DlgSearch2(Action action, QStringList &dbIdList, QWidget *parent,
    const QString &userName)
    : QDialog(parent),
    m_action(action),
    m_dbIdList(dbIdList),
    m_userName(userName),
    m_currentPage(0),
    m_target(FULLTEXT_ALL),
    m_box_type(DBTYPE_SYSTEM),
    m_phrase()
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
	this->resultGroupBox->hide();

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
	    SLOT(searchNewDataboxes()));
	connect(this->nextPushButton, SIGNAL(clicked()), this,
	    SLOT(showNextDataboxes()));
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
 * Start new search and show first page of results
 */
void DlgSearch2::searchNewDataboxes()
/* ========================================================================= */
{
	m_phrase = this->textLineEdit->text();
	m_currentPage = 0;

	if (this->dbIDRadioButton->isChecked()) {
		m_target = FULLTEXT_BOX_ID;
	} else if (this->icRadioButton->isChecked()) {
		m_target = FULLTEXT_IC;
	} else if (this->addressRadioButton->isChecked()) {
		m_target = FULLTEXT_ADDRESS;
	}

	if (this->ovmRadioButton->isChecked()) {
		m_box_type = DBTYPE_OVM;
	} else if (this->poRadioButton->isChecked()) {
		m_box_type = DBTYPE_PO;
	} else if (this->pfoRadioButton->isChecked()) {
		m_box_type = DBTYPE_PFO;
	} else if (this->foRradioButton->isChecked()) {
		m_box_type = DBTYPE_FO;
	} else {
		m_box_type = DBTYPE_SYSTEM;
	}

	findDataboxes(m_currentPage, m_target, m_box_type , m_phrase);
}


/* ========================================================================= */
/*
 * Show next page from result
 */
void DlgSearch2::showNextDataboxes()
/* ========================================================================= */
{
	++m_currentPage;
	findDataboxes(m_currentPage, m_target, m_box_type , m_phrase);
}


/* ========================================================================= */
/*
 * Search databoxes on ISDS
 */
void DlgSearch2::findDataboxes(quint64 pageNumber, isds_fulltext_target target,
isds_DbType box_type, const QString &phrase)
/* ========================================================================= */
{
	this->contactTableWidget->setRowCount(0);
	this->contactTableWidget->setEnabled(false);
	this->nextPushButton->setEnabled(false);
	this->nextPushButton->hide();
	this->resultGroupBox->hide();

	QString resultString = tr("Total found: 0") ;

	struct isds_list *boxes = NULL;
	TaskSearchOwnerFulltext *task;
	task = new (std::nothrow) TaskSearchOwnerFulltext(m_userName,
	    phrase, &target, &box_type,
	    TaskSearchOwnerFulltext::maxResponseSize, pageNumber);
	task->setAutoDelete(false);
	globWorkPool.runSingle(task);

	int ret = task->m_isdsRetError;
	boxes = task->m_results; task->m_results = NULL;

	this->resultGroupBox->show();
	this->resultGroupBox->setEnabled(true);

	if (ret != IE_SUCCESS) {
		delete task;
		this->searchResultText->setText(resultString);
		return;
	}

	quint64 totalDb = task->m_totalMatchingBoxes;
	quint64 currentPageStart = task->m_currentPageStart;
	quint64 currentPageSize = task->m_currentPageSize;
	bool isLastPage = task->m_isLastPage;

	delete task; task = NULL;

	struct isds_list *box;
	box = boxes;

	QString name = tr("Unknown");
	QString address = tr("Unknown");
	QString dbID = "n/a";
	isds_DbType dbType;

	if (isLastPage) {
		this->nextPushButton->setEnabled(false);
		this->nextPushButton->hide();
	} else {
		this->nextPushButton->setEnabled(true);
		this->nextPushButton->show();
	}

	QString interval = QString::number(currentPageStart) + "-" +
	    QString::number(currentPageStart + currentPageSize);
	resultString = tr("Total found: ") + QString::number(totalDb);
	resultString += "; " + tr("Shown: ") + interval;

	this->searchResultText->setText(resultString);

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
