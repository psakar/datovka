#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include "ui_preferences.h"

class PreferencesDialog : public QDialog, public Ui::Preferences {
    Q_OBJECT

public:
    PreferencesDialog( QWidget * parent = 0);
};

#endif // PREFERENCESDIALOG_H
