#ifndef DLG_DS_SEARCH_H
#define DLG_DS_SEARCH_H

#include <QDialog>
#include "src/common.h"
#include "ui_dlg_ds_search.h"

class dlg_ds_search_dialog : public QDialog, public Ui::dlg_ds_search_dialog {
	Q_OBJECT

public:
	dlg_ds_search_dialog(QWidget *parent = 0,
	    QTableWidget *recipientTableWidget = 0,
	    QString action = "Blank");

private slots:
	void checkInputFields(void);
	void insertDsItems(void);
	void enableOkButton(void);
	void searchDataBox(void);


private:
	bool isInRecipientTable(QString idDs);
	void initSearchWindow(void);
	void addContactsToTable(QList<QVector<QString>> contactList);
	QTableWidget *m_recipientTableWidget;
	QString m_action;
};

#endif // DLG_DS_SEARCH_H
