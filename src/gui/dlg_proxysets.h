

#ifndef _DLG_PROXYSETS_H_
#define _DLG_PROXYSETS_H_

#include <QDialog>

#include "src/common.h"
#include "ui_dlg_proxysets.h"


class DlgProxysets : public QDialog, public Ui::Proxysets {
    Q_OBJECT

public:
	DlgProxysets(QWidget *parent = 0);

private slots:
	void saveChanges(void) const;
	void setActiveTextEdit1(bool state);
	void setActiveTextEdit2(bool state);

private:
	void initProxyDialog(void);
};


#endif /* _DLG_PROXYSETS_H_ */
