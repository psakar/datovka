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

#define CBOX_TARGET_ALL 0
#define CBOX_TARGET_ADDRESS 1
#define CBOX_TARGET_IC 2
#define CBOX_TARGET_BOX_ID 3

#define CBOX_TYPE_ALL 0
#define CBOX_TYPE_OVM 1
#define CBOX_TYPE_PO 2
#define CBOX_TYPE_PFO 3
#define CBOX_TYPE_FO 4

DlgDsSearch::DlgDsSearch(const QString &userName, const QString &dbType,
    bool dbEffectiveOVM, bool dbOpenAddressing, QStringList *dbIdList,
    QWidget *parent)
    : QDialog(parent),
    m_userName(userName),
    m_dbType(dbType),
    m_dbEffectiveOVM(dbEffectiveOVM),
    m_dbOpenAddressing(dbOpenAddressing),
    m_contactTableModel(this),
    m_boxTypeCBoxModel(this),
    m_fulltextCBoxModel(this),
    m_dbIdList(dbIdList),
    m_pingTimer(Q_NULLPTR),
    m_showInfoLabel(false)
{
	setupUi(this);
	/* Set default line height for table views/widgets. */
	this->contactTableView->setNarrowedLineHeight();
	this->contactTableView->setSelectionBehavior(
	    QAbstractItemView::SelectRows);

	m_boxTypeCBoxModel.appendRow(
	    tr("All") + QStringLiteral(" - ") + tr("All types"),
	    CBOX_TYPE_ALL);
	m_boxTypeCBoxModel.appendRow(
	    tr("OVM") + QStringLiteral(" - ") + tr("Orgán veřejné moci"),
	    CBOX_TYPE_OVM);
	m_boxTypeCBoxModel.appendRow(
	    tr("PO") + QStringLiteral(" - ") + tr("Právnická osoba"),
	    CBOX_TYPE_PO);
	m_boxTypeCBoxModel.appendRow(
	    tr("PFO") + QStringLiteral(" - ") + tr("Podnikající fyzická osoba"),
	    CBOX_TYPE_PFO);
	m_boxTypeCBoxModel.appendRow(
	    tr("FO") + QStringLiteral(" - ") + tr("Fyzická osoba"),
	    CBOX_TYPE_FO);
	this->dataBoxTypeCBox->setModel(&m_boxTypeCBoxModel);

	m_fulltextCBoxModel.appendRow(
	    tr("All") + QStringLiteral(" - ") + tr("Search in all fields"),
	    CBOX_TARGET_ALL);
	m_fulltextCBoxModel.appendRow(
	    tr("Address") + QStringLiteral(" - ") + tr("Search in address data"),
	    CBOX_TARGET_ADDRESS);
	m_fulltextCBoxModel.appendRow(
	    tr("IC") + QStringLiteral(" - ") + tr("Identification number"),
	    CBOX_TARGET_IC);
	m_fulltextCBoxModel.appendRow(
	    tr("ID") + QStringLiteral(" - ") + tr("Box identifier"),
	    CBOX_TARGET_BOX_ID);
	this->fulltextTargetCBox->setModel(&m_fulltextCBoxModel);

	m_contactTableModel.setHeader();
	this->contactTableView->setModel(&m_contactTableModel);

	connect(this->contactTableView->selectionModel(),
	    SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
	    this, SLOT(setFirstColumnActive(QItemSelection, QItemSelection)));

	connect(this->textLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
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
	connect(&m_contactTableModel,
	    SIGNAL(dataChanged(QModelIndex, QModelIndex)),
	    this, SLOT(enableOkButton()));
	connect(this->contactTableView, SIGNAL(doubleClicked(QModelIndex)),
	    this, SLOT(contactItemDoubleClicked(QModelIndex)));
	connect(this->buttonBox, SIGNAL(accepted()),
	    this, SLOT(addSelectedDbIDs()));
	connect(this->searchPushButton, SIGNAL(clicked()),
	    this, SLOT(searchDataBox()));

	connect(this->useFulltextCheckBox, SIGNAL(stateChanged(int)),
	    this, SLOT(makeSearchElelementsVisible(int)));
	this->useFulltextCheckBox->setCheckState(Qt::Checked);

	this->contactTableView->installEventFilter(
	    new TableHomeEndFilter(this));
	this->contactTableView->installEventFilter(
	    new TableSpaceSelectionFilter(this));

#if 0 /* Don't use the timer. */
	m_pingTimer = new QTimer(this);
	m_pingTimer->start(DLG_ISDS_KEEPALIVE_MS);

	connect(m_pingTimer, SIGNAL(timeout()), this, SLOT(pingIsdsServer()));
#endif

	initContent();
}

DlgDsSearch::~DlgDsSearch(void)
{
	if (m_pingTimer != Q_NULLPTR) {
		delete m_pingTimer;
	}
}

void DlgDsSearch::enableOkButton(void)
{
	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
	    m_contactTableModel.somethingChecked());
}

void DlgDsSearch::setFirstColumnActive(const QItemSelection &selected,
    const QItemSelection &deselected)
{
	Q_UNUSED(deselected);

	if (selected.isEmpty()) {
		return;
	}
	this->contactTableView->selectColumn(BoxContactsModel::CHECKBOX_COL);
	this->contactTableView->selectRow(selected.first().top());
}

void DlgDsSearch::checkInputFields(void)
{
	if (Qt::Checked == this->useFulltextCheckBox->checkState()) {
		checkInputFieldsFulltext();
	} else {
		checkInputFieldsNormal();
	}
}

void DlgDsSearch::searchDataBox(void)
{
	if (Qt::Checked == this->useFulltextCheckBox->checkState()) {
		searchDataBoxFulltext();
	} else {
		searchDataBoxNormal();
	}
}

void DlgDsSearch::contactItemDoubleClicked(const QModelIndex &index)
{
	if (index.isValid() && (m_dbIdList != Q_NULLPTR)) {
		m_dbIdList->append(index.sibling(index.row(),
		    BoxContactsModel::BOX_ID_COL).data(
		        Qt::DisplayRole).toString());
	}
	this->close();
}

void DlgDsSearch::addSelectedDbIDs(void) const
{
	if (m_dbIdList != Q_NULLPTR) {
		m_dbIdList->append(m_contactTableModel.checkedBoxIds());
	}
}

void DlgDsSearch::pingIsdsServer(void)
{
	if (globIsdsSessions.isConnectedToIsds(m_userName)) {
		qDebug() << "Connection to ISDS is alive :)";
	} else {
		qDebug() << "Connection to ISDS is dead :(";
	}
}

void DlgDsSearch::makeSearchElelementsVisible(int fulltextState)
{
	labelSearchInfo->hide();

	fulltextTargetLabel->hide();
	fulltextTargetCBox->hide();

	iDLineLabel->hide();
	iDLineEdit->clear();
	iDLineEdit->hide();

	iCLineLabel->hide();
	iCLineEdit->clear();
	iCLineEdit->hide();

	nameLineLabel->hide();
	nameLineEdit->clear();
	nameLineEdit->hide();

	pscLineLabel->hide();
	pscLineEdit->clear();
	pscLineEdit->hide();

	textLineLabel->hide();
	textLineEdit->clear();
	textLineEdit->hide();

	m_contactTableModel.removeRows(0, m_contactTableModel.rowCount());

	if (Qt::Checked == fulltextState) {
		labelSearchDescr->setText(
		    tr("Full-text data box search. Enter phrase for finding and set optional restrictions:"));

		m_boxTypeCBoxModel.setEnabled(CBOX_TYPE_ALL, true);

		fulltextTargetLabel->show();
		fulltextTargetCBox->show();

		textLineLabel->show();
		textLineEdit->show();
	} else {
		labelSearchDescr->setText(
		    tr("Enter the ID, IČ or at least three letters from the name of the data box you look for:"));

		/* Cannot search for all data box types. */
		m_boxTypeCBoxModel.setEnabled(CBOX_TYPE_ALL, false);
		if (dataBoxTypeCBox->currentData(CBoxModel::valueRole).toInt() ==
		    CBOX_TYPE_ALL) {
			dataBoxTypeCBox->setCurrentIndex(
			    m_boxTypeCBoxModel.findRow(CBOX_TYPE_OVM));
		}

		iDLineLabel->show();
		iDLineEdit->show();

		iCLineLabel->show();
		iCLineEdit->show();

		nameLineLabel->show();
		nameLineEdit->show();

		pscLineLabel->show();
		pscLineEdit->show();
	}
}

void DlgDsSearch::initContent(void)
{
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

	this->labelSearchInfo->setStyleSheet("QLabel { color: red }");
	this->labelSearchInfo->setToolTip(toolTipInfo);
	this->labelSearchInfo->hide();

	this->contactTableView->setColumnWidth(BoxContactsModel::CHECKBOX_COL, 20);
	this->contactTableView->setColumnWidth(BoxContactsModel::BOX_ID_COL, 60);
	this->contactTableView->setColumnWidth(BoxContactsModel::BOX_TYPE_COL, 70);
	this->contactTableView->setColumnWidth(BoxContactsModel::BOX_NAME_COL, 120);
	this->contactTableView->setColumnWidth(BoxContactsModel::ADDRESS_COL, 100);

	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	this->contactTableView->
	    setEditTriggers(QAbstractItemView::NoEditTriggers);

	checkInputFields();
}

void DlgDsSearch::checkInputFieldsNormal(void)
{
	this->nameLineEdit->setEnabled(true);
	this->pscLineEdit->setEnabled((true));
	this->iCLineEdit->setEnabled((true));
	this->iDLineEdit->setEnabled((true));

	switch (this->dataBoxTypeCBox->currentData(CBoxModel::valueRole).toInt()) {
	case CBOX_TYPE_OVM:
		this->iCLineEdit->setEnabled(true);
		this->nameLineLabel->setText(tr("Subject Name:"));
		this->nameLineLabel->setToolTip(tr("Enter name of subject"));
		this->nameLineEdit->setToolTip(tr("Enter name of subject"));
		this->labelSearchInfo->hide();
		break;
	case CBOX_TYPE_PO:
		this->iCLineEdit->setEnabled(true);
		this->nameLineLabel->setText(tr("Subject Name:"));
		this->nameLineLabel->setToolTip(tr("Enter name of subject"));
		this->nameLineEdit->setToolTip(tr("Enter name of subject"));
		this->labelSearchInfo->setVisible(m_showInfoLabel);
		break;
	case CBOX_TYPE_PFO:
		this->iCLineEdit->setEnabled(true);
		this->nameLineLabel->setText(tr("Name:"));
		this->nameLineLabel->setToolTip(tr("Enter last name of the PFO or company name."));
		this->nameLineEdit->setToolTip(tr("Enter last name of the PFO or company name."));
		this->labelSearchInfo->setVisible(m_showInfoLabel);
		break;
	case CBOX_TYPE_FO:
		this->iCLineEdit->setEnabled(false);
		this->nameLineLabel->setText(tr("Last Name:"));
		this->nameLineLabel->setToolTip(tr("Enter last name or last name at birth of the FO."));
		this->nameLineEdit->setToolTip(tr("Enter last name or last name at birth of the FO."));
		this->labelSearchInfo->setVisible(m_showInfoLabel);
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

void DlgDsSearch::checkInputFieldsFulltext(void)
{
	this->searchPushButton->setEnabled(
	    this->textLineEdit->text().size() > 2);
}

void DlgDsSearch::searchDataBoxNormal(void)
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
	switch (this->dataBoxTypeCBox->currentData(CBoxModel::valueRole).toInt()) {
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

	this->contactTableView->setColumnHidden(BoxContactsModel::POST_CODE_COL,
	    false);
	this->contactTableView->setColumnHidden(BoxContactsModel::PDZ_COL,
	    true);

	queryBoxNormal(this->iDLineEdit->text(), boxType,
	    this->iCLineEdit->text(), this->nameLineEdit->text(),
	    this->pscLineEdit->text());
}

void DlgDsSearch::searchDataBoxFulltext(void)
{
	enum TaskSearchOwnerFulltext::FulltextTarget target =
	    TaskSearchOwnerFulltext::FT_ALL;
	switch (this->fulltextTargetCBox->currentData(CBoxModel::valueRole).toInt()) {
	case CBOX_TARGET_ALL:
		target = TaskSearchOwnerFulltext::FT_ALL;
		break;
	case CBOX_TARGET_ADDRESS:
		target = TaskSearchOwnerFulltext::FT_ADDRESS;
		break;
	case CBOX_TARGET_IC:
		target = TaskSearchOwnerFulltext::FT_IC;
		break;
	case CBOX_TARGET_BOX_ID:
	default:
		target = TaskSearchOwnerFulltext::FT_BOX_ID;
		break;
	}

	enum TaskSearchOwnerFulltext::BoxType boxType =
	    TaskSearchOwnerFulltext::BT_ALL;
	switch (this->dataBoxTypeCBox->currentData(CBoxModel::valueRole).toInt()) {
	case CBOX_TYPE_FO:
		boxType = TaskSearchOwnerFulltext::BT_FO;
		break;
	case CBOX_TYPE_PFO:
		boxType = TaskSearchOwnerFulltext::BT_PFO;
		break;
	case CBOX_TYPE_PO:
		boxType = TaskSearchOwnerFulltext::BT_PO;
		break;
	case CBOX_TYPE_OVM:
		boxType = TaskSearchOwnerFulltext::BT_OVM;
		break;
	case CBOX_TYPE_ALL:
	default:
		boxType = TaskSearchOwnerFulltext::BT_ALL;
		break;
	}

	this->contactTableView->setColumnHidden(BoxContactsModel::POST_CODE_COL,
	    true);
	this->contactTableView->setColumnHidden(BoxContactsModel::PDZ_COL,
	    true);

	queryBoxFulltext(target, boxType, this->textLineEdit->text());
}

void DlgDsSearch::queryBoxNormal(const QString &boxId,
    enum TaskSearchOwner::BoxType boxType, const QString &ic,
    const QString &name, const QString &zipCode)
{
	this->contactTableView->setEnabled(false);
	m_contactTableModel.removeRows(0, m_contactTableModel.rowCount());

	QString resultString(tr("Total found") + QStringLiteral(": "));
	this->searchResultText->setText(resultString + QStringLiteral("0"));

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
		    tr("It was not possible find any data box because") +
		    QStringLiteral(":\n\n") + longErrMsg,
		    QMessageBox::Ok);
		return;
		break;
	case TaskSearchOwner::SO_ERROR:
	default:
		QMessageBox::critical(this, tr("Search error"),
		    tr("It was not possible find any data box because an error occurred during the search process!"),
		    QMessageBox::Ok);
		return;
		break;
	}

	this->searchResultText->setText(
	    resultString + QString::number(foundBoxes.size()));

	this->contactTableView->setEnabled(true);
	m_contactTableModel.appendData(foundBoxes);

	if (m_contactTableModel.rowCount() > 0) {
		this->contactTableView->selectColumn(
		    BoxContactsModel::CHECKBOX_COL);
		this->contactTableView->selectRow(0);
	}

	//this->contactTableView->resizeColumnsToContents();
}

void DlgDsSearch::queryBoxFulltext(
    enum TaskSearchOwnerFulltext::FulltextTarget target,
    enum TaskSearchOwnerFulltext::BoxType boxType, const QString &phrase)
{
	this->contactTableView->setEnabled(false);
	m_contactTableModel.removeRows(0, m_contactTableModel.rowCount());

	QString resultString(tr("Total found") + QStringLiteral(": "));
	this->searchResultText->setText(resultString + QStringLiteral("0"));

	TaskSearchOwnerFulltext *task =
	    new (std::nothrow) TaskSearchOwnerFulltext(m_userName, phrase,
	        target, boxType);
	task->setAutoDelete(false);
	globWorkPool.runSingle(task);

	enum TaskSearchOwnerFulltext::Result result = task->m_result;
	QString errMsg = task->m_isdsError;
	QString longErrMsg = task->m_isdsLongError;
	QList<TaskSearchOwnerFulltext::BoxEntry> foundBoxes(task->m_foundBoxes);
	quint64 totalDb = task->m_totalMatchingBoxes;

	delete task; task = Q_NULLPTR;

	switch (result) {
	case TaskSearchOwnerFulltext::SOF_SUCCESS:
		break;
	case TaskSearchOwnerFulltext::SOF_BAD_DATA:
		QMessageBox::information(this, tr("Search result"),
		    longErrMsg, QMessageBox::Ok);
		return;
		break;
	case TaskSearchOwnerFulltext::SOF_COM_ERROR:
		QMessageBox::information(this, tr("Search result"),
		    tr("It was not possible find any data box because") +
		    QStringLiteral(":\n\n") + longErrMsg,
		    QMessageBox::Ok);
		return;
		break;
	case TaskSearchOwnerFulltext::SOF_ERROR:
	default:
		QMessageBox::critical(this, tr("Search error"),
		    tr("It was not possible find any data box because an error occurred during the search process!"),
		    QMessageBox::Ok);
		return;
		break;
	}

	this->searchResultText->setText(
	    resultString + QString::number(totalDb));

	this->contactTableView->setEnabled(true);
	m_contactTableModel.appendData(foundBoxes);

	if (m_contactTableModel.rowCount() > 0) {
		this->contactTableView->selectColumn(
		    BoxContactsModel::CHECKBOX_COL);
		this->contactTableView->selectRow(0);
	}

	//this->contactTableView->resizeColumnsToContents();
}
