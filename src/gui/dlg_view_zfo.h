#ifndef DLG_VIEW_ZFO_H
#define DLG_VIEW_ZFO_H

#include <QDialog>
#include "src/common.h"
#include "ui_dlg_view_zfo.h"

class dlg_view_zfo : public QDialog, public Ui::dlg_view_zfo
{
	Q_OBJECT

public:
	dlg_view_zfo(QWidget *parent = 0);
};

#endif // DLG_VIEW_ZFO_H
