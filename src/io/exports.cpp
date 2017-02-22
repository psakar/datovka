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


#include <QDebug>
#include <QFileDialog>
#include <QTextDocument>
#include <QPrinter>

#include "src/io/exports.h"
#include "src/log/log.h"
#include "src/io/filesystem.h"
#include "src/io/message_db_set.h"
#include "src/settings/preferences.h"


bool Exports::exportAs(QWidget *parent, const MessageDbSet &dbSet,
    ExportFileType fileType, const QString &attachPath,
    const QString &attachFileName, const QString &userName, const QString &dbId,
    MessageDb::MsgId msgId, bool askLocation, QString &lastPath, QString &errStr)
{
	debugFuncCall();

	Q_ASSERT(!userName.isEmpty());
	Q_ASSERT(msgId.dmId >= 0);

	QString fileTypeStr;
	QString fileSufix;
	QByteArray base64;
	QString fileNameformat;
	bool ret = false;
	QString msgID = QString::number(msgId.dmId);

	const MessageDb *messageDb =
	    dbSet.constAccessMessageDb(msgId.deliveryTime);

	if (Q_NULLPTR == messageDb) {
		Q_ASSERT(0);
		return ret;
	}

	switch (fileType) {
	case ZFO_MESSAGE:
		fileTypeStr = QObject::tr("message");
		fileSufix = ".zfo";
		fileNameformat = globPref.message_filename_format;
		base64 = messageDb->msgsMessageBase64(msgId.dmId);
		break;
	case ZFO_DELIVERY:
		fileTypeStr = QObject::tr("delivery info");
		fileSufix = ".zfo";
		fileNameformat = globPref.delivery_filename_format;
		base64 = messageDb->msgsGetDeliveryInfoBase64(msgId.dmId);
		break;
	case ZFO_DELIV_ATTACH:
		fileTypeStr = QObject::tr("delivery info");
		fileSufix = ".zfo";
		fileNameformat = globPref.delivery_filename_format_all_attach;
		base64 = messageDb->msgsGetDeliveryInfoBase64(msgId.dmId);
		break;
	case PDF_DELIVERY:
		fileTypeStr = QObject::tr("delivery info");
		fileSufix = ".pdf";
		fileNameformat = globPref.delivery_filename_format;
		base64 = messageDb->msgsGetDeliveryInfoBase64(msgId.dmId);
		break;
	case PDF_DELIV_ATTACH:
		fileTypeStr = QObject::tr("delivery info");
		fileSufix = ".pdf";
		fileNameformat = globPref.delivery_filename_format_all_attach;
		base64 = messageDb->msgsGetDeliveryInfoBase64(msgId.dmId);
		break;
	case PDF_ENVELOPE:
		fileTypeStr = QObject::tr("message envelope");
		fileSufix = ".pdf";
		fileNameformat = globPref.message_filename_format;
		base64 = "n/a";
		break;
	default:
		Q_ASSERT(0);
		return ret;
		break;
	}

	if (base64.isEmpty()) {
		// TODO - complete message is required.
		errStr = QObject::tr("There is not complete message \"%1\" "
		    "for export!").arg(msgID);
		return ret;
	}

	MessageDb::FilenameEntry entry =
	    messageDb->msgsGetAdditionalFilenameEntry(msgId.dmId);

	QString fileName = fileNameFromFormat(fileNameformat, msgId.dmId,
	    dbId, userName, attachFileName, entry.dmDeliveryTime,
	    entry.dmAcceptanceTime, entry.dmAnnotation, entry.dmSender);

	fileName = attachPath + QDir::separator() + fileName + fileSufix;

	Q_ASSERT(!fileName.isEmpty());

	if (askLocation) {
		fileName = QFileDialog::getSaveFileName(parent,
		    QObject::tr("Save %1 as file (*%2)").
		         arg(fileTypeStr).arg(fileSufix),
		    fileName, QObject::tr("File (*%1)").arg(fileSufix));
	}

	if (fileName.isEmpty()) {
		errStr = QObject::tr("Export of %1 \"%2\" to file was "
		    "not successful!").arg(fileTypeStr).arg(msgID);
		return ret;
	}

	lastPath = QFileInfo(fileName).absoluteDir().absolutePath();

	switch (fileType) {
	case ZFO_DELIV_ATTACH:
	case ZFO_DELIVERY:
	case ZFO_MESSAGE:
		ret = writeZFO(fileName, QByteArray::fromBase64(base64));
		break;
	case PDF_DELIVERY:
	case PDF_DELIV_ATTACH:
		ret = printPDF(fileName,
		    messageDb->deliveryInfoHtmlToPdf(msgId.dmId));
		break;
	case PDF_ENVELOPE:
		ret = printPDF(fileName,
		    messageDb->envelopeInfoHtmlToPdf(msgId.dmId, ""));
		break;
	default:
		Q_ASSERT(0);
		break;
	}

	return ret;
}

bool Exports::exportEnvAndAttachments(QWidget *parent, const MessageDbSet &dbSet,
    const QString &attachPath, const QString &userName, const QString &dbId,
    MessageDb::MsgId msgId, bool askLocation)
{
	debugFuncCall();

	Q_ASSERT(!userName.isEmpty());
	Q_ASSERT(msgId.dmId >= 0);

	QString newAttachPath;
	QDir dir(attachPath);
	dir.mkdir(QString::number(msgId.dmId));
	newAttachPath = attachPath + QDir::separator() +
	    QString::number(msgId.dmId);

	MessageDb *messageDb = dbSet.constAccessMessageDb(msgId.deliveryTime);
	if (Q_NULLPTR == messageDb) {
		Q_ASSERT(0);
		return false;
	}

	QList<MessageDb::FileData> attachList(
	    messageDb->getFilesFromMessage(msgId.dmId));

	if (attachList.isEmpty()) {
		// TODO - complete message is required.
		return false;
	}

	MessageDb::FilenameEntry entry =
	    messageDb->msgsGetAdditionalFilenameEntry(msgId.dmId);

	foreach (const MessageDb::FileData &attach, attachList) {
		QString filename(attach.dmFileDescr);
		if (filename.isEmpty()) {
			Q_ASSERT(0);
			continue;
		}

		filename = fileNameFromFormat(
		    globPref.attachment_filename_format,
		    msgId.dmId, dbId, userName, filename, entry.dmDeliveryTime,
		    entry.dmAcceptanceTime, entry.dmAnnotation, entry.dmSender);

		filename = newAttachPath + QDir::separator() + filename;

		QByteArray data(
		    QByteArray::fromBase64(attach.dmEncodedContent));

		if (WF_SUCCESS !=
		    writeFile(nonconflictingFileName(filename), data)) {
			continue;
		}
	}

	QString fileName = fileNameFromFormat(globPref.message_filename_format,
	    msgId.dmId, dbId, userName, "", entry.dmDeliveryTime,
	    entry.dmAcceptanceTime, entry.dmAnnotation, entry.dmSender);

	fileName = newAttachPath + QDir::separator() + fileName + ".pdf";

	if (askLocation) {
		fileName = QFileDialog::getSaveFileName(parent,
		    QObject::tr("Save message envelope as PDF file"), fileName,
		    QObject::tr("PDF file (*.pdf)"));
	}

	if (fileName.isEmpty()) {
		return false;
	}

	return printPDF(fileName,
	    messageDb->descriptionHtml(msgId.dmId, 0) +
	    messageDb->fileListHtmlToPdf(msgId.dmId));
}

bool Exports::printPDF(const QString &fileName, const QString &data)
{
	if (data.isEmpty() || fileName.isEmpty()) {
		return false;
	}

	QTextDocument doc;
	doc.setHtml(data);
	QPrinter printer;
	printer.setOutputFileName(fileName);
	printer.setOutputFormat(QPrinter::PdfFormat);
	doc.print(&printer);

	return true;
}

bool Exports::writeZFO(const QString &fileName, const QByteArray &data)
{
	if (data.isEmpty() || fileName.isEmpty()) {
		return false;
	}

	enum WriteFileState ret = writeFile(fileName, data);
	return (WF_SUCCESS == ret);
}
