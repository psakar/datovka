/*
 * Copyright (C) 2014-2016 CZ.NIC
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

#include "src/gui/dlg_about.h"
#include "src/io/filesystem.h"

DlgAbout::DlgAbout(QWidget *parent)
    : QDialog(parent)
{
	setupUi(this);

#ifdef PORTABLE_APPLICATION
	this->labelDatovka->setText(this->labelDatovka->text() + " - " +
	    tr("Portable version"));
	this->labelCopy->setTextInteractionFlags(Qt::TextBrowserInteraction);
#endif /* PORTABLE_APPLICATION */
	this->labelVersion->setText(this->labelVersion->text() + " " VERSION);
	this->labelVersion->setTextInteractionFlags(Qt::TextBrowserInteraction);

	const QString copyrightHtml(
	    "Copyright &copy; 2014–2016 CZ.NIC, z. s. p. o. "
	    "&lt;<a href=\"" CZ_NIC_URL "\">" CZ_NIC_URL "</a>&gt;");
	this->labelCopy->setText(copyrightHtml);
	this->labelCopy->setTextFormat(Qt::RichText);
	this->labelCopy->setTextInteractionFlags(Qt::TextBrowserInteraction);
	this->labelCopy->setOpenExternalLinks(true);

	QString url = "&lt;<a href=\"" DATOVKA_HOMEPAGE_URL "\">"
	    DATOVKA_HOMEPAGE_URL "</a>&gt;";
	this->labelUrl->setText(url);
	this->labelUrl->setTextFormat(Qt::RichText);
	this->labelUrl->setTextInteractionFlags(Qt::TextBrowserInteraction);
	this->labelUrl->setOpenExternalLinks(true);

	QString librariesStr("<b>");
	librariesStr += QObject::tr("Depends on libraries:");
	librariesStr += "</b><br/>";
	this->labelLibs->setText(librariesStr +
	    libraryDependencies().join("<br/>"));
	this->labelLibs->setTextFormat(Qt::RichText);
	this->labelLibs->setTextInteractionFlags(Qt::TextBrowserInteraction);
	this->labelLibs->setAlignment(Qt::AlignHCenter);
	this->labelLibs->setWordWrap(true);

	connect(this->pushButtonLicence, SIGNAL(clicked()), this,
	    SLOT(showLicence()));
	connect(this->pushButtonCredits, SIGNAL(clicked()), this,
	    SLOT(showCredits()));

	connect(this->buttonBox, SIGNAL(accepted()), this, SLOT(close()));
}

QStringList DlgAbout::libraryDependencies(void)
{
	QStringList libs;

	libs.append(QStringLiteral("Qt ") + qVersion());

	char *isdsVer = isds_version();
	libs.append(QStringLiteral("libisds ") + isdsVer);
	free(isdsVer); isdsVer = NULL;

	libs.append(SSLeay_version(SSLEAY_VERSION));

	return libs;
}

void DlgAbout::showLicence(void)
{
	this->textEdit->setPlainText(
	    suppliedTextFileContent(TEXT_FILE_LICENCE));
}

void DlgAbout::showCredits(void)
{
	this->textEdit->setPlainText(
	    suppliedTextFileContent(TEXT_FILE_CREDITS));
}
