

#include "dlg_preferences.h"
#include "ui_dlg_preferences.h"


DlgPreferences::DlgPreferences(QWidget * parent)
    : QDialog(parent)
{
	setupUi(this);
	initPrefDialog();
}

void DlgPreferences::initPrefDialog(void)
{
	this->auto_download_whole_messages->setChecked(globPref.auto_download_whole_messages);
	this->send_stats_with_version_checks->setChecked(globPref.send_stats_with_version_checks);
	this->check_new_versions->setChecked(globPref.check_new_versions);
	this->store_messages_on_disk->setChecked(globPref.store_messages_on_disk);
	this->store_additional_data_on_disk->setChecked(globPref.store_additional_data_on_disk);
	this->check_crl->setChecked(globPref.check_crl);
	this->language->setCurrentIndex(getLangugeIndex(globPref.language));
	this->send_stats_with_version_checks->setEnabled(this->check_new_versions->isChecked());
	connect(this->check_new_versions, SIGNAL(stateChanged(int)), this, SLOT(setActiveCheckBox(int)));
	connect(this->prefButtonBox, SIGNAL(accepted()), this, SLOT(saveChanges(void)));
	//not used in this dialog
	//this->default_download_signed->setChecked(globPref.default_download_signed);

	if (globPref.after_start_select == 1) {
		this->after_start_select_1->setChecked(true);
		this->after_start_select_2->setChecked(false);
		this->after_start_select_3->setChecked(false);
	} else if (globPref.after_start_select == 2) {
		this->after_start_select_1->setChecked(false);
		this->after_start_select_2->setChecked(true);
		this->after_start_select_3->setChecked(false);
	} else {
		this->after_start_select_1->setChecked(false);
		this->after_start_select_2->setChecked(false);
		this->after_start_select_3->setChecked(true);
	}

	if (globPref.certificate_validation_date == 1) {
		this->certificate_validation_date_1->setChecked(true);
		this->certificate_validation_date_2->setChecked(false);
	} else {
		this->certificate_validation_date_1->setChecked(false);
		this->certificate_validation_date_2->setChecked(true);
	}
}

int DlgPreferences::getLangugeIndex(const QString &language)
{
	if (langCs == language) {
		return 1;
	} else if (langEn == language) {
		return 2;
	} else {
		return 0;
	}
}


const QString & DlgPreferences::getIndexFromLanguge(int index)
{
	switch (index) {
	case 1:
		return langCs;
		break;
	case 2:
		return langEn;
		break;
	default:
		return langSystem;
		break;
	}
}


const QString DlgPreferences::langCs("cs");
const QString DlgPreferences::langEn("en");
const QString DlgPreferences::langSystem("system");


void DlgPreferences::setActiveCheckBox(int state)
{
	this->send_stats_with_version_checks->setEnabled(Qt::Checked == state);
}


void DlgPreferences::saveChanges(void) const
{
	globPref.auto_download_whole_messages =
	    this->auto_download_whole_messages->isChecked();
	// Not used in this dialog.
	//globPref.default_download_signed;
	globPref.store_messages_on_disk =
	    this->store_messages_on_disk->isChecked();
	globPref.store_additional_data_on_disk =
	    this->store_additional_data_on_disk->isChecked();
	globPref.certificate_validation_date =
	    this->certificate_validation_date_1->isChecked() ?
	        GlobPreferences::DOWNLOAD_DATE :
	        GlobPreferences::CURRENT_DATE;
	globPref.check_crl = this->check_crl->isChecked();
	globPref.check_new_versions = this->check_new_versions->isChecked();
	globPref.send_stats_with_version_checks =
	    this->send_stats_with_version_checks->isChecked();
	// Not used in this dialog.
	//date_format;
	globPref.language =
	    getIndexFromLanguge(this->language->currentIndex());
	if (this->after_start_select_1->isChecked()) {
		globPref.after_start_select = GlobPreferences::SELECT_NEWEST;
	} else if (this->after_start_select_2->isChecked()) {
		globPref.after_start_select =
		    GlobPreferences::SELECT_LAST_VISITED;
	} else {
		globPref.after_start_select = GlobPreferences::SELECT_NOTHING;
	}
}
