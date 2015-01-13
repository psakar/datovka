#ifndef DLG_IMPORT_ZFO_RESULT_H
#define DLG_IMPORT_ZFO_RESULT_H

#include <QDialog>
#include "src/common.h"
#include "ui_dlg_import_zfo_result.h"

class ImportZFOResultDialog : public QDialog, public Ui::ImportZFOResult
{
	Q_OBJECT

public:
	ImportZFOResultDialog(int filesCnt,
	    QList<QPair<QString,QString>> errImportList,
	    QList<QPair<QString,QString>> succImportList,
	    QWidget *parent = 0);

private:
	int m_filesCnt;
	QList<QPair<QString,QString>> m_errImportList;
	QList<QPair<QString,QString>> m_succImportList;
};

#endif // DLG_IMPORT_ZFO_RESULT_H
