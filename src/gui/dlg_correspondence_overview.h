#ifndef _DLG_CORRESPONDENCE_OVERVIEW_H_
#define _DLG_CORRESPONDENCE_OVERVIEW_H_

#include <QDialog>

#include "src/common.h"
#include "ui_dlg_correspondence_overview.h"


class dlg_correspondence_overview : public QDialog,
    public Ui::dlg_correspondence_overview {
    Q_OBJECT

public:
	dlg_correspondence_overview(QWidget *parent = 0);

};

#endif // _DLG_CORRESPONDENCE_OVERVIEW_H_
