#include "dlg_preferences.h"
#include "ui_dlg_preferences.h"

PreferencesDialog::PreferencesDialog( QWidget * parent) : QDialog(parent) {

	setupUi(this);
	setPreference();
	// perform additional setup here ...
}

void PreferencesDialog::setPreference(void) {


	qDebug() << globPref.language;

}
