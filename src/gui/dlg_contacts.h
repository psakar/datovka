#ifndef DLG_CONTACTS_H
#define DLG_CONTACTS_H

#include <QDialog>
#include "ui_dlg_contacts.h"
#include "src/common.h"

class dlg_contacts : public QDialog, public Ui::dlg_contacts
{
	Q_OBJECT

public:
	dlg_contacts(QWidget *parent = 0);

private slots:
	void findContact(QString);
	void clearContactText(void);
	void fillContactFromMessage(void);
};

#endif // DLG_CONTACTS_H
