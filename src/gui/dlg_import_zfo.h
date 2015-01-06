#ifndef DLG_IMPORT_ZFO_H
#define DLG_IMPORT_ZFO_H

#include <QDialog>
#include "src/common.h"
#include "ui_dlg_import_zfo.h"

class ImportZFODialog : public QDialog, public Ui::ImportZFO
{
	Q_OBJECT

public:
	enum ZFOaction {
		IMPOR_FROM_DIR,
		IMPOR_SEL_FILES
	};

public:
	ImportZFODialog(QWidget *parent = 0);
signals:
	void returnZFOAction(int);

private slots:
	void ImportFiles(void);
};

#endif // DLG_IMPORT_ZFO_H
