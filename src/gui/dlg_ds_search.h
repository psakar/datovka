

#ifndef _DLG_DS_SEARCH_H_
#define _DLG_DS_SEARCH_H_


#include <QDialog>
#include <QTimer>

#include "src/common.h"
#include "src/models/accounts_model.h"
#include "ui_dlg_ds_search.h"


class DlgDsSearch : public QDialog, public Ui::DsSearch {
	Q_OBJECT

public:
	enum Action {
		ACT_BLANK,
		ACT_ADDNEW
	};

	DlgDsSearch(Action action, QTableWidget *recipientTableWidget,
	    QWidget *parent = 0, QString useName = "");

private slots:
	void checkInputFields(void);
	void insertDsItems(void);
	void enableOkButton(void);
	void searchDataBox(void);
	void pingIsdsServer(void);

private:
	QTimer *pingTimer;
	bool isInRecipientTable(const QString &idDs) const;
	void initSearchWindow(void);
	void addContactsToTable(const QList< QVector<QString> > &contactList);

	QTableWidget *m_recipientTableWidget;
	Action m_action;
	const QString m_userName;
};


#endif /* DLG_DS_SEARCH_H */
