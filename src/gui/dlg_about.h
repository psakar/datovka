#ifndef DLG_ABOUT_H
#define DLG_ABOUT_H

#include <QDialog>
#include "src/common.h"
#include "ui_dlg_about.h"

class aboutDialog : public QDialog, public Ui::aboutDialog
{
	Q_OBJECT

public:
	aboutDialog(QWidget *parent = 0);

private:
	void initAboutDialog(void);
private slots:
	void showLicence(void);
	void showCredits(void);
	void closeDialog(void);
};

#endif // DLG_ABOUT_H
