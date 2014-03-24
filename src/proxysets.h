#ifndef PROXYPREFERENCES_H
#define PROXYPREFERENCES_H

#include <QDialog>
#include "ui_proxysets.h"

class ProxyDialog : public QDialog, public Ui::ProxyDialog {
    Q_OBJECT

public:
    ProxyDialog( QWidget * parent = 0);
};

#endif // PROXYPREFERENCES_H
