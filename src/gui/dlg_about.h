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
};

#endif // DLG_ABOUT_H
