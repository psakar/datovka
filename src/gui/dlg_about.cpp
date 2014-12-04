

#if defined(__APPLE__) || defined(__clang__)
#  define __USE_C99_MATH
#  define _Bool bool
#else /* !__APPLE__ */
#  include <cstdbool>
#endif /* __APPLE__ */


#include <cstdlib>
#include <isds.h>
#include <openssl/crypto.h> /* SSLeay_version(3) */
#include <QFile>

#include "dlg_about.h"
#include "src/common.h"


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

	QString url = "<a href=\"" + QString(DATOVKA_ONLINE_HELP_URL) + "\">" +
	    QString(DATOVKA_ONLINE_HELP_URL) + "</a>";
	this->labelUrl->setText(url);
	this->labelUrl->setTextFormat(Qt::RichText);
	this->labelUrl->setTextInteractionFlags(Qt::TextBrowserInteraction);
	this->labelUrl->setOpenExternalLinks(true);

	char * isdsVer = isds_version();
	QString isdsVerStr("libisds ");
	isdsVerStr += isdsVer;
	free(isdsVer); isdsVer = NULL;

	QString librariesStr("<b>");
	librariesStr += QObject::tr("Depends on libraries:");
	librariesStr += "</b><br/>";
	librariesStr += QString("Qt ") + qVersion() + "<br/>";
	librariesStr += isdsVerStr + "<br/>" + SSLeay_version(SSLEAY_VERSION);
	this->labelLibs->setAlignment(Qt::AlignHCenter);
	this->labelLibs->setTextFormat(Qt::RichText);
	this->labelLibs->setWordWrap(true);
	this->labelLibs->setText(librariesStr);
	

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
	this->textEdit->setPlainText(
	    suppliedTextFileContent(TEXT_FILE_LICENCE));
}


/* ========================================================================= */
/*
 * Show credits info in textEdit
 */
void aboutDialog::showCredits(void)
/* ========================================================================= */
{
	this->textEdit->setPlainText(
	    suppliedTextFileContent(TEXT_FILE_CREDITS));
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

