/*
 * Copyright (C) 2014-2015 CZ.NIC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations including
 * the two.
 */


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
#include "src/io/filesystem.h"


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
#ifdef PORTABLE_APPLICATION
	this->labelDatovka->setText(this->labelDatovka->text() + " - " +
	    tr("Portable version"));
#endif /* PORTABLE_APPLICATION */
	this->labelVersionNum->setText(VERSION);

	QString copyright =
	    "Copyright © 2014–2015 CZ.NIC, z. s. p. o. "
	    "&lt;<a href=\"" CZ_NIC_URL "\">" CZ_NIC_URL "</a>&gt;";
	this->labelCopy->setText(copyright);
	this->labelCopy->setTextFormat(Qt::RichText);
	this->labelCopy->setTextInteractionFlags(Qt::TextBrowserInteraction);
	this->labelCopy->setOpenExternalLinks(true);

	QString url = "&lt;<a href=\"" DATOVKA_HOMEPAGE_URL "\">"
	    DATOVKA_HOMEPAGE_URL "</a>&gt;";
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

