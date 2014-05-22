#ifndef _DLG_DS_SEARCH_H_
#define _DLG_DS_SEARCH_H_


#include <QDialog>

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
	    const AccountModel::SettingsMap &accountInfo,
	    QWidget *parent = 0, QString useName = "");

private slots:
	void checkInputFields(void);
	void insertDsItems(void);
	void enableOkButton(void);
	void searchDataBox(void);

private:
	bool isInRecipientTable(const QString &idDs) const;
	void initSearchWindow(void);
	void addContactsToTable(const QList< QVector<QString> > &contactList);

	QTableWidget *m_recipientTableWidget;
	Action m_action;
	QString m_userName;
	AccountModel::SettingsMap m_accountInfo;
};


#endif /* DLG_DS_SEARCH_H */
