#ifndef DLG_CHANGE_PWD_H
#define DLG_CHANGE_PWD_H

#include <QDialog>
#include <QTreeView>
#include "src/common.h"
#include "ui_dlg_change_pwd.h"

const QString possibleCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
const int randomStringLength = 10;

class changePassword : public QDialog, public Ui::changePassword {
	Q_OBJECT

public:
	changePassword(QWidget *parent = 0, QTreeView *accountList = 0,
	    QString boxId = "");

private slots:
	void generatePassword(void);
	void showHidePasswordLine(void);
	void saveChange(void);
	void checkInputFields(void);

private:
	void initPwdChangeDialog(QString idBox);
	QString getRandomString(void) const;
	QTreeView *m_accountList;
	QString  m_idBox;
};

#endif // DLG_CHANGE_PWD_H
