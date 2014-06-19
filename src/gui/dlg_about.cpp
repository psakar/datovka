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
	QFile file(filename);
	if(!file.exists()) {
		qDebug() << "Licence file " << filename << "not found";
	}

	QString line;
	this->textEdit->clear();
	this->textEdit->setReadOnly(true);
	if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QTextStream stream(&file);
		while (!stream.atEnd()){
			line = stream.readLine();
			this->textEdit->setText(this->textEdit->toPlainText()
			    + line +"\n");
		}
	}
	file.close();
}


/* ========================================================================= */
/*
 * Show credits info in textEdit
 */
void aboutDialog::showCredits(void)
/* ========================================================================= */
{
	QString filename = CREDITS_PATH ;
	QFile file(filename);
	if(!file.exists()){
		qDebug() << "Credits file " << filename << "not found";
	}

	QString line;
	this->textEdit->clear();
	if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QTextStream stream(&file);
		while (!stream.atEnd()){
			line = stream.readLine();
			this->textEdit->setText(this->textEdit->toPlainText()
			    + line +"\n");
		}
	}
	file.close();
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

