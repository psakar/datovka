#ifndef DLG_DB_IMPORT_H
#define DLG_DB_IMPORT_H

#include <QDialog>
#include "src/common.h"
#include "ui_dlg_db_import.h"

class DbImportDialog : public QDialog, public Ui::DbImport
{
	Q_OBJECT

public:
	enum DbAction {
		IMPORT_FROM_DIR,
		IMPORT_SEL_FILES
	};

public:
	DbImportDialog(QWidget *parent = 0);
signals:
	void returnDbAction(int);

private slots:
	void ImportDbFiles(void);
};


#endif // DLG_DB_IMPORT_H
