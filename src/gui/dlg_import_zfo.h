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
		IMPORT_FROM_DIR,
		IMPORT_FROM_SUBDIR,
		IMPORT_SEL_FILES
	};

	enum ZFOtype {
		IMPORT_ALL_ZFO,
		IMPORT_MESSAGE_ZFO,
		IMPORT_DELIVERY_ZFO
	};

public:
	ImportZFODialog(QWidget *parent = 0);

signals:
	void returnZFOAction(enum ImportZFODialog::ZFOtype,
	    enum ImportZFODialog::ZFOaction);

private slots:
	void ImportFiles(void);
	void ChangeRadioBox(void);
};

#endif // DLG_IMPORT_ZFO_H
