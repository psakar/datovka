#ifndef _DLG_PROXYSETS_H_
#define _DLG_PROXYSETS_H_

#include <QDialog>
#include "src/common.h"
#include "ui_dlg_proxysets.h"

class ProxyDialog : public QDialog, public Ui::ProxyDialog {
    Q_OBJECT

public:
	ProxyDialog( QWidget * parent = 0);

private slots:
	void saveChanges(void);
	void setActiveTextEdit1(bool);
	void setActiveTextEdit2(bool);

private:
	void initProxyDialog(void);
};

#endif /* _DLG_PROXYSETS_H_ */
