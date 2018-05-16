/*
 * Copyright (C) 2014-2018 CZ.NIC
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

#include <QCoreApplication>
#include <QMessageBox>

#include "src/datovka_shared/worker/pool.h"
#include "src/global.h"
#include "src/gui/dlg_ds_search.h"
#include "src/io/isds_sessions.h"
#include "src/views/table_home_end_filter.h"
#include "src/views/table_space_selection_filter.h"
#include "src/views/table_tab_ignore_filter.h"
#include "ui_dlg_ds_search.h"

#define CBOX_TARGET_ALL 0
#define CBOX_TARGET_ADDRESS 1
#define CBOX_TARGET_IC 2
#define CBOX_TARGET_BOX_ID 3

#define CBOX_TYPE_ALL 0
#define CBOX_TYPE_OVM 1
#define CBOX_TYPE_PO 2
#define CBOX_TYPE_PFO 3
#define CBOX_TYPE_FO 4

FulltextSearchThread::FulltextSearchThread(DlgDsSearch *dlg)
    : m_dlg(dlg)
{
	Q_ASSERT(dlg != Q_NULLPTR);
}

void FulltextSearchThread::run(void)
{
	if (Q_UNLIKELY(m_dlg == Q_NULLPTR)) {
		Q_ASSERT(0);
		return;
	}
	DlgDsSearch::SearchResultFt searchResult(m_dlg->searchDataBoxFulltext());

	emit searchFinished(searchResult.result, searchResult.longErrMsg);
}

DlgDsSearch::DlgDsSearch(const QString &userName, const QString &dbType,
    bool dbEffectiveOVM, bool dbOpenAddressing, QStringList *dbIdList,
    QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgDsSearch),
    m_fulltextThread(this),
    m_userName(userName),
    m_dbType(dbType),
    m_dbEffectiveOVM(dbEffectiveOVM),
    m_dbOpenAddressing(dbOpenAddressing),
    m_contactListProxyModel(this),
    m_contactTableModel(this),
    m_boxTypeCBoxModel(this),
    m_fulltextCBoxModel(this),
    m_dbIdList(dbIdList),
    m_breakDownloadLoop(false),
    m_showInfoLabel(false)
{
	m_ui->setupUi(this);

	/* Set default line height for table views/widgets. */
	m_ui->contactTableView->setNarrowedLineHeight();
	m_ui->contactTableView->setSelectionMode(
	    QAbstractItemView::ExtendedSelection);
	m_ui->contactTableView->setSelectionBehavior(
	    QAbstractItemView::SelectRows);

	m_boxTypeCBoxModel.appendRow(
	    tr("All") + QStringLiteral(" - ") + tr("All types"),
	    CBOX_TYPE_ALL);
	m_boxTypeCBoxModel.appendRow(
	    tr("OVM") + QStringLiteral(" - ") + tr("Public authority"), /* organ verejne moci */
	    CBOX_TYPE_OVM);
	m_boxTypeCBoxModel.appendRow(
	    tr("PO") + QStringLiteral(" - ") + tr("Legal person"), /* pravnicka osoba */
	    CBOX_TYPE_PO);
	m_boxTypeCBoxModel.appendRow(
	    tr("PFO") + QStringLiteral(" - ") + tr("Self-employed person"), /* podnikajici fyzicka osoba */
	    CBOX_TYPE_PFO);
	m_boxTypeCBoxModel.appendRow(
	    tr("FO") + QStringLiteral(" - ") + tr("Natural person"), /* fyzicka osoba */
	    CBOX_TYPE_FO);
	m_ui->dataBoxTypeCBox->setModel(&m_boxTypeCBoxModel);

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
	m_ui->fulltextTargetCBox->setModel(&m_fulltextCBoxModel);

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

	connect(m_ui->contactTableView->selectionModel(),
	    SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
	    this, SLOT(setFirstColumnActive(QItemSelection, QItemSelection)));

	connect(m_ui->textLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(m_ui->iDLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(m_ui->iCLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(m_ui->nameLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(m_ui->pscLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(m_ui->dataBoxTypeCBox, SIGNAL(currentIndexChanged(int)),
	    this, SLOT(checkInputFields()));
	connect(m_ui->fulltextTargetCBox, SIGNAL(currentIndexChanged(int)),
	    this, SLOT(checkInputFields()));
	connect(&m_contactTableModel,
	    SIGNAL(dataChanged(QModelIndex, QModelIndex)),
	    this, SLOT(enableOkButton()));
	connect(m_ui->contactTableView, SIGNAL(doubleClicked(QModelIndex)),
	    this, SLOT(contactItemDoubleClicked(QModelIndex)));
	connect(m_ui->buttonBox, SIGNAL(accepted()),
	    this, SLOT(addSelectedDbIDs()));
	connect(m_ui->buttonBox, SIGNAL(rejected()),
	    this, SLOT(setBreakDownloadLoop()));
	connect(m_ui->searchPushButton, SIGNAL(clicked()),
	    this, SLOT(searchDataBox()));

	connect(&m_fulltextThread, SIGNAL(searchFinished(int, QString)),
	    this, SLOT(collectFulltextThreadOutcome(int, QString)));

	connect(m_ui->useFulltextCheckBox, SIGNAL(stateChanged(int)),
	    this, SLOT(makeSearchElelementsVisible(int)));
	m_ui->useFulltextCheckBox->setCheckState(Qt::Checked);

	m_ui->contactTableView->installEventFilter(
	    new TableHomeEndFilter(m_ui->contactTableView));
	m_ui->contactTableView->installEventFilter(
	    new TableSpaceSelectionFilter(
	        BoxContactsModel::CHECKBOX_COL, m_ui->contactTableView));
	m_ui->contactTableView->installEventFilter(
	    new TableTabIgnoreFilter(m_ui->contactTableView));

	m_ui->filterLine->setFixedWidth(200);
	m_ui->filterLine->setToolTip(tr("Enter sought expression"));
	m_ui->filterLine->setClearButtonEnabled(true);

	connect(m_ui->filterLine, SIGNAL(textChanged(QString)),
	    this, SLOT(filterContact(QString)));

	initContent();
}

DlgDsSearch::~DlgDsSearch(void)
{
	m_fulltextThread.wait();
	delete m_ui;
}

QStringList DlgDsSearch::search(const QString &userName, const QString &dbType,
    bool dbEffectiveOVM, bool dbOpenAddressing, QWidget *parent)
{
	QStringList dbIdList;

	DlgDsSearch dlg(userName, dbType, dbEffectiveOVM, dbOpenAddressing,
	    &dbIdList, parent);
	dlg.exec();

	return dbIdList;
}

void DlgDsSearch::enableOkButton(void)
{
	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
	    m_contactTableModel.somethingChecked());
}

void DlgDsSearch::setFirstColumnActive(const QItemSelection &selected,
    const QItemSelection &deselected)
{
	Q_UNUSED(deselected);

	if (selected.isEmpty()) {
		return;
	}
	m_ui->contactTableView->selectColumn(BoxContactsModel::CHECKBOX_COL);
	m_ui->contactTableView->selectRow(selected.first().top());
}

void DlgDsSearch::checkInputFields(void)
{
	if (Qt::Checked == m_ui->useFulltextCheckBox->checkState()) {
		checkInputFieldsFulltext();
	} else {
		checkInputFieldsNormal();
	}
}

/*!
 * @brief Enables/disables search controls.
 *
 * @param[in,out] ui User interface.
 * @param[in]     enabled State to be set.
 */
static inline
void setSearchControlsEnabled(Ui::DlgDsSearch *ui, bool enabled)
{
	if (Q_UNLIKELY(Q_NULLPTR == ui)) {
		Q_ASSERT(0);
		return;
	}
	ui->useFulltextCheckBox->setEnabled(enabled);
	ui->searchPushButton->setEnabled(enabled);
}

void DlgDsSearch::collectFulltextThreadOutcome(int result,
    const QString &longErrMsg)
{
	displaySearchResultFt(result, longErrMsg);

	setSearchControlsEnabled(m_ui, true);
}

void DlgDsSearch::searchDataBox(void)
{
	setSearchControlsEnabled(m_ui, false);
	QCoreApplication::processEvents();
	if (Qt::Checked == m_ui->useFulltextCheckBox->checkState()) {
#if 0
		displaySearchResultFt(searchDataBoxFulltext());
		setSearchControlsEnabled(m_ui, true);
#else
		/* Signal stop to any possibly running thread. */
		setBreakDownloadLoop();
		/* Wait for thread to finish. */
		m_fulltextThread.wait();
		/* Just in case the thread was not running. */
		m_breakDownloadLoop = false;
		/* Execute search thread. */
		searchDataBoxFulltextThread();
#endif
	} else {
		if (m_ui->iDLineEdit->text() == ID_ISDS_SYS_DATABOX) {
			QMessageBox::information(this, tr("Search result"),
			    tr("This is a special ID for a ISDS system data box. "
			        "You can't use this ID for message delivery. Try again."),
			    QMessageBox::Ok);
		} else {
			displaySearchResult(searchDataBoxNormal());
		}
		setSearchControlsEnabled(m_ui, true);
	}
}

void DlgDsSearch::contactItemDoubleClicked(const QModelIndex &index)
{
	setBreakDownloadLoop(); /* Break download loop. */

	if (index.isValid() && (m_dbIdList != Q_NULLPTR)) {
		m_dbIdList->append(index.sibling(index.row(),
		    BoxContactsModel::BOX_ID_COL).data(
		        Qt::DisplayRole).toString());
	}
	this->close();
}

void DlgDsSearch::addSelectedDbIDs(void)
{
	setBreakDownloadLoop(); /* Break download loop. */

	if (m_dbIdList != Q_NULLPTR) {
		m_dbIdList->append(m_contactTableModel.boxIdentifiers(
		    BoxContactsModel::CHECKED));
	}
}

void DlgDsSearch::setBreakDownloadLoop(void)
{
	m_breakDownloadLoop = true;
}

void DlgDsSearch::makeSearchElelementsVisible(int fulltextState)
{
	m_ui->labelSearchInfo->hide();

	m_ui->fulltextTargetLabel->hide();
	m_ui->fulltextTargetCBox->hide();

	m_ui->iDLineLabel->hide();
	m_ui->iDLineEdit->clear();
	m_ui->iDLineEdit->hide();

	m_ui->iCLineLabel->hide();
	m_ui->iCLineEdit->clear();
	m_ui->iCLineEdit->hide();

	m_ui->nameLineLabel->hide();
	m_ui->nameLineEdit->clear();
	m_ui->nameLineEdit->hide();

	m_ui->pscLineLabel->hide();
	m_ui->pscLineEdit->clear();
	m_ui->pscLineEdit->hide();

	m_ui->textLineLabel->hide();
	m_ui->textLineEdit->clear();
	m_ui->textLineEdit->hide();

	m_ui->searchResultText->setText(QString());

	m_contactTableModel.removeRows(0, m_contactTableModel.rowCount());

	if (Qt::Checked == fulltextState) {
		m_ui->labelSearchDescr->setText(
		    tr("Full-text data box search. Enter phrase for finding and set optional restrictions:"));

		m_boxTypeCBoxModel.setEnabled(CBOX_TYPE_ALL, true);

		m_ui->contactTableView->setColumnHidden(
		    BoxContactsModel::POST_CODE_COL, true);
		m_ui->contactTableView->setColumnHidden(BoxContactsModel::PDZ_COL,
		    true);

		m_ui->fulltextTargetLabel->show();
		m_ui->fulltextTargetCBox->show();

		m_ui->textLineLabel->show();
		m_ui->textLineEdit->show();
	} else {
		m_ui->labelSearchDescr->setText(
		    tr("Enter the ID, IČ or at least three letters from the name of the data box you look for:"));

		/* Cannot search for all data box types. */
		m_boxTypeCBoxModel.setEnabled(CBOX_TYPE_ALL, false);
		if (m_ui->dataBoxTypeCBox->currentData(CBoxModel::valueRole).toInt() ==
		    CBOX_TYPE_ALL) {
			m_ui->dataBoxTypeCBox->setCurrentIndex(
			    m_boxTypeCBoxModel.findRow(CBOX_TYPE_OVM));
		}

		m_ui->contactTableView->setColumnHidden(
		    BoxContactsModel::POST_CODE_COL, false);
		m_ui->contactTableView->setColumnHidden(BoxContactsModel::PDZ_COL,
		    true);

		m_ui->iDLineLabel->show();
		m_ui->iDLineEdit->show();

		m_ui->iCLineLabel->show();
		m_ui->iCLineEdit->show();

		m_ui->nameLineLabel->show();
		m_ui->nameLineEdit->show();

		m_ui->pscLineLabel->show();
		m_ui->pscLineEdit->show();
	}
}

void DlgDsSearch::filterContact(const QString &text)
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

	m_ui->accountInfo->setText("<strong>" + m_userName + "</strong>" +
	    " - " + m_dbType + dbOpenAddressing);

	m_ui->labelSearchInfo->setStyleSheet("QLabel { color: red }");
	m_ui->labelSearchInfo->setToolTip(toolTipInfo);
	m_ui->labelSearchInfo->hide();

	m_ui->contactTableView->setColumnWidth(BoxContactsModel::CHECKBOX_COL, 20);
	m_ui->contactTableView->setColumnWidth(BoxContactsModel::BOX_ID_COL, 60);
	m_ui->contactTableView->setColumnWidth(BoxContactsModel::BOX_TYPE_COL, 70);
	m_ui->contactTableView->setColumnWidth(BoxContactsModel::BOX_NAME_COL, 120);
	m_ui->contactTableView->setColumnWidth(BoxContactsModel::ADDRESS_COL, 100);

	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	m_ui->contactTableView->setEditTriggers(
	    QAbstractItemView::NoEditTriggers);

	checkInputFields();
}

void DlgDsSearch::checkInputFieldsNormal(void)
{
	m_ui->nameLineEdit->setEnabled(true);
	m_ui->pscLineEdit->setEnabled((true));
	m_ui->iCLineEdit->setEnabled((true));
	m_ui->iDLineEdit->setEnabled((true));

	switch (m_ui->dataBoxTypeCBox->currentData(CBoxModel::valueRole).toInt()) {
	case CBOX_TYPE_OVM:
		m_ui->iCLineEdit->setEnabled(true);
		m_ui->nameLineLabel->setText(tr("Subject Name:"));
		m_ui->nameLineLabel->setToolTip(tr("Enter name of subject"));
		m_ui->nameLineEdit->setToolTip(tr("Enter name of subject"));
		m_ui->labelSearchInfo->hide();
		break;
	case CBOX_TYPE_PO:
		m_ui->iCLineEdit->setEnabled(true);
		m_ui->nameLineLabel->setText(tr("Subject Name:"));
		m_ui->nameLineLabel->setToolTip(tr("Enter name of subject"));
		m_ui->nameLineEdit->setToolTip(tr("Enter name of subject"));
		m_ui->labelSearchInfo->setVisible(m_showInfoLabel);
		break;
	case CBOX_TYPE_PFO:
		m_ui->iCLineEdit->setEnabled(true);
		m_ui->nameLineLabel->setText(tr("Name:"));
		m_ui->nameLineLabel->setToolTip(tr("Enter last name of the PFO or company name."));
		m_ui->nameLineEdit->setToolTip(tr("Enter last name of the PFO or company name."));
		m_ui->labelSearchInfo->setVisible(m_showInfoLabel);
		break;
	case CBOX_TYPE_FO:
		m_ui->iCLineEdit->setEnabled(false);
		m_ui->nameLineLabel->setText(tr("Last Name:"));
		m_ui->nameLineLabel->setToolTip(tr("Enter last name or last name at birth of the FO."));
		m_ui->nameLineEdit->setToolTip(tr("Enter last name or last name at birth of the FO."));
		m_ui->labelSearchInfo->setVisible(m_showInfoLabel);
		break;
	default:
		break;
	}

	if (!m_ui->iDLineEdit->text().isEmpty()) {
		m_ui->nameLineEdit->setEnabled(false);
		m_ui->pscLineEdit->setEnabled(false);
		m_ui->iCLineEdit->setEnabled(false);

		if (m_ui->iDLineEdit->text().length() == 7) {
			m_ui->searchPushButton->setEnabled(true);
		}
		else {
			m_ui->searchPushButton->setEnabled(false);
		}
	} else if (!m_ui->iCLineEdit->text().isEmpty()) {
		m_ui->iDLineEdit->setEnabled(false);
		m_ui->nameLineEdit->setEnabled(false);
		m_ui->pscLineEdit->setEnabled(false);

		if (m_ui->iCLineEdit->text().length() == 8) {
			m_ui->searchPushButton->setEnabled(true);
		}
		else {
			m_ui->searchPushButton->setEnabled(false);
		}
	} else if (!m_ui->nameLineEdit->text().isEmpty()) {
		m_ui->iDLineEdit->setEnabled(false);

		if (m_ui->nameLineEdit->text().length() > 2) {
			m_ui->searchPushButton->setEnabled(true);
		}
		else {
			m_ui->searchPushButton->setEnabled(false);
		}
	} else {
		m_ui->searchPushButton->setEnabled(false);
		m_ui->iDLineEdit->setEnabled(true);
		m_ui->nameLineEdit->setEnabled(true);
		m_ui->pscLineEdit->setEnabled(true);
	}
}

void DlgDsSearch::checkInputFieldsFulltext(void)
{
	m_ui->searchPushButton->setEnabled(
	    m_ui->textLineEdit->text().size() > 2);
}

DlgDsSearch::SearchResult DlgDsSearch::searchDataBoxNormal(void)
{
	enum Isds::Type::DbType boxType = Isds::Type::BT_OVM;
	switch (m_ui->dataBoxTypeCBox->currentData(CBoxModel::valueRole).toInt()) {
	case CBOX_TYPE_FO:
		boxType = Isds::Type::BT_FO;
		break;
	case CBOX_TYPE_PFO:
		boxType = Isds::Type::BT_PFO;
		break;
	case CBOX_TYPE_PO:
		boxType = Isds::Type::BT_PO;
		break;
	case CBOX_TYPE_OVM:
	default:
		boxType = Isds::Type::BT_OVM;
		break;
	}

	m_ui->contactTableView->setEnabled(false);
	m_contactTableModel.removeRows(0, m_contactTableModel.rowCount());

	m_ui->searchResultText->setText(totalFoundStr(0));

	Isds::Address address;
	Isds::DbOwnerInfo dbOwnerInfo;
	Isds::PersonName personName;

	dbOwnerInfo.setDbID(m_ui->iDLineEdit->text());
	dbOwnerInfo.setDbType(boxType);
	dbOwnerInfo.setIc(m_ui->iCLineEdit->text());
	personName.setFirstName(m_ui->nameLineEdit->text());
	personName.setLastName(m_ui->nameLineEdit->text());
	dbOwnerInfo.setPersonName(personName);
	dbOwnerInfo.setFirmName(m_ui->nameLineEdit->text());
	address.setZipCode(m_ui->pscLineEdit->text());
	dbOwnerInfo.setAddress(address);

	TaskSearchOwner *task = new (std::nothrow) TaskSearchOwner(m_userName,
	    dbOwnerInfo);
	if (Q_UNLIKELY(task == Q_NULLPTR)) {
		Q_ASSERT(0);
		m_ui->contactTableView->setEnabled(true);
		return SearchResult(TaskSearchOwner::SO_ERROR, QString());
	}
	task->setAutoDelete(false);
	GlobInstcs::workPoolPtr->runSingle(task);

	enum TaskSearchOwner::Result taskResult = task->m_result;
	QString longErrMsg(task->m_isdsLongError);
	QList<Isds::DbOwnerInfo> foundBoxes(task->m_foundBoxes);

	delete task; task = Q_NULLPTR;

	if (taskResult != TaskSearchOwner::SO_SUCCESS) {
		return SearchResult(taskResult, longErrMsg);
	}

	bool selectFirstRow = m_contactTableModel.rowCount() == 0;
	m_contactTableModel.appendData(foundBoxes);

	m_ui->searchResultText->setText(totalFoundStr(foundBoxes.size()));

	if (m_contactTableModel.rowCount() > 0) {
		m_ui->contactTableView->selectColumn(
		    BoxContactsModel::CHECKBOX_COL);
		if (selectFirstRow) {
			m_ui->contactTableView->selectRow(0);
		}
	}

	m_ui->contactTableView->setEnabled(true);

	//m_ui->contactTableView->resizeColumnsToContents();

	return SearchResult(taskResult, QString());
}

DlgDsSearch::SearchResultFt DlgDsSearch::searchDataBoxFulltext(void)
{
	enum TaskSearchOwnerFulltext::FulltextTarget target =
	    TaskSearchOwnerFulltext::FT_ALL;
	switch (m_ui->fulltextTargetCBox->currentData(CBoxModel::valueRole).toInt()) {
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
	switch (m_ui->dataBoxTypeCBox->currentData(CBoxModel::valueRole).toInt()) {
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

	return queryBoxFulltextAll(target, boxType, m_ui->textLineEdit->text());
}

void DlgDsSearch::searchDataBoxFulltextThread(void)
{
	m_fulltextThread.start();
}

void DlgDsSearch::displaySearchResult(const SearchResult &searchResult)
{
	switch (searchResult.result) {
	case TaskSearchOwner::SO_SUCCESS:
		/* Don't display anything on success. */
		break;
	case TaskSearchOwner::SO_BAD_DATA:
		QMessageBox::information(this, tr("Search result"),
		    searchResult.longErrMsg, QMessageBox::Ok);
		m_ui->contactTableView->setEnabled(true);
		return;
		break;
	case TaskSearchOwner::SO_COM_ERROR:
		QMessageBox::information(this, tr("Search result"),
		    tr("It was not possible find any data box because") +
		    QStringLiteral(":\n\n") + searchResult.longErrMsg,
		    QMessageBox::Ok);
		m_ui->contactTableView->setEnabled(true);
		return;
		break;
	case TaskSearchOwner::SO_ERROR:
	default:
		QMessageBox::critical(this, tr("Search error"),
		    tr("It was not possible find any data box because an error occurred during the search process!"),
		    QMessageBox::Ok);
		m_ui->contactTableView->setEnabled(true);
		return;
		break;
	}
}

DlgDsSearch::SearchResultFt DlgDsSearch::queryBoxFulltextPage(
    enum TaskSearchOwnerFulltext::FulltextTarget target,
    enum TaskSearchOwnerFulltext::BoxType boxType,
    const QString &phrase, qint64 pageNum)
{
	TaskSearchOwnerFulltext *task =
	    new (std::nothrow) TaskSearchOwnerFulltext(m_userName, phrase,
	        target, boxType, pageNum, false);
	if (Q_UNLIKELY(task == Q_NULLPTR)) {
		Q_ASSERT(0);
		return SearchResultFt(TaskSearchOwnerFulltext::SOF_ERROR,
		    QString(), false);
	}
	task->setAutoDelete(false);
	GlobInstcs::workPoolPtr->runSingle(task);

	enum TaskSearchOwnerFulltext::Result taskResult = task->m_result;
	QString longErrMsg(task->m_isdsLongError);
	QList<TaskSearchOwnerFulltext::BoxEntry> foundBoxes(task->m_foundBoxes);
	quint64 totalDb = task->m_totalMatchingBoxes;
	bool gotLastPage = task->m_gotLastPage;

	delete task; task = Q_NULLPTR;

	/*
	 * No dialogue can be raised here because this method can be called
	 * from a non-GUI thread.
	 */
	if (taskResult != TaskSearchOwnerFulltext::SOF_SUCCESS) {
		return SearchResultFt(taskResult, longErrMsg, gotLastPage);
	}

	bool selectFirstRow = m_contactTableModel.rowCount() == 0;
	m_contactTableModel.appendData(foundBoxes);

	m_ui->searchResultText->setText(totalFoundStr(totalDb) + QLatin1String("; ") +
	    tr("Displayed") + QLatin1String(": ") +
	    QString::number(m_contactTableModel.rowCount()));

	if (m_contactTableModel.rowCount() > 0) {
		m_ui->contactTableView->selectColumn(
		    BoxContactsModel::CHECKBOX_COL);
		if (selectFirstRow) {
			m_ui->contactTableView->selectRow(0);
		}
	}

	return SearchResultFt(taskResult, QString(), gotLastPage);
}

DlgDsSearch::SearchResultFt DlgDsSearch::queryBoxFulltextAll(
    enum TaskSearchOwnerFulltext::FulltextTarget target,
    enum TaskSearchOwnerFulltext::BoxType boxType, const QString &phrase)
{
	m_contactTableModel.removeRows(0, m_contactTableModel.rowCount());

	m_ui->searchResultText->setText(totalFoundStr(0));

	qint64 pageNum = 0;
	SearchResultFt searchResult(TaskSearchOwnerFulltext::SOF_ERROR,
	    QString(), false);
	while (searchResult = queryBoxFulltextPage(target, boxType, phrase, pageNum),
	       ((searchResult.result == TaskSearchOwnerFulltext::SOF_SUCCESS) && !searchResult.lastPage)) {
		//QCoreApplication::processEvents();
		++pageNum;
		if (m_breakDownloadLoop) {
			m_breakDownloadLoop = false;
			break;
		}
	}

	//m_ui->contactTableView->resizeColumnsToContents();

	return searchResult;
}

void DlgDsSearch::displaySearchResultFt(int result, const QString &longErrMsg)
{
	switch (result) {
	case TaskSearchOwnerFulltext::SOF_SUCCESS:
		/* Don't display anything on success. */
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
}

QString DlgDsSearch::totalFoundStr(int total)
{
	return tr("Total found") + QLatin1String(": ") +
	    QString::number(total);
}
