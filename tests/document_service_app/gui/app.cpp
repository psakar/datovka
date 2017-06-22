/*
 * Copyright (C) 2014-2017 CZ.NIC
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

#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QPalette>
#include <QRegExp>
#include <QtGlobal> // qVersion

#include "src/document_service/json/helper.h"
#include "src/document_service/json/service_info.h"
#include "src/document_service/json/stored_files.h"
#include "src/document_service/json/upload_file.h"
#include "tests/document_service_app/gui/app.h"
#include "tests/document_service_app/gui/dialogue_service_info.h"
#include "tests/document_service_app/gui/dialogue_stored_files.h"

#define SVG_PATH "datovka.svg"

const QString blankFilterEditStyle("QLineEdit{background: white;}");
const QString foundFilterEditStyle("QLineEdit{background: #afffaf;}");
const QString notFoundFilterEditStyle("QLineEdit{background: #ffafaf;}");

/*!
 * @brief Return .
 */
static
const QPalette &disableEditPalette(void)
{
	static QPalette palette;
	static bool prepared = false;
	if (!prepared) {
		palette.setColor(QPalette::Base, Qt::lightGray);
		palette.setColor(QPalette::Text, Qt::darkGray);
		prepared = true;
	}
	return palette;
}

MainWindow::MainWindow(const QString &baseUrl, const QString &token,
    const QString &caCertPath, bool ignoreSslErrors)
    : QMainWindow(),
    m_uploadModel(this),
    m_uploadProxyModel(this),
    m_dsc(ignoreSslErrors, this)
{
	setupUi(this);

	ui_baseUrlLine->setText(baseUrl);
	ui_baseUrlLine->setReadOnly(true);
	ui_baseUrlLine->setPalette(disableEditPalette());
	ui_tokenLine->setText(token);
	ui_tokenLine->setReadOnly(true);
	ui_tokenLine->setPalette(disableEditPalette());

	ui_filterLine->setClearButtonEnabled(true);
	connect(ui_filterLine, SIGNAL(textChanged(QString)),
	    this, SLOT(filterHierarchy(QString)));

	ui_treeView->setModel(&m_uploadProxyModel);
	m_uploadProxyModel.setSourceModel(&m_uploadModel);
	connect(ui_treeView->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
	    this, SLOT(uploadHierarchySelectionChanged(QItemSelection, QItemSelection)));

	ui_vSplitter->setCollapsible(0, false);
	ui_vSplitter->setCollapsible(1, false);

	ui_textEdit->setReadOnly(true);

	connectTopMenuBarActions();
	initialiseActions();

	m_dsc.setConnection(ui_baseUrlLine->text(), ui_tokenLine->text());

	connect(&m_dsc, SIGNAL(connectionError(QString)),
	    this, SLOT(writeMessage(QString)));

	/* Add CA certificate. */
	if (!caCertPath.isEmpty()) {
		DocumentServiceConnection::addTrustedCertificate(caCertPath);
	}
}

void MainWindow::filterHierarchy(const QString &text)
{
	m_uploadProxyModel.setFilterRole(UploadHierarchyModel::ROLE_FILTER);

	m_uploadProxyModel.setFilterRegExp(QRegExp(text,
	    Qt::CaseInsensitive, QRegExp::FixedString));

	m_uploadProxyModel.setFilterKeyColumn(0);

	ui_treeView->expandAll();

	if (text.isEmpty()) {
		ui_filterLine->setStyleSheet(blankFilterEditStyle);
	} else if (m_uploadProxyModel.rowCount() != 0) {
		ui_filterLine->setStyleSheet(foundFilterEditStyle);
	} else {
		ui_filterLine->setStyleSheet(notFoundFilterEditStyle);
	}
}

void MainWindow::callSrvcGetAllClients(void)
{
	ui_textEdit->clear();

	QByteArray response;

	if (m_dsc.communicate(DocumentServiceConnection::SRVC_GET_ALL_CLIENTS,
	        QByteArray(), response)) {
		QString responseStr(JsonHelper::toIndentedString(response));
		if (!response.isEmpty() && responseStr.isEmpty()) {
			ui_textEdit->append(QStringLiteral("Could not parse obtained response.\n"));
			ui_textEdit->append(QStringLiteral("\n"));
			ui_textEdit->append(QString::fromUtf8(response));
		} else {
			ui_textEdit->append(responseStr);
		}
	} else {
		ui_textEdit->append(QStringLiteral("Service failed."));
	}
}

void MainWindow::callSrvcServiceInfo(void)
{
	ui_textEdit->clear();

	/* Create dummy response. */
	QByteArray jsonSI(
	    jsonServiceInfo(QLatin1String(SVG_PATH),
	        QStringLiteral("Dummy Document Dervice"),
	        QStringLiteral("Dummy Token")));
	ServiceInfoResp sires(ServiceInfoResp::fromJson(jsonSI));

	QByteArray response;
	if (m_dsc.communicate(DocumentServiceConnection::SRVC_SERVICE_INFO,
	        QByteArray(), response)) {
		if (!response.isEmpty()) {
			sires = ServiceInfoResp::fromJson(response);
			ui_textEdit->append(JsonHelper::toIndentedString(response));
		} else {
			ui_textEdit->append(QStringLiteral("Empty response."));
			ui_textEdit->append(QStringLiteral("Using dummy response."));
		}
	} else {
		ui_textEdit->append(QStringLiteral("Service failed."));
		ui_textEdit->append(QStringLiteral("Using dummy response."));
	}

	/* Show dialogue. */
	DialogueServiceInfo dlg(sires, qobject_cast<QWidget *>(this));
	dlg.exec();
}

void MainWindow::callSrvcUploadHierarchy(void)
{
	ui_textEdit->clear();
	m_uploadModel.setHierarchy(UploadHierarchyResp());

	/* Create dummy response. */
	UploadHierarchyResp uhres(
	    UploadHierarchyResp::fromJson(jsonUploadHierarchy()));

	QByteArray response;
	if (m_dsc.communicate(DocumentServiceConnection::SRVC_UPLOAD_HIERARCHY,
	        QByteArray(), response)) {
		if (!response.isEmpty()) {
			uhres = UploadHierarchyResp::fromJson(response);
			ui_textEdit->append(JsonHelper::toIndentedString(response));
		} else {
			ui_textEdit->append(QStringLiteral("Empty response."));
			ui_textEdit->append(QStringLiteral("Using dummy response."));
		}
	} else {
		ui_textEdit->append(QStringLiteral("Service failed."));
		ui_textEdit->append(QStringLiteral("Using dummy response."));
	}

	m_uploadModel.setHierarchy(uhres);

	ui_treeView->expandAll();
}

/*!
 * @brief Read file content.
 *
 * @param[in]  path File path.
 * @param[out] name File name.
 * @param[out] data File content.
 * @return True if file could be read.
 */
static
bool readFileContent(const QString &path, QString &name, QByteArray &data)
{
	if (path.isEmpty()) {
		return false;
	}

	{
		QFile file(path);
		if (!file.open(QIODevice::ReadOnly)) {
			return false;
		}

		data = file.readAll();
		file.close();
	}

	name = QFileInfo(path).fileName();

	return true;
}

void MainWindow::callSrvcUploadFile(void)
{
	ui_textEdit->clear();

	/* Create upload file request. */
	QStringList idList;
	foreach (const QModelIndex &index, ui_treeView->selectionModel()->selectedIndexes()) {
		const QString idStr(index.data(UploadHierarchyModel::ROLE_ID).toString());
		if (!idStr.isEmpty()) {
			idList.append(idStr);
		} else {
			Q_ASSERT(0);
		}
	}

	if (idList.isEmpty()) {
		ui_textEdit->append(QStringLiteral("No valid places are selected."));
		return;
	}

	QString filePath(QFileDialog::getOpenFileName(this,
	    QStringLiteral("Open File"), QString(),
	    QStringLiteral("ZFO file (*.zfo)")));
	if (filePath.isEmpty()) {
		return;
	}

	QString fileName;
	QByteArray fileContent;
	if (!readFileContent(filePath, fileName, fileContent)) {
		ui_textEdit->append(QStringLiteral("Could not read file content."));
		return;
	}

	UploadFileReq ufreq(idList, fileName, fileContent);
	if (!ufreq.isValid()) {
		ui_textEdit->append(QStringLiteral("Could not create upload file request."));
		return;
	}

	ui_textEdit->append(ufreq.toJson());

	QByteArray response;
	UploadFileResp ufres(UploadFileResp::fromJson(QByteArray()));
	if (m_dsc.communicate(DocumentServiceConnection::SRVC_UPLOAD_FILE,
	        ufreq.toJson(), response)) {
		if (!response.isEmpty()) {
			ufres = UploadFileResp::fromJson(response);
			ui_textEdit->append(JsonHelper::toIndentedString(response));
		} else {
			ui_textEdit->append(QStringLiteral("Empty response."));
			ui_textEdit->append(QStringLiteral("Using dummy response."));
		}
	} else {
		ui_textEdit->append(QStringLiteral("Service failed."));
		ui_textEdit->append(QStringLiteral("Using dummy response."));
	}
}

void MainWindow::callSrvcStoredFiles(void)
{
	ui_textEdit->clear();

	DialogueStoredFiles::Values ids(DialogueStoredFiles::getIdentifiers());
	if (ids.dmIds.isEmpty() && ids.diIds.isEmpty()) {
		ui_textEdit->append(QStringLiteral("No input."));
		return;
	}

	StoredFilesReq sfreq(ids.dmIds, ids.diIds);
	if (!sfreq.isValid()) {
		ui_textEdit->append(QStringLiteral("Could not create stored files request."));
		return;
	}

	ui_textEdit->append(sfreq.toJson());

	QByteArray response;
	StoredFilesResp sfres(StoredFilesResp::fromJson(QByteArray()));
	if (m_dsc.communicate(DocumentServiceConnection::SRVC_STORED_FILES,
	        sfreq.toJson(), response)) {
		if (!response.isEmpty()) {
			sfres = StoredFilesResp::fromJson(response);
			ui_textEdit->append(JsonHelper::toIndentedString(response));
		} else {
			ui_textEdit->append(QStringLiteral("Empty response."));
		}
	} else {
		ui_textEdit->append(QStringLiteral("Service failed."));
	}
}

void MainWindow::callAbout(void)
{
	QMessageBox::about(this, QStringLiteral("About Application"),
	    QStringLiteral("Testing Application") + QStringLiteral("\n") +
	    QStringLiteral("App Version") + QStringLiteral(": ") + VERSION +
	    QStringLiteral("\n") +
	    QStringLiteral("Qt Version") + QStringLiteral(": ") + qVersion() +
	    QStringLiteral("\n") +
	    QStringLiteral("Copyright CZ.NIC, z. s. p. o."));
}

void MainWindow::uploadHierarchySelectionChanged(const QItemSelection &selected,
    const QItemSelection &deselected)
{
	Q_UNUSED(selected)
	Q_UNUSED(deselected)

	const QModelIndexList indexes(ui_treeView->selectionModel()->selectedIndexes());

	ui_actionUploadFile->setEnabled(!indexes.isEmpty());

	/* Entries with empty identifiers should not be able to be selected. */
	foreach (const QModelIndex &index, indexes) {
		if (index.data(UploadHierarchyModel::ROLE_ID).toString().isEmpty()) {
			Q_ASSERT(0);
		}
	}
}

void MainWindow::writeMessage(const QString &message)
{
	ui_textEdit->append(QStringLiteral("Network error: ") + message);
}

void MainWindow::connectTopMenuBarActions(void)
{
	/* File menu. */
	connect(ui_actionQuit, SIGNAL(triggered()), this, SLOT(close()));

	/* Service menu. */
	connect(ui_actionGetAllClients, SIGNAL(triggered()),
	    this, SLOT(callSrvcGetAllClients()));
	    /* Separator. */
	connect(ui_actionServiceInfo, SIGNAL(triggered()),
	    this, SLOT(callSrvcServiceInfo()));
	connect(ui_actionUploadHierarchy, SIGNAL(triggered()),
	    this, SLOT(callSrvcUploadHierarchy()));
	connect(ui_actionUploadFile, SIGNAL(triggered()),
	    this, SLOT(callSrvcUploadFile()));
	connect(ui_actionStoredFiles, SIGNAL(triggered()),
	    this, SLOT(callSrvcStoredFiles()));

	/* Help menu. */
	connect(ui_actionAbout, SIGNAL(triggered()), this, SLOT(callAbout()));
}

void MainWindow::initialiseActions(void)
{
	ui_actionServiceInfo->setEnabled(true);
	ui_actionUploadHierarchy->setEnabled(true);
	ui_actionUploadFile->setEnabled(false);
	ui_actionStoredFiles->setEnabled(true);
}
