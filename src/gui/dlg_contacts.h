#ifndef DLG_CONTACTS_H
#define DLG_CONTACTS_H

#include <QDialog>
#include <QTableWidget>
#include "ui_dlg_contacts.h"
#include "src/common.h"

class dlg_contacts : public QDialog, public Ui::dlg_contacts
{
	Q_OBJECT

public:
	dlg_contacts(QWidget *parent = 0, QTableWidget *recipientTableWidget = 0);

private slots:
	void findContact(QString);
	void clearContactText(void);
	void fillContactFromMessage(void);
	void doClick(QTableWidgetItem*);
	void insertDsItems(void);
private:
	QTableWidget *m_recipientTableWidget;
};

#endif // DLG_CONTACTS_H
