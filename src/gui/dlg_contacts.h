

#ifndef _DLG_CONTACTS_H_
#define _DLG_CONTACTS_H_


#include <QDialog>
#include <QTableWidget>

#include "src/common.h"
#include "src/io/message_db.h"
#include "ui_dlg_contacts.h"


class DlgContacts : public QDialog, public Ui::Contacts
{
	Q_OBJECT

public:
	DlgContacts(const MessageDb &db, const QString &dbId,
	    QTableWidget &recipientTableWidget,
	    QString dbType, bool dbEffectiveOVM, bool dbOpenAddressing,
	    QWidget *parent = 0, QString useName = "");

private slots:
	void filterContact(const QString &text);
	void clearContactText(void);
	void fillContactsFromMessageDb(void);
	void enableOkButton(void);
	void insertDsItems(void);

private:
	bool isInRecipientTable(const QString &idDs) const;

	QTableWidget &m_recipientTableWidget;
	const MessageDb &m_messDb;
	const QString m_dbId;
	QString m_dbType;
	bool m_dbEffectiveOVM;
	bool m_dbOpenAddressing;
	const QString m_userName;
	QString getUserInfoFormIsds(QString idDbox);
};


#endif /* _DLG_CONTACTS_H_ */
