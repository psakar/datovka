

#include "dlg_proxysets.h"
#include "ui_dlg_proxysets.h"


DlgProxysets::DlgProxysets(QWidget *parent)
    : QDialog(parent)
{
	setupUi(this);
	initProxyDialog();
}

void DlgProxysets::initProxyDialog(void)
{
	QStringList hostport;

	connect(this->manualProxyRadioButton1, SIGNAL(toggled (bool)),
	    this, SLOT(setActiveTextEdit1(bool)));
	connect(this->manualProxyRadioButton2, SIGNAL(toggled (bool)),
	    this, SLOT(setActiveTextEdit2(bool)));
	connect(this->proxyButtonBox, SIGNAL(accepted()),
	    this, SLOT(saveChanges(void)));

	if (globProxSet.http_proxy == "-1") {
		this->noProxyRadioButton1->setChecked(false);
		this->autoProxyRadioButton1->setChecked(true);
		this->manualProxyRadioButton1->setChecked(false);
		this->hostnameLineEdit1->setEnabled(false);
		this->portLineEdit1->setEnabled(false);
	} else if (globProxSet.http_proxy == "None") {
		this->noProxyRadioButton1->setChecked(true);
		this->autoProxyRadioButton1->setChecked(false);
		this->manualProxyRadioButton1->setChecked(false);
		this->hostnameLineEdit1->setEnabled(false);
		this->portLineEdit1->setEnabled(false);
	} else {
		this->noProxyRadioButton1->setChecked(false);
		this->autoProxyRadioButton1->setChecked(false);
		this->manualProxyRadioButton1->setChecked(true);
		this->hostnameLineEdit1->setEnabled(true);
		hostport = globProxSet.http_proxy.split(":");
		this->hostnameLineEdit1->setText(hostport[0]);
		this->portLineEdit1->setText( hostport[1]);
	}

	if (globProxSet.https_proxy == "-1") {
		this->noProxyRadioButton2->setChecked(false);
		this->autoProxyRadioButton2->setChecked(true);
		this->manualProxyRadioButton2->setChecked(false);
		this->hostnameLineEdit2->setEnabled(false);
		this->portLineEdit2->setEnabled(false);
	} else if (globProxSet.https_proxy == "None") {
		this->noProxyRadioButton2->setChecked(true);
		this->autoProxyRadioButton2->setChecked(false);
		this->manualProxyRadioButton2->setChecked(false);
		this->hostnameLineEdit2->setEnabled(false);
		this->portLineEdit2->setEnabled(false);
	} else {
		this->noProxyRadioButton2->setChecked(false);
		this->autoProxyRadioButton2->setChecked(false);
		this->manualProxyRadioButton2->setChecked(true);
		this->hostnameLineEdit2->setEnabled(true);
		hostport = globProxSet.https_proxy.split(":");
		this->hostnameLineEdit2->setText(hostport[0]);
		this->portLineEdit2->setText(hostport[1]);
	}
}

void DlgProxysets::setActiveTextEdit1(bool state)
{
	this->hostnameLineEdit1->setEnabled(state);
	this->portLineEdit1->setEnabled(state);
}

void DlgProxysets::setActiveTextEdit2(bool state)
{
	this->hostnameLineEdit2->setEnabled(state);
	this->portLineEdit2->setEnabled(state);
}

void DlgProxysets::saveChanges(void) const
{
	if (this->noProxyRadioButton1->isChecked()) {
		globProxSet.http_proxy = "None";
	} else if (this->autoProxyRadioButton1->isChecked()) {
		globProxSet.http_proxy = "-1";
	} else {
		globProxSet.http_proxy = this->hostnameLineEdit1->text() +
		    ":" + this->portLineEdit1->text();
	}

	if (this->noProxyRadioButton2->isChecked()) {
		globProxSet.https_proxy = "None";
	} else if (this->autoProxyRadioButton2->isChecked()) {
		globProxSet.https_proxy = "-1";
	} else {
		globProxSet.https_proxy = this->hostnameLineEdit2->text() +
		    ":" + this->portLineEdit2->text();
	}
}
