#ifndef DLG_DS_SEARCH_H
#define DLG_DS_SEARCH_H

#include <QDialog>
#include "src/common.h"
#include "ui_dlg_ds_search.h"

class dlg_ds_search_dialog : public QDialog, public Ui::dlg_ds_search_dialog {
	Q_OBJECT

public:
	dlg_ds_search_dialog(QWidget *parent = 0);

private slots:
	void checkInputFields();


private:
	void initSearchWindow(void);
};

#endif // DLG_DS_SEARCH_H
