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

#define CON_COL_CHECKBOX 0
#define CON_COL_BOX_ID 1
#define CON_COL_BOX_NAME 2
#define CON_COL_ADDRESS 3
#define CON_COL_POST_CODE 4
#define CON_COL_PDZ 5

#define CBOX_TYPE_OVM 0
#define CBOX_TYPE_PO 1
#define CBOX_TYPE_PFO 2
#define CBOX_TYPE_FO 3

DlgDsSearch::DlgDsSearch(const QString &userName, const QString &dbType,
    bool dbEffectiveOVM, bool dbOpenAddressing, QStringList *dbIdList,
    QWidget *parent)
    : QDialog(parent),
    m_userName(userName),
    m_dbType(dbType),
    m_dbEffectiveOVM(dbEffectiveOVM),
    m_dbOpenAddressing(dbOpenAddressing),
    m_dbIdList(dbIdList),
    m_pingTimer(Q_NULLPTR),
    m_showInfoLabel(false)
{
	setupUi(this);
	/* Set default line height for table views/widgets. */
	contactTableWidget->setNarrowedLineHeight();

	initSearchWindow();
}

DlgDsSearch::~DlgDsSearch(void)
{
	if (m_pingTimer != Q_NULLPTR) {
		delete m_pingTimer;
	}
}

void DlgDsSearch::enableOkButton(void)
{
	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	for (int i = 0; i < this->contactTableWidget->rowCount(); ++i) {
		if (this->contactTableWidget->item(i, CON_COL_CHECKBOX)->checkState()) {
			this->buttonBox->button(QDialogButtonBox::Ok)->
			    setEnabled(true);
			return;
		}
	}
}

void DlgDsSearch::setFirtsColumnActive(void)
{
	this->contactTableWidget->selectColumn(CON_COL_CHECKBOX);
	this->contactTableWidget->selectRow(
	    this->contactTableWidget->currentRow());
}

void DlgDsSearch::checkInputFields(void)
{
	this->nameLineEdit->setEnabled(true);
	this->pscLineEdit->setEnabled((true));
	this->iCLineEdit->setEnabled((true));
	this->iDLineEdit->setEnabled((true));

	switch (this->dataBoxTypeCBox->currentIndex()) {
	case CBOX_TYPE_OVM:
		this->iCLineEdit->setEnabled(true);
		this->labelName->setText(tr("Subject Name:"));
		this->labelName->setToolTip(tr("Enter name of subject"));
		this->nameLineEdit->setToolTip(tr("Enter name of subject"));
		this->infoLabel->hide();
		break;
	case CBOX_TYPE_PO:
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
	case CBOX_TYPE_PFO:
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
	case CBOX_TYPE_FO:
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

void DlgDsSearch::searchDataBox(void)
{
	if (this->iDLineEdit->text() == ID_ISDS_SYS_DATABOX) {
		QMessageBox::information(this, tr("Search result"),
		    tr("This is a special ID for system databox of "
		    "Datové schránky. You can't use this ID for "
		    "message delivery. Try again."),
		    QMessageBox::Ok);
		return;
	}

	enum TaskSearchOwner::BoxType boxType = TaskSearchOwner::BT_OVM;
	switch (this->dataBoxTypeCBox->currentIndex()) {
	case CBOX_TYPE_FO:
		boxType = TaskSearchOwner::BT_FO;
		break;
	case CBOX_TYPE_PFO:
		boxType = TaskSearchOwner::BT_PFO;
		break;
	case CBOX_TYPE_PO:
		boxType = TaskSearchOwner::BT_PO;
		break;
	case CBOX_TYPE_OVM:
	default:
		boxType = TaskSearchOwner::BT_OVM;
		break;
	}

	queryBox(this->iDLineEdit->text(), boxType, this->iCLineEdit->text(),
	    this->nameLineEdit->text(), this->pscLineEdit->text());
}

void DlgDsSearch::contactItemDoubleClicked(const QModelIndex &index)
{
	if (!index.isValid()) {
		this->close();
		return;
	}

	if (m_dbIdList != Q_NULLPTR) {
		m_dbIdList->append(this->contactTableWidget->
		    item(index.row(), CON_COL_BOX_ID)->text());
	}
	this->close();
}

void DlgDsSearch::addSelectedDbIDs(void)
{
	if (m_dbIdList == Q_NULLPTR) {
		return;
	}

	for (int i = 0; i < this->contactTableWidget->rowCount(); ++i) {
		if (this->contactTableWidget->item(i, CON_COL_CHECKBOX)->checkState()) {
			m_dbIdList->append(this->contactTableWidget->
			    item(i, CON_COL_BOX_ID)->text());
		}
	}
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
	    SLOT(addSelectedDbIDs()));
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

	m_pingTimer = new QTimer(this);
	m_pingTimer->start(DLG_ISDS_KEEPALIVE_MS);

	connect(m_pingTimer, SIGNAL(timeout()), this, SLOT(pingIsdsServer()));

	checkInputFields();
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

void DlgDsSearch::queryBox(const QString &boxId,
    enum TaskSearchOwner::BoxType boxType, const QString &ic,
    const QString &name, const QString &zipCode)
{
	this->contactTableWidget->setRowCount(0);
	this->contactTableWidget->setEnabled(false);

	TaskSearchOwner::SoughtOwnerInfo soughtInfo(boxId, boxType, ic, name,
	    name, name, zipCode);

	TaskSearchOwner *task =
	    new (std::nothrow) TaskSearchOwner(m_userName, soughtInfo);
	task->setAutoDelete(false);
	globWorkPool.runSingle(task);

	enum TaskSearchOwner::Result result = task->m_result;
	QString errMsg = task->m_isdsError;
	QString longErrMsg = task->m_isdsLongError;
	QList<TaskSearchOwner::BoxEntry> foundBoxes(task->m_foundBoxes);

	delete task; task = Q_NULLPTR;

	switch (result) {
	case TaskSearchOwner::SO_SUCCESS:
		break;
	case TaskSearchOwner::SO_BAD_DATA:
		QMessageBox::information(this, tr("Search result"),
		    longErrMsg, QMessageBox::Ok);
		return;
		break;
	case TaskSearchOwner::SO_COM_ERROR:
		QMessageBox::information(this, tr("Search result"),
		    tr("It is not possible find databox because") + ":\n\n" +
		    longErrMsg, QMessageBox::Ok);
		return;
		break;
	case TaskSearchOwner::SO_ERROR:
	default:
		QMessageBox::critical(this, tr("Search error"),
		    tr("It is not possible find databox because "
		         "error occurred during search process!"),
		    QMessageBox::Ok);
		return;
		break;
	}

	this->contactTableWidget->setRowCount(0);
	this->contactTableWidget->setEnabled(true);
	foreach (const TaskSearchOwner::BoxEntry &entry, foundBoxes) {
		int row = this->contactTableWidget->rowCount();
		this->contactTableWidget->insertRow(row);
		QTableWidgetItem *item = new QTableWidgetItem;
		item->setCheckState(Qt::Unchecked);
		this->contactTableWidget->setItem(row, CON_COL_CHECKBOX, item);
		item = new QTableWidgetItem;
		item->setText(entry.id);
		this->contactTableWidget->setItem(row, CON_COL_BOX_ID, item);
		item = new QTableWidgetItem;
		item->setText(entry.name);
		this->contactTableWidget->setItem(row, CON_COL_BOX_NAME, item);
		item = new QTableWidgetItem;
		item->setText(entry.address);
		this->contactTableWidget->setItem(row, CON_COL_ADDRESS, item);
		item = new QTableWidgetItem;
		item->setText(entry.zipCode);
		this->contactTableWidget->setItem(row, CON_COL_POST_CODE, item);
		item = new QTableWidgetItem;
		item->setText(entry.effectiveOVM ? tr("no") : tr("yes"));
		this->contactTableWidget->setItem(row, CON_COL_PDZ, item);
	}

	if (this->contactTableWidget->rowCount() > 0) {
		this->contactTableWidget->selectColumn(CON_COL_CHECKBOX);
		this->contactTableWidget->selectRow(0);
	}

	this->contactTableWidget->resizeColumnsToContents();
}
