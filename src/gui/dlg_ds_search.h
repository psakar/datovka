#ifndef DLG_DS_SEARCH_H
#define DLG_DS_SEARCH_H

#include <QDialog>
#include "src/common.h"
#include "ui_dlg_ds_search.h"


/* Box type */
typedef enum {
    DBTYPE_SYSTEM = 0,
    DBTYPE_OVM = 10,
    DBTYPE_OVM_NOTAR = 11,
    DBTYPE_OVM_EXEKUT = 12,
    DBTYPE_OVM_REQ = 13,
    DBTYPE_PO = 20,
    DBTYPE_PO_ZAK = 21,
    DBTYPE_PO_REQ = 22,
    DBTYPE_PFO = 30,
    DBTYPE_PFO_ADVOK = 31,
    DBTYPE_PFO_DANPOR = 32,
    DBTYPE_PFO_INSSPR = 33,
    DBTYPE_FO = 40
} isds_DbType;

class isds_DbOwnerInfo {
public:
	char *dbID;
	isds_DbType *dbType;
	char *ic;
	char *firmName;
	char *address;
};


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
