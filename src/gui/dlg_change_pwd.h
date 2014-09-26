

#ifndef _DLG_CHANGE_PWD_H_
#define _DLG_CHANGE_PWD_H_


#include <QDialog>
#include <QTimer>
#include <QTreeView>

#include "src/common.h"
#include "src/models/accounts_model.h"
#include "ui_dlg_change_pwd.h"


class DlgChangePwd : public QDialog, public Ui::ChangePwd {
	Q_OBJECT

public:
	DlgChangePwd(const QString &boxId, QTreeView &accountList,
	    const AccountModel::SettingsMap &accountInfo, QWidget *parent = 0);

private slots:
	void generatePassword(void);
	void showHidePasswordLine(void);
	void changePassword(void);
	void checkInputFields(void);
	void pingIsdsServer(void);

private:
	QTimer *pingTimer;
	void initPwdChangeDialog(void);

	static
	QString generateRandomString(void);

	static
	const QString possibleCharacters;
	static
	const int randomStringLength;

	QTreeView &m_accountList;
	const AccountModel::SettingsMap &m_accountInfo;
	const QString m_boxId;
};


#endif /* _DLG_CHANGE_PWD_H_ */
