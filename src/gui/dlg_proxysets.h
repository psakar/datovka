

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
	void showHttpProxyPassword(int state);
	void showHttpsProxyPassword(int state);

	void toggleHttpProxyDetection(bool state);
	void toggleHttpsProxyDetection(bool state);

	void toggleHttpProxyPassword(bool state);
	void toggleHttpsProxyPassword(bool state);

	void saveChanges(void) const;
	void setActiveHttpProxyEdit(bool state);
	void setActiveHttpsProxyEdit(bool state);

private:
	void loadProxyDialog(const GlobProxySettings &proxySettings);
};


#endif /* _DLG_PROXYSETS_H_ */
