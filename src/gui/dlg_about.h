

#ifndef _DLG_ABOUT_H_
#define _DLG_ABOUT_H_


#include <QDialog>
#include "src/common.h"
#include "ui_dlg_about.h"


class aboutDialog : public QDialog, public Ui::AboutDialog
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


#endif /* _DLG_ABOUT_H_ */
