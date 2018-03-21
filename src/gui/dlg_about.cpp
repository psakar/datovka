/*
 * Copyright (C) 2014-2018 CZ.NIC
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

#include "src/about.h"
#include "src/common.h"
#include "src/gui/dlg_about.h"
#include "src/io/filesystem.h"
#include "ui_dlg_about.h"

DlgAbout::DlgAbout(QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgAbout)
{
	m_ui->setupUi(this);

	m_ui->labelVersion->setText(
	    "<span style=\"font-size:15pt;\"><b>Datovka"
#ifdef PORTABLE_APPLICATION
	    " - " + tr("Portable version") +
#endif /* PORTABLE_APPLICATION */
	    "</b></span>"
	    "<br/><br/>"
	    "<b>" + tr("Version") + ": " VERSION "</b>"
	    "<br/>"
	    + tr("Free client for Czech eGov data boxes."));
	m_ui->labelVersion->setAlignment(Qt::AlignHCenter);
	m_ui->labelVersion->setTextFormat(Qt::RichText);
	m_ui->labelVersion->setTextInteractionFlags(Qt::TextBrowserInteraction);

	QString copyrightHtml(
	    "Copyright &copy; 2014–2018 CZ.NIC, z. s. p. o. "
	    "&lt;<a href=\"" CZ_NIC_URL "\">" CZ_NIC_URL "</a>&gt;");
	copyrightHtml += "<br/>" + tr("Additional informations") + ": "
	    "<a href=\"" DATOVKA_HOMEPAGE_URL "\">" + tr("home page") + "</a>"
	    ","
	    "<a href=\"" DATOVKA_ONLINE_HELP_URL "\">" + tr("handbook") + "</a>"
	    ","
	    "<a href=\"" DATOVKA_FAQ_URL "\">" + tr("FAQ") + "</a>";
	copyrightHtml += "<br/>" + tr("Support") + ": "
	    "&lt;<a href=\"mailto:" SUPPORT_MAIL "?Subject=[Datovka%20" VERSION "]\">" SUPPORT_MAIL "</a>&gt;";
	m_ui->labelCopy->setText(copyrightHtml);
	m_ui->labelCopy->setTextFormat(Qt::RichText);
	m_ui->labelCopy->setTextInteractionFlags(Qt::TextBrowserInteraction);
	m_ui->labelCopy->setOpenExternalLinks(true);

	QString librariesStr("<b>");
	librariesStr += QObject::tr("Depends on libraries:");
	librariesStr += "</b><br/>";
	m_ui->labelLibs->setText(librariesStr +
	    libraryDependencies().join("<br/>"));
	m_ui->labelLibs->setTextFormat(Qt::RichText);
	m_ui->labelLibs->setTextInteractionFlags(Qt::TextBrowserInteraction);
	m_ui->labelLibs->setAlignment(Qt::AlignHCenter);
	m_ui->labelLibs->setWordWrap(true);

	showLicence();

	connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(close()));
}

DlgAbout::~DlgAbout(void)
{
	delete m_ui;
}

void DlgAbout::about(QWidget *parent)
{
	DlgAbout dlg(parent);
	dlg.exec();
}

void DlgAbout::showLicence(void)
{
	m_ui->textEdit->setPlainText(
	    suppliedTextFileContent(TEXT_FILE_LICENCE));
	if (m_ui->textEdit->toPlainText().isEmpty()) {
		m_ui->textEdit->setPlainText(
		    tr("File '%1' either doesn't exist or is empty.")
		        .arg(expectedTextFilePath(TEXT_FILE_LICENCE)));
	}
}
