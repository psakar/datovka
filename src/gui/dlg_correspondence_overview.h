#ifndef _DLG_CORRESPONDENCE_OVERVIEW_H_
#define _DLG_CORRESPONDENCE_OVERVIEW_H_

#include <QDialog>

#include "src/common.h"
#include "ui_dlg_correspondence_overview.h"


class DlgCorrespondenceOverview : public QDialog,
    public Ui::CorrespondenceOverview {
    Q_OBJECT

public:
	DlgCorrespondenceOverview(QWidget *parent = 0);

};

#endif // _DLG_CORRESPONDENCE_OVERVIEW_H_
