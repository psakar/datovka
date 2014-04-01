#ifndef _DLG_PROXYSETS_H_
#define _DLG_PROXYSETS_H_

#include <QDialog>

#include "ui_dlg_proxysets.h"

class ProxyDialog : public QDialog, public Ui::ProxyDialog {
    Q_OBJECT

public:
	ProxyDialog( QWidget * parent = 0);
};

#endif /* _DLG_PROXYSETS_H_ */
