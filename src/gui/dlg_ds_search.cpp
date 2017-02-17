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

#include <QMessageBox>

#include "src/gui/dlg_ds_search.h"
#include "src/io/isds_sessions.h"
#include "src/views/table_home_end_filter.h"
#include "src/views/table_space_selection_filter.h"
#include "src/worker/pool.h"
#include "src/worker/task_search_owner.h"

#define CON_COL_CHECKBOX 0
#define CON_COL_BOX_ID 1
#define CON_COL_BOX_NAME 2
#define CON_COL_ADDRESS 3
#define CON_COL_POST_CODE 4
#define CON_COL_PDZ 5

DlgDsSearch::DlgDsSearch(Action action, QTableWidget *recipientTableWidget,
    const QString &dbType, bool dbEffectiveOVM, bool dbOpenAddressing,
    QWidget *parent, const QString &userName)
    : QDialog(parent),
    pingTimer(Q_NULLPTR),
    m_action(action),
    m_recipientTableWidget(recipientTableWidget),
    m_dbType(dbType),
    m_dbEffectiveOVM(dbEffectiveOVM),
    m_dbOpenAddressing(dbOpenAddressing),
    m_userName(userName),
    m_showInfoLabel(false)
{
	setupUi(this);
	/* Set default line height for table views/widgets. */
	contactTableWidget->setNarrowedLineHeight();

	initSearchWindow();
}

/* ========================================================================= */
/*
 * Init ISDS search dialog
 */
void DlgDsSearch::initSearchWindow(void)
/* ========================================================================= */
{
	this->contactTableWidget->setColumnHidden(CON_COL_PDZ, true);

	QString dbOpenAddressing = "";
	QString toolTipInfo = "";

	if ("OVM" != m_dbType && !m_dbEffectiveOVM) {
		if (m_dbOpenAddressing) {
			toolTipInfo = tr("Your account is of type")
			    + " " + m_dbType + ".\n" +
			    tr("You have also Post Data Messages activated.\n"
			    "This means you can only search for accounts of "
			    "type OVM and accounts that have Post Data "
			    "Messages delivery activated.\nBecause of this "
			    "limitation the results of your current search "
			    "might not contain all otherwise matching"
			    " databoxes.");
			dbOpenAddressing = " - " +
			    tr("commercial messages are enabled");
		} else {
			toolTipInfo = tr("Your account is of type")
			    + " " + m_dbType + ".\n" +
			    tr("This means you can only search for accounts of "
			    "type OVM.\nThe current search settings will thus "
			    "probably yield no result.");
			dbOpenAddressing = " - " +
			    tr("commercial messages are disabled");
		}
		m_showInfoLabel = true;
	}

	this->accountInfo->setText("<strong>" + m_userName +
	    "</strong>" + " - " + m_dbType + dbOpenAddressing);

	this->infoLabel->setStyleSheet("QLabel { color: red }");
	this->infoLabel->setToolTip(toolTipInfo);
	this->infoLabel->hide();

	this->dataBoxTypeCBox->addItem(tr("OVM – Orgán veřejné moci"));
	this->dataBoxTypeCBox->addItem(tr("PO – Právnická osoba"));
	this->dataBoxTypeCBox->addItem(tr("PFO – Podnikající fyzická osoba"));
	this->dataBoxTypeCBox->addItem(tr("FO – Fyzická osoba"));

	this->contactTableWidget->setColumnWidth(CON_COL_CHECKBOX, 20);
	this->contactTableWidget->setColumnWidth(CON_COL_BOX_ID, 60);
	this->contactTableWidget->setColumnWidth(CON_COL_BOX_NAME, 120);
	this->contactTableWidget->setColumnWidth(CON_COL_ADDRESS, 100);

	connect(this->contactTableWidget,
	    SIGNAL(itemSelectionChanged()), this,
	    SLOT(setFirtsColumnActive()));

	connect(this->iDLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->iCLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->nameLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->pscLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->dataBoxTypeCBox, SIGNAL(currentIndexChanged (int)),
	    this, SLOT(checkInputFields()));
	connect(this->contactTableWidget,SIGNAL(itemClicked(QTableWidgetItem*)),
	    this, SLOT(enableOkButton()));
	connect(this->contactTableWidget,
	    SIGNAL(itemChanged(QTableWidgetItem*)), this,
	    SLOT(enableOkButton()));
	connect(this->buttonBox, SIGNAL(accepted()), this,
	    SLOT(insertDsItems()));
	connect(this->searchPushButton, SIGNAL(clicked()), this,
	    SLOT(searchDataBox()));
	connect(this->contactTableWidget, SIGNAL(doubleClicked(QModelIndex)),
	    this, SLOT(contactItemDoubleClicked(QModelIndex)));

	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	this->contactTableWidget->
	    setEditTriggers(QAbstractItemView::NoEditTriggers);

	this->contactTableWidget->installEventFilter(
	    new TableHomeEndFilter(this));
	this->contactTableWidget->installEventFilter(
	    new TableSpaceSelectionFilter(this));

	pingTimer = new QTimer(this);
	pingTimer->start(DLG_ISDS_KEEPALIVE_MS);

	connect(pingTimer, SIGNAL(timeout()), this,
	    SLOT(pingIsdsServer()));

	checkInputFields();
}


/* ========================================================================= */
/*
 * Set first column with checkbox active if item was changed
 */
void DlgDsSearch::setFirtsColumnActive(void)
/* ========================================================================= */
{
	this->contactTableWidget->selectColumn(CON_COL_CHECKBOX);
	this->contactTableWidget->selectRow(
	    this->contactTableWidget->currentRow());
}


/* ========================================================================= */
/*
 * Ping isds server, test if connection on isds server is active
 */
void DlgDsSearch::pingIsdsServer(void)
/* ========================================================================= */
{
	if (globIsdsSessions.isConnectedToIsds(m_userName)) {
		qDebug() << "Connection to ISDS is alive :)";
	} else {
		qDebug() << "Connection to ISDS is dead :(";
	}
}

/* ========================================================================= */
/*
 * Check input fields in the dialog
 */
void DlgDsSearch::checkInputFields(void)
/* ========================================================================= */
{
	this->nameLineEdit->setEnabled(true);
	this->pscLineEdit->setEnabled((true));
	this->iCLineEdit->setEnabled((true));
	this->iDLineEdit->setEnabled((true));

	switch (this->dataBoxTypeCBox->currentIndex()) {
	/* OVM */
	case 0:
		this->iCLineEdit->setEnabled(true);
		this->labelName->setText(tr("Subject Name:"));
		this->labelName->setToolTip(tr("Enter name of subject"));
		this->nameLineEdit->setToolTip(tr("Enter name of subject"));
		this->infoLabel->hide();
		break;
	/* PO */
	case 1:
		this->iCLineEdit->setEnabled(true);
		this->labelName->setText(tr("Subject Name:"));
		this->labelName->setToolTip(tr("Enter name of subject"));
		this->nameLineEdit->setToolTip(tr("Enter name of subject"));
		if (m_showInfoLabel) {
			this->infoLabel->show();
		} else {
			this->infoLabel->hide();
		}
		break;
	/* FPO */
	case 2:
		this->iCLineEdit->setEnabled(true);
		this->labelName->setText(tr("Name:"));
		this->labelName->setToolTip(tr("Enter PFO last name or company name."));
		this->nameLineEdit->setToolTip(tr("Enter PFO last name or company name."));
		if (m_showInfoLabel) {
			this->infoLabel->show();
		} else {
			this->infoLabel->hide();
		}
		break;
	/* FO */
	case 3:
		this->iCLineEdit->setEnabled(false);
		this->labelName->setText(tr("Last Name:"));
		this->labelName->setToolTip(tr("Enter last name or "
		    "birth last name of FO."));
		this->nameLineEdit->setToolTip(tr("Enter last name or "
		    "birth last name of FO."));
		if (m_showInfoLabel) {
			this->infoLabel->show();
		} else {
			this->infoLabel->hide();
		}
		break;
	default:
		break;
	}

	if (!this->iDLineEdit->text().isEmpty()) {
		this->nameLineEdit->setEnabled(false);
		this->pscLineEdit->setEnabled(false);
		this->iCLineEdit->setEnabled(false);

		if (this->iDLineEdit->text().length() == 7) {
			this->searchPushButton->setEnabled(true);
		}
		else {
			this->searchPushButton->setEnabled(false);
		}
	} else if (!this->iCLineEdit->text().isEmpty()) {
		this->iDLineEdit->setEnabled(false);
		this->nameLineEdit->setEnabled(false);
		this->pscLineEdit->setEnabled(false);

		if (this->iCLineEdit->text().length() == 8) {
			this->searchPushButton->setEnabled(true);
		}
		else {
			this->searchPushButton->setEnabled(false);
		}
	} else if (!this->nameLineEdit->text().isEmpty()) {
		this->iDLineEdit->setEnabled(false);

		if (this->nameLineEdit->text().length() > 2) {
			this->searchPushButton->setEnabled(true);
		}
		else {
			this->searchPushButton->setEnabled(false);
		}
	} else {
		this->searchPushButton->setEnabled(false);
		this->iDLineEdit->setEnabled(true);
		this->nameLineEdit->setEnabled(true);
		this->pscLineEdit->setEnabled(true);
	}
}


/* ========================================================================= */
/*
 * Call ISDS and find data boxes with given criteria
 */
void DlgDsSearch::searchDataBox(void)
/* ========================================================================= */
{
	this->contactTableWidget->setRowCount(0);
	this->contactTableWidget->setEnabled(false);

	if (this->iDLineEdit->text() == ID_ISDS_SYS_DATABOX) {
		QMessageBox::information(this, tr("Search result"),
		    tr("This is a special ID for system databox of "
		    "Datové schránky. You can't use this ID for "
		    "message delivery. Try again."),
		    QMessageBox::Ok);
		return;
	}

	enum TaskSearchOwner::Result result = TaskSearchOwner::SO_ERROR;
	QString errMsg;
	QString longErrMsg;

	struct isds_list *boxes = NULL;
	QList<QVector<QString>> list_contacts;

	enum TaskSearchOwner::BoxType boxType = TaskSearchOwner::BT_OVM;
	switch (this->dataBoxTypeCBox->currentIndex()) {
	case 3:
		boxType = TaskSearchOwner::BT_FO;
		break;
	case 2:
		boxType = TaskSearchOwner::BT_PFO;
		break;
	case 1:
		boxType = TaskSearchOwner::BT_PO;
		break;
	case 0:
	default:
		boxType = TaskSearchOwner::BT_OVM;
		break;
	}

	TaskSearchOwner::SoughtOwnerInfo soughtInfo(
	    this->iDLineEdit->text(), boxType, this->iCLineEdit->text(),
	    this->nameLineEdit->text(), this->nameLineEdit->text(),
	    this->nameLineEdit->text(), this->pscLineEdit->text());

	TaskSearchOwner *task =
	    new (std::nothrow) TaskSearchOwner(m_userName, soughtInfo);
	task->setAutoDelete(false);
	globWorkPool.runSingle(task);

	result = task->m_result;
	errMsg = task->m_isdsError;
	longErrMsg = task->m_isdsLongError;
	boxes = task->m_results; task->m_results = NULL;

	delete task;

	switch (result) {
	case TaskSearchOwner::SO_SUCCESS:
		break;
	case TaskSearchOwner::SO_BAD_DATA:
		QMessageBox::information(this, tr("Search result"),
		    longErrMsg, QMessageBox::Ok);
		goto fail;
		break;
	case TaskSearchOwner::SO_COM_ERROR:
		QMessageBox::information(this, tr("Search result"),
		    tr("It is not possible find databox because") + ":\n\n" +
		    longErrMsg, QMessageBox::Ok);
		goto fail;
		break;
	case TaskSearchOwner::SO_ERROR:
	default:
		QMessageBox::critical(this, tr("Search error"),
		    tr("It is not possible find databox because "
		         "error occurred during search process!"),
		    QMessageBox::Ok);
		goto fail;
		break;
	}

	struct isds_list *box;
	box = boxes;

	while (0 != box) {

		this->contactTableWidget->setEnabled(true);
		isds_DbOwnerInfo *item = (isds_DbOwnerInfo *) box->data;
		Q_ASSERT(0 != item);
		QVector<QString> contact;
		contact.append(item->dbID);

		QString name;

		if ((item->dbType != NULL) && (*item->dbType == DBTYPE_FO)) {
			name = QString(item->personName->pnFirstName) +
			    " " + QString(item->personName->pnLastName);
		} else if ((item->dbType != NULL) &&
		           (*item->dbType == DBTYPE_PFO)) {
			QString firmName = item->firmName;
			if (firmName.isEmpty() || firmName == " ") {
				name = QString(item->personName->pnFirstName) +
				    " " + QString(item->personName->pnLastName);
			} else {
				name = item->firmName;
			}
		} else {
			name = item->firmName;
		}

		QString address, street, adNumberInStreet, adNumberInMunicipality;
		if (NULL != item->address) {

			street = item->address->adStreet;
			adNumberInStreet = item->address->adNumberInStreet;
			adNumberInMunicipality = item->address->adNumberInMunicipality;

			if (street.isEmpty() || street == " ") {
				address = item->address->adCity;
			} else {
				address = street;
				if (adNumberInStreet.isEmpty() ||
				    adNumberInStreet == " ") {
					address += + " " +
					    adNumberInMunicipality +
					    ", " + QString(item->address->adCity);
				} else if (adNumberInMunicipality.isEmpty() ||
				    adNumberInMunicipality == " ") {
					address += + " " +
					    adNumberInStreet +
					    ", " + QString(item->address->adCity);
				} else {
					address += + " "+
					    adNumberInMunicipality
					    + "/" +
					    adNumberInStreet +
					", " + QString(item->address->adCity);
				}
			}

			contact.append(name);
			contact.append(address);
			contact.append(QString(item->address->adZipCode));
			contact.append(*item->dbEffectiveOVM ? tr("no") : tr("yes"));

			list_contacts.append(contact);
			addContactsToTable(list_contacts);
		}

		box = box->next;
	}

	this->contactTableWidget->resizeColumnsToContents();

fail:
	isds_list_free(&boxes);
}


/* ========================================================================= */
/*
 * Enable action button
 */
void DlgDsSearch::enableOkButton(void)
/* ========================================================================= */
{
	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	for (int i = 0; i < this->contactTableWidget->rowCount(); ++i) {
		if (this->contactTableWidget->item(i, CON_COL_CHECKBOX)->checkState()) {
			this->buttonBox->button(QDialogButtonBox::Ok)->
			    setEnabled(true);
		}
	}
}


/* ========================================================================= */
/*
 * At contact list from ISDS into search result table widget
 */
void DlgDsSearch::addContactsToTable(
    const QList< QVector<QString> > &contactList)
/* ========================================================================= */
{
	this->contactTableWidget->setRowCount(0);

	this->contactTableWidget->setEnabled(true);
	for (int i = 0; i < contactList.count(); i++) {

		int row = this->contactTableWidget->rowCount();
		this->contactTableWidget->insertRow(row);
		QTableWidgetItem *item = new QTableWidgetItem;
		item->setCheckState(Qt::Unchecked);
		this->contactTableWidget->setItem(row, CON_COL_CHECKBOX, item);
		item = new QTableWidgetItem;
		item->setText(contactList[i].at(0));
		this->contactTableWidget->setItem(row, CON_COL_BOX_ID, item);
		item = new QTableWidgetItem;
		item->setText(contactList[i].at(1));
		this->contactTableWidget->setItem(row, CON_COL_BOX_NAME, item);
		item = new QTableWidgetItem;
		item->setText(contactList[i].at(2));
		this->contactTableWidget->setItem(row, CON_COL_ADDRESS, item);
		item = new QTableWidgetItem;
		item->setText(contactList[i].at(3));
		this->contactTableWidget->setItem(row, CON_COL_POST_CODE, item);
		item = new QTableWidgetItem;
		item->setText(contactList[i].at(4));
		this->contactTableWidget->setItem(row, CON_COL_PDZ, item);
	}

	if (this->contactTableWidget->rowCount() > 0) {
		this->contactTableWidget->selectColumn(CON_COL_CHECKBOX);
		this->contactTableWidget->selectRow(0);
	}

}


/* ========================================================================= */
/*
 * Test if the selected item is not in recipient list
 */
bool DlgDsSearch::isInRecipientTable(const QString &idDs) const
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
void DlgDsSearch::insertDsItems(void)
/* ========================================================================= */
{
	if (ACT_ADDNEW == m_action) {
		Q_ASSERT(0 != m_recipientTableWidget);
		for (int i = 0; i < this->contactTableWidget->rowCount(); ++i) {
			if (this->contactTableWidget->item(i, CON_COL_CHECKBOX)->checkState()) {
				insertContactToRecipentTable(i);
			}
		}
	}
}


/* ========================================================================= */
/*
 * Doubleclick of selected contact.
 */
void DlgDsSearch::contactItemDoubleClicked(const QModelIndex &index)
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
void DlgDsSearch::insertContactToRecipentTable(int selRow)
/* ========================================================================= */
{
	if (!isInRecipientTable(
	        this->contactTableWidget->item(selRow, 1)->text())) {

		int row = m_recipientTableWidget->rowCount();
		m_recipientTableWidget->insertRow(row);
		QTableWidgetItem *item = new QTableWidgetItem;
		item->setText(this->contactTableWidget->
		    item(selRow, CON_COL_BOX_ID)->text());
		this->m_recipientTableWidget->setItem(row,0,item);
		item = new QTableWidgetItem;
		item->setText(this->contactTableWidget->
		    item(selRow, CON_COL_BOX_NAME)->text());
		this->m_recipientTableWidget->setItem(row,1,item);
		item = new QTableWidgetItem;
		item->setText(this->contactTableWidget->
		    item(selRow, CON_COL_ADDRESS)->text() + " " +
		    this->contactTableWidget->item(selRow, CON_COL_POST_CODE)->text());
		this->m_recipientTableWidget->setItem(row, 2, item);
		item = new QTableWidgetItem;
		item->setText(m_dbEffectiveOVM ? tr("no") :
		    this->contactTableWidget->item(selRow, CON_COL_PDZ)->text());
		item->setTextAlignment(Qt::AlignCenter);
		this->m_recipientTableWidget->setItem(row, 3, item);
	}
}
