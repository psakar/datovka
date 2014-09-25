

#ifndef _DLG_PREFERENCES_H_
#define _DLG_PREFERENCES_H_


#include <QDialog>

#include "src/common.h"
#include "ui_dlg_preferences.h"


class DlgPreferences : public QDialog, public Ui::Preferences {
    Q_OBJECT

public:
	DlgPreferences(QWidget *parent = 0);

private slots:
	void setActiveTimerSetup(int);
	void setActiveCheckBox(int);
	void saveChanges(void) const;

private:
	void initPrefDialog(void);

	static
	int getLangugeIndex(const QString &language);

	static
	const QString & getIndexFromLanguge(int index);

	static
	const QString langCs;
	static
	const QString langEn;
	static
	const QString langSystem;
};


#endif /* _DLG_PREFERENCES_H_ */
