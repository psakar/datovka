#ifndef _DLG_CREATE_ACCOUNT_H_
#define _DLG_CREATE_ACCOUNT_H_


#include <QDialog>
#include <QFileDialog>
#include <QTreeView>

#include "src/common.h"
#include "ui_dlg_create_account.h"
#include "src/io/isds_sessions.h"
#include "src/io/account_db.h"

class DlgCreateAccount : public QDialog, public Ui::CreateAccount {
	Q_OBJECT

public:
	enum Action {
		ACT_ADDNEW,
		ACT_EDIT,
		ACT_PWD,
		ACT_CERT,
		ACT_CERTPWD,
		ACT_IDBOX
	};

	DlgCreateAccount(QTreeView &accountList, AccountDb &m_accountDb,
	    QModelIndex acntTopIdx, Action action, QWidget *parent = 0);

private slots:
	void setActiveButton(int);
	void addCertificateFromFile(void);
	void saveAccount(void);
	void checkInputFields(void);

signals:
	void getAccountUserDataboxInfo(QModelIndex);

private:
	void initAccountDialog(void);
	void setCurrentAccountData(void);

	QTreeView &m_accountList;
	AccountDb &m_accountDb;
	QModelIndex m_acntTopIdx;
	const Action m_action;
	int m_loginmethod;
	QString m_certPath;
};


#endif /* _DLG_CREATE_ACCOUNT_H_ */
