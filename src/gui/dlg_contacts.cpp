

#include "dlg_contacts.h"
#include "src/io/isds_sessions.h"

DlgContacts::DlgContacts(const MessageDb &db, const QString &dbId,
    QTableWidget &recipientTableWidget,
    QString dbType, bool dbEffectiveOVM, bool dbOpenAddressing,
    QWidget *parent, QString userName)
    : QDialog(parent),
    m_recipientTableWidget(recipientTableWidget),
    m_messDb(db),
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

	connect(this->filterLineEdit, SIGNAL(textChanged(QString)),
	    this, SLOT(filterContact(QString)));
	connect(this->contactTableWidget,
	    SIGNAL(itemClicked(QTableWidgetItem*)), this,
	    SLOT(enableOkButton()));
	connect(this->clearPushButton, SIGNAL(clicked()), this,
	    SLOT(clearContactText()));
	connect(this->buttonBox, SIGNAL(accepted()), this,
	    SLOT(insertDsItems()));

	this->contactTableWidget->
	    setEditTriggers(QAbstractItemView::NoEditTriggers);
}


void DlgContacts::filterContact(const QString &text)
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

void DlgContacts::clearContactText(void)
{
	this->filterLineEdit->clear();
	this->clearPushButton->setEnabled(false);
}

void DlgContacts::fillContactsFromMessageDb()
{
	QList<QVector<QString>> contactList;
	contactList = m_messDb.uniqueContacts();

	for (int i = 0; i < contactList.count(); i++) {
		if (m_dbId != contactList[i].at(0)) {
			int row = this->contactTableWidget->rowCount();
			this->contactTableWidget->insertRow(row);
			QTableWidgetItem *item = new QTableWidgetItem;
			item->setCheckState(Qt::Unchecked);
			this->contactTableWidget->setItem(row,0,item);
			item = new QTableWidgetItem;
			item->setText(contactList[i].at(0));
			this->contactTableWidget->setItem(row,1,item);
			item = new QTableWidgetItem;
			item->setText(contactList[i].at(1));
			this->contactTableWidget->setItem(row,2,item);
			item = new QTableWidgetItem;
			item->setText(contactList[i].at(2));
			this->contactTableWidget->setItem(row,3,item);
		};
	}
	this->contactTableWidget->resizeColumnsToContents();
}

void DlgContacts::enableOkButton(void)
{

	this->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	for (int i = 0; i < this->contactTableWidget->rowCount(); i++) {
		if (this->contactTableWidget->item(i,0)->checkState()) {
			this->buttonBox->button(QDialogButtonBox::Ok)->
			    setEnabled(true);
		}
	}
}


bool DlgContacts::isInRecipientTable(const QString &idDs) const
{
	for (int i = 0; i < this->m_recipientTableWidget.rowCount(); i++) {
		if (this->m_recipientTableWidget.item(i,0)->text() == idDs)
		return true;
	}
	return false;
}



void DlgContacts::insertDsItems(void)
{
	for (int i = 0; i < this->contactTableWidget->rowCount(); i++) {
		if (this->contactTableWidget->item(i,0)->checkState()) {
			if (!isInRecipientTable(
			    this->contactTableWidget->item(i,1)->text())) {
				int row = m_recipientTableWidget.rowCount();
				m_recipientTableWidget.insertRow(row);
				QTableWidgetItem *item = new QTableWidgetItem;
				item->setText(this->contactTableWidget->
				    item(i,1)->text());
				this->m_recipientTableWidget.setItem(row, 0,
				    item);
				item = new QTableWidgetItem;
				item->setText(this->contactTableWidget->
				    item(i,2)->text());
				this->m_recipientTableWidget.setItem(row, 1,
				    item);
				item = new QTableWidgetItem;
				item->setText(this->contactTableWidget->
				    item(i,3)->text());
				this->m_recipientTableWidget.setItem(row, 2,
				    item);

				item = new QTableWidgetItem;
				if (!m_dbEffectiveOVM) {
					item->setText(getUserInfoFormIsds(
					    this->contactTableWidget->item(i,1)
					    ->text()));
				} else {
					item->setText(tr("no"));
				}
				this->m_recipientTableWidget.setItem(row,3,item);
			}
		}
	}
}


/* ========================================================================= */
/*
 * return dbEffectiveOVM for recipient
 */
QString DlgContacts::getUserInfoFormIsds(QString idDbox)
/* ========================================================================= */
{
	QString str = tr("no");
	struct isds_list *box = NULL;
	struct isds_PersonName *personName = NULL;
	struct isds_Address *address = NULL;
	isds_DbType dbType = DBTYPE_FO;

	isds_DbOwnerInfo_search(&box, m_userName, idDbox, dbType, "",
	    personName, "", NULL, address, "", "", "", "", "", 0, false, false);

	if (0 != box) {
		isds_DbOwnerInfo *item = (isds_DbOwnerInfo *) box->data;
		Q_ASSERT(0 != item);
		str = *item->dbEffectiveOVM ? tr("no") : tr("yes");
	}

	isds_PersonName_free(&personName);
	isds_Address_free(&address);
	isds_list_free(&box);

	return str;
}
