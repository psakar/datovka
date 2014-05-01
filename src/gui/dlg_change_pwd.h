#ifndef _DLG_CHANGE_PWD_H_
#define _DLG_CHANGE_PWD_H_


#include <QDialog>
#include <QTreeView>

#include "src/common.h"
#include "ui_dlg_change_pwd.h"


class DlgChangePwd : public QDialog, public Ui::ChangePwd {
	Q_OBJECT

public:
	DlgChangePwd(const QString &boxId, QTreeView &accountList,
	    QWidget *parent = 0);

private slots:
	void generatePassword(void);
	void showHidePasswordLine(void);
	void saveChange(void);
	void checkInputFields(void);

private:
	void initPwdChangeDialog(void);

	static
	QString generateRandomString(void);

	static
	const QString possibleCharacters;
	static
	const int randomStringLength;

	QTreeView &m_accountList;
	const QString m_boxId;
};


#endif /* _DLG_CHANGE_PWD_H_ */
