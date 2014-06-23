#include <QFile>

#include "dlg_about.h"


aboutDialog::aboutDialog(QWidget *parent) :
    QDialog(parent)
{
	setupUi(this);
	initAboutDialog();
}

/* ========================================================================= */
/*
 * Init dialog
 */
void aboutDialog::initAboutDialog(void)
/* ========================================================================= */
{
	this->labelVersionNum->setText(VERSION);
	this->labelUrl->setText("<a href=\"http://www.nic.cz/\">www.datovka.nic.cz</a>");
	this->labelUrl->setTextFormat(Qt::RichText);
	this->labelUrl->setTextInteractionFlags(Qt::TextBrowserInteraction);
	this->labelUrl->setOpenExternalLinks(true);

	connect(this->pushButtonLicence, SIGNAL(clicked()), this,
	    SLOT(showLicence()));
	connect(this->pushButtonCredits, SIGNAL(clicked()), this,
	    SLOT(showCredits()));

	connect(this->buttonBox, SIGNAL(accepted()), this,
	    SLOT(closeDialog(void)));

}

/* ========================================================================= */
/*
 * Show licence in textEdit
 */
void aboutDialog::showLicence(void)
/* ========================================================================= */
{
	QString filename = LICENCE_PATH;
	QString content = "";
	QFile file(filename);

	if(!file.exists()) {
		qDebug() << "Licence file " << filename << "not found";
	}

	QTextStream textStream(&file);
	this->textEdit->clear();
	this->textEdit->setReadOnly(true);
	if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		content = textStream.readAll();
	}

	file.close();
	this->textEdit->setPlainText(content);
}


/* ========================================================================= */
/*
 * Show credits info in textEdit
 */
void aboutDialog::showCredits(void)
/* ========================================================================= */
{
	QString filename = CREDITS_PATH;
	QString content = "";
	QFile file(filename);

	if(!file.exists()){
		qDebug() << "Credits file " << filename << "not found";
	}

	QTextStream textStream(&file);
	this->textEdit->clear();
	this->textEdit->setReadOnly(true);
	if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		content = textStream.readAll();
	}

	file.close();
	this->textEdit->setPlainText(content);
}


/* ========================================================================= */
/*
 * Close dialog
 */
void aboutDialog::closeDialog(void)
/* ========================================================================= */
{
	close();
}

