#ifndef _DLG_PREFERENCES_H_
#define _DLG_PREFERENCES_H_

#include <QDialog>

#include "ui_dlg_preferences.h"

class PreferencesDialog : public QDialog, public Ui::Preferences {
    Q_OBJECT

public:
    PreferencesDialog( QWidget * parent = 0);
};

#endif /* _DLG_PREFERENCES_H_ */
