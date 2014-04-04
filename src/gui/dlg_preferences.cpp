#include "dlg_preferences.h"
#include "ui_dlg_preferences.h"

PreferencesDialog::PreferencesDialog( QWidget * parent) : QDialog(parent) {

	setupUi(this);

	initPreferencesInDialog();
}

void PreferencesDialog::initPreferencesInDialog(void)
{

	this->auto_download_whole_messages->setChecked(globPref.auto_download_whole_messages);
	this->send_stats_with_version_checks->setChecked(globPref.send_stats_with_version_checks);
	this->check_new_versions->setChecked(globPref.check_new_versions);
	this->store_messages_on_disk->setChecked(globPref.store_messages_on_disk);
	this->store_additional_data_on_disk->setChecked(globPref.store_additional_data_on_disk);
	this->check_crl->setChecked(globPref.check_crl);
	//this->default_download_signed->setChecked(globPref.default_download_signed);
	this->language->setCurrentIndex(getLangugeIndex(globPref.language));
	this->send_stats_with_version_checks->setEnabled(this->check_new_versions->isChecked());
	connect(this->check_new_versions, SIGNAL(stateChanged(int)), this, SLOT(setActiveCheckBox(int)));

}

int PreferencesDialog::getLangugeIndex(QString language)
{
	if (language == "cs") {
		return 1;
	} else if (language == "en") {
		return 2;
	} else {
		return 0;
	}
}


QString PreferencesDialog::getIndexFromLanguge(int index)
{
	switch (index) {
	case 1: return "cs";
		break;
	case 2: return "en";
		break;
	default: return "system";
		break;
	}
}

void PreferencesDialog::setActiveCheckBox(int state)
{
	this->send_stats_with_version_checks->setEnabled(Qt::Checked == state);
}


void PreferencesDialog::saveChangesInDialog(void)
{
	globPref.auto_download_whole_messages = this->auto_download_whole_messages->isChecked();
	globPref.send_stats_with_version_checks = this->send_stats_with_version_checks->isChecked();
	globPref.check_new_versions = this->check_new_versions->isChecked();
	globPref.store_messages_on_disk = this->store_messages_on_disk->isChecked();
	globPref.store_additional_data_on_disk = this->store_additional_data_on_disk->isChecked();
	globPref.check_crl = this->check_crl->isChecked();
	globPref.language = getIndexFromLanguge(this->language->currentIndex());
	//globPref.default_download_signed = this->default_download_signed->isChecked();
}

