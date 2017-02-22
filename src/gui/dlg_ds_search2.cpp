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

/*
 * Column indexes into recipient table widget.
 */
#define CON_COL_CHECKBOX 0
#define CON_COL_BOX_ID 1
#define CON_COL_BOX_TYPE 2
#define CON_COL_BOX_NAME 3
#define CON_COL_ADDRESS 4

#define CBOX_TYPE_ALL 0
#define CBOX_TYPE_OVM 1
#define CBOX_TYPE_PO 2
#define CBOX_TYPE_PFO 3
#define CBOX_TYPE_FO 4

#define CBOX_TARGET_ALL 0
#define CBOX_TARGET_ADDRESS 1
#define CBOX_TARGET_IC 2
#define CBOX_TARGET_BOX_ID 3

DlgSearch2::DlgSearch2(Action action, QStringList &dbIdList, QWidget *parent,
    const QString &userName)
    : QDialog(parent),
    m_action(action),
    m_dbIdList(dbIdList),
    m_userName(userName),
    m_target(TaskSearchOwnerFulltext::FT_ALL),
    m_boxType(TaskSearchOwnerFulltext::BT_ALL),
    m_phrase()
{
	setupUi(this);

	/* Set default line height for table views/widgets. */
	this->contactTableWidget->setEnabled(false);
	this->contactTableWidget->setNarrowedLineHeight();
	this->contactTableWidget->setColumnWidth(CON_COL_CHECKBOX, 20);
	this->contactTableWidget->setColumnWidth(CON_COL_BOX_ID, 70);
	this->contactTableWidget->setColumnWidth(CON_COL_BOX_TYPE, 70);
	this->contactTableWidget->setColumnWidth(CON_COL_BOX_NAME, 200);

	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	this->searchPushButton->setEnabled(false);
	this->resultGroupBox->hide();

	this->dataBoxTypeCBox->addItem(tr("All") + QStringLiteral(" - ") + tr("All types"));
	this->dataBoxTypeCBox->addItem(tr("OVM") + QStringLiteral(" - ") + tr("Orgán veřejné moci"));
	this->dataBoxTypeCBox->addItem(tr("PO") + QStringLiteral(" - ") + tr("Právnická osoba"));
	this->dataBoxTypeCBox->addItem(tr("PFO") + QStringLiteral(" - ") + tr("Podnikající fyzická osoba"));
	this->dataBoxTypeCBox->addItem(tr("FO") + QStringLiteral(" - ") + tr("Fyzická osoba"));

	this->fulltextTargetCBox->addItem(tr("All") + QStringLiteral(" - ") + tr("Search in all fields"));
	this->fulltextTargetCBox->addItem(tr("Address") + QStringLiteral(" - ") + tr(""));
	this->fulltextTargetCBox->addItem(tr("IC") + QStringLiteral(" - ") + tr("Identification number"));
	this->fulltextTargetCBox->addItem(tr("ID") + QStringLiteral(" - ") + tr("Box identifier"));

	if (this->contactTableWidget->rowCount() > 0) {
		this->contactTableWidget->selectColumn(CON_COL_CHECKBOX);
		this->contactTableWidget->selectRow(0);
	}

	connect(this->contactTableWidget,
	    SIGNAL(itemSelectionChanged()), this,
	    SLOT(setFirtsColumnActive()));
	connect(this->textLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(checkInputFields()));
	connect(this->contactTableWidget,
	    SIGNAL(itemClicked(QTableWidgetItem*)), this,
	    SLOT(enableOkButton()));
	connect(this->contactTableWidget,
	    SIGNAL(itemChanged(QTableWidgetItem*)), this,
	    SLOT(enableOkButton()));
	connect(this->searchPushButton, SIGNAL(clicked()), this,
	    SLOT(searchDataBoxFulltext()));
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

void DlgSearch2::enableOkButton(void)
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

void DlgSearch2::setFirtsColumnActive(void)
{
	this->contactTableWidget->selectColumn(CON_COL_CHECKBOX);
	this->contactTableWidget->selectRow(
	    this->contactTableWidget->currentRow());
}

void DlgSearch2::checkInputFields(void)
{
	this->searchPushButton->setEnabled(
	    this->textLineEdit->text().size() > 2);
}

void DlgSearch2::searchDataBoxFulltext(void)
{
	m_target = TaskSearchOwnerFulltext::FT_ALL;
	switch (this->fulltextTargetCBox->currentIndex()) {
	case CBOX_TARGET_ALL:
		m_target = TaskSearchOwnerFulltext::FT_ALL;
		break;
	case CBOX_TARGET_ADDRESS:
		m_target = TaskSearchOwnerFulltext::FT_ADDRESS;
		break;
	case CBOX_TARGET_IC:
		m_target = TaskSearchOwnerFulltext::FT_IC;
		break;
	case CBOX_TARGET_BOX_ID:
	default:
		m_target = TaskSearchOwnerFulltext::FT_BOX_ID;
		break;
	}

	switch (this->dataBoxTypeCBox->currentIndex()) {
	case CBOX_TYPE_FO:
		m_boxType = TaskSearchOwnerFulltext::BT_FO;
		break;
	case CBOX_TYPE_PFO:
		m_boxType = TaskSearchOwnerFulltext::BT_PFO;
		break;
	case CBOX_TYPE_PO:
		m_boxType = TaskSearchOwnerFulltext::BT_PO;
		break;
	case CBOX_TYPE_OVM:
		m_boxType = TaskSearchOwnerFulltext::BT_OVM;
		break;
	case CBOX_TYPE_ALL:
	default:
		m_boxType = TaskSearchOwnerFulltext::BT_ALL;
		break;
	}

	m_phrase = this->textLineEdit->text();

	queryBoxFulltext(m_target, m_boxType, m_phrase);
}

void DlgSearch2::contactItemDoubleClicked(const QModelIndex &index)
{
	if (!index.isValid()) {
		this->close();
		return;
	}

	m_dbIdList.append(this->contactTableWidget->
	    item(index.row(), CON_COL_BOX_ID)->text());
	this->close();
}

/* ========================================================================= */
/*
 * Add selected recipient databox IDs into recipient list
 */
void DlgSearch2::addSelectedDbIDs(void)
/* ========================================================================= */
{
	for (int i = 0; i < this->contactTableWidget->rowCount(); ++i) {
		if (this->contactTableWidget->item(i, CON_COL_CHECKBOX)->checkState()) {
			m_dbIdList.append(this->contactTableWidget->
			    item(i, CON_COL_BOX_ID)->text());
		}
	}
}

void DlgSearch2::queryBoxFulltext(
    enum TaskSearchOwnerFulltext::FulltextTarget target,
    enum TaskSearchOwnerFulltext::BoxType boxType, const QString &phrase)
{
	this->contactTableWidget->setRowCount(0);
	this->contactTableWidget->setEnabled(false);
	this->resultGroupBox->hide();

	QString resultString = tr("Total found: 0") ;

	TaskSearchOwnerFulltext *task = new (std::nothrow) TaskSearchOwnerFulltext(m_userName,
	    phrase, target, boxType);
	task->setAutoDelete(false);
	globWorkPool.runSingle(task);

	enum TaskSearchOwnerFulltext::Result result = task->m_result;

	this->resultGroupBox->show();
	this->resultGroupBox->setEnabled(true);

	if (result != TaskSearchOwnerFulltext::SOF_SUCCESS) {
		delete task; task = Q_NULLPTR;
		this->searchResultText->setText(resultString);
		return;
	}

	quint64 totalDb = task->m_totalMatchingBoxes;
	QList<TaskSearchOwnerFulltext::BoxEntry> foundBoxes(task->m_foundBoxes);

	delete task; task = Q_NULLPTR;

	resultString = tr("Total found") + ": " + QString::number(totalDb);

	this->searchResultText->setText(resultString);

	this->contactTableWidget->setEnabled(!foundBoxes.isEmpty());
	foreach (const TaskSearchOwnerFulltext::BoxEntry &entry, foundBoxes) {
		int row = this->contactTableWidget->rowCount();
		this->contactTableWidget->insertRow(row);
		QTableWidgetItem *item = new QTableWidgetItem;
		item->setCheckState(Qt::Unchecked);
		this->contactTableWidget->setItem(row, CON_COL_CHECKBOX, item);
		item = new QTableWidgetItem;
		item->setText(entry.id);
		this->contactTableWidget->setItem(row, CON_COL_BOX_ID, item);
		item = new QTableWidgetItem;
		item->setText(convertDbTypeToString(entry.type));
		this->contactTableWidget->setItem(row, CON_COL_BOX_TYPE, item);
		item = new QTableWidgetItem;
		item->setText(entry.name);
		this->contactTableWidget->setItem(row, CON_COL_BOX_NAME, item);
		item = new QTableWidgetItem;
		item->setText(entry.address);
		this->contactTableWidget->setItem(row, CON_COL_ADDRESS, item);
		//this->contactTableWidget->resizeColumnsToContents();
	}
}
