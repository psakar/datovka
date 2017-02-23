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
#include <QPrinter>
#include <QTextDocument>

#include "src/io/exports.h"
#include "src/io/filesystem.h"
#include "src/io/message_db_set.h"
#include "src/log/log.h"
#include "src/settings/preferences.h"

QString Exports::attachmentSavePathWithFileName(const MessageDbSet &dbSet,
    const QString &targetPath, const QString &attachFileName,
    const QString &dbId, const QString &userName,
    const MessageDb::MsgId &msgId)
{
	debugFuncCall();

	Q_ASSERT(!userName.isEmpty());
	Q_ASSERT(msgId.dmId >= 0);

	const MessageDb *messageDb =
	    dbSet.constAccessMessageDb(msgId.deliveryTime);
	if (Q_NULLPTR == messageDb) {
		Q_ASSERT(0);
		return QString();
	}

	MessageDb::FilenameEntry entry =
	    messageDb->msgsGetAdditionalFilenameEntry(msgId.dmId);

	QString fileName = fileNameFromFormat(globPref.attachment_filename_format,
	    msgId.dmId, dbId, userName, attachFileName, entry.dmDeliveryTime,
	    entry.dmAcceptanceTime, entry.dmAnnotation, entry.dmSender);
	if (fileName.isEmpty()) {
		return QString();
	}
	return targetPath + QDir::separator() + fileName;
}

Exports::ExportError Exports::exportAs(QWidget *parent,
    const MessageDbSet &dbSet, ExportFileType fileType,
    const QString &targetPath, const QString &attachFileName,
    const QString &userName, const QString &dbId, MessageDb::MsgId msgId,
    bool askLocation, QString &lastPath, QString &errStr)
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
		errStr = QObject::tr("Cannot access message database "
		    "for username \"%1\".").arg(userName);
		return EXP_DB_ERROR;
	}

	// what we will export?
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
		errStr = QObject::tr("Export file type of message \"%1\" "
		    "was not specified!").arg(msgID);
		return EXP_ERROR;
		break;
	}

	// is complete message in database?
	if (base64.isEmpty()) {
		errStr = QObject::tr("Complete message \"%1\" "
		    "missing!").arg(msgID);
		return EXP_NOT_MSG_DATA;
	}

	errStr = QObject::tr("Export of %1 \"%2\" to %3 was not "
	    "successful!").arg(fileTypeStr).arg(msgID).arg(fileSufix);

	MessageDb::FilenameEntry entry =
	    messageDb->msgsGetAdditionalFilenameEntry(msgId.dmId);

	// create new file name with format string
	QString fileName = fileNameFromFormat(fileNameformat, msgId.dmId,
	    dbId, userName, attachFileName, entry.dmDeliveryTime,
	    entry.dmAcceptanceTime, entry.dmAnnotation, entry.dmSender);

	fileName = targetPath + QDir::separator() + fileName + fileSufix;

	Q_ASSERT(!fileName.isEmpty());
	if (fileName.isEmpty()) {
		return EXP_ERROR;
	}

	// ask for a new location?
	if (askLocation) {
		fileName = QFileDialog::getSaveFileName(parent,
		    QObject::tr("Save %1 as file (*%2)").
		         arg(fileTypeStr).arg(fileSufix),
		    fileName, QObject::tr("File (*%1)").arg(fileSufix));
	}

	Q_ASSERT(!fileName.isEmpty());
	if (fileName.isEmpty()) {
		return EXP_ERROR;
	}

	// save file to disk
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
		return EXP_ERROR;
		break;
	}

	if (ret) {
		// remember last path
		lastPath = QFileInfo(fileName).absoluteDir().absolutePath();
		errStr = QObject::tr("Export of %1 \"%2\" to %3 was "
		    "successful.").arg(fileTypeStr).arg(msgID).arg(fileSufix);
		return EXP_SUCCESS;
	} else {
		return EXP_WRITE_FILE_ERROR;
	}
}

Exports::ExportError Exports::exportEnvAndAttachments(const MessageDbSet &dbSet,
    const QString &targetPath, const QString &userName, const QString &dbId,
    MessageDb::MsgId msgId, QString &errStr)
{
	debugFuncCall();

	Q_ASSERT(!userName.isEmpty());
	Q_ASSERT(msgId.dmId >= 0);

	QString newTargetPath;
	QDir dir(targetPath);
	QString msgID = QString::number(msgId.dmId);
	bool attachWriteSuccess = true;

	// create new folder with message id
	dir.mkdir(QString::number(msgId.dmId));
	newTargetPath = targetPath + QDir::separator() +
	    QString::number(msgId.dmId);

	MessageDb *messageDb = dbSet.constAccessMessageDb(msgId.deliveryTime);
	if (Q_NULLPTR == messageDb) {
		Q_ASSERT(0);
		errStr = QObject::tr("Cannot access message database "
		    "for username \"%1\".").arg(userName);
		return EXP_DB_ERROR;
	}

	MessageDb::FilenameEntry entry =
	    messageDb->msgsGetAdditionalFilenameEntry(msgId.dmId);

	// get list of attachments
	QList<MessageDb::FileData> attachList(
	    messageDb->getFilesFromMessage(msgId.dmId));
	if (attachList.isEmpty()) {
		errStr = QObject::tr("Complete message \"%1\" "
		    "missing!").arg(msgID);
		return EXP_NOT_MSG_DATA;
	}

	// save attachments to target folder
	foreach (const MessageDb::FileData &attach, attachList) {
		QString filename(attach.dmFileDescr);
		if (filename.isEmpty()) {
			Q_ASSERT(0);
			errStr = QObject::tr("Some files of message \"%1\""
			    " were not saved to disk!").arg(msgID);
			attachWriteSuccess = false;
			continue;
		}

		// create new file name with format string
		filename = attachmentSavePathWithFileName(dbSet, newTargetPath,
		    filename, dbId, userName, msgId);

		QByteArray data(
		    QByteArray::fromBase64(attach.dmEncodedContent));

		// save file to disk
		if (WF_SUCCESS !=
		    writeFile(nonconflictingFileName(filename), data)) {
		    	errStr = QObject::tr("Some files of message \"%1\""
			    " were not saved to disk!").arg(msgID);
			attachWriteSuccess = false;
			continue;
		}
	}

	// create new file name with format string
	QString fileName = fileNameFromFormat(globPref.message_filename_format,
	    msgId.dmId, dbId, userName, QString(), entry.dmDeliveryTime,
	    entry.dmAcceptanceTime, entry.dmAnnotation, entry.dmSender);

	fileName = newTargetPath + QDir::separator() + fileName + ".pdf";

	if (fileName.isEmpty()) {
		errStr = QObject::tr("Export of message envelope \"%1\" to "
		    "PDF was not successful!").arg(msgID);
		return EXP_ERROR;
	}

	// save file to disk
	if (printPDF(fileName,
	    messageDb->descriptionHtml(msgId.dmId, 0) +
	    messageDb->fileListHtmlToPdf(msgId.dmId)) && attachWriteSuccess) {
		errStr = QObject::tr("Export of message envelope \"%1\" to "
		    "PDF and attachments were successful.").arg(msgID);
		return EXP_SUCCESS;
	} else {
		errStr = QObject::tr("Export of message envelope \"%1\" to "
		    "PDF and attachments were not successful!").arg(msgID);
		return EXP_WRITE_FILE_ERROR;
	}
}

Exports::ExportError Exports::saveAttachmentsWithExports(const MessageDbSet &dbSet,
    const QString &targetPath, const QString &userName, const QString &dbId,
    MessageDb::MsgId msgId, QString &errStr)
{
	debugFuncCall();

	QString lastPath;
	QString msgID = QString::number(msgId.dmId);
	bool attachWriteSuccess = true;

	MessageDb *messageDb = dbSet.constAccessMessageDb(msgId.deliveryTime);
	if (Q_NULLPTR == messageDb) {
		Q_ASSERT(0);
		errStr = QObject::tr("Cannot access message database "
		    "for username \"%1\".").arg(userName);
		return EXP_DB_ERROR;
	}

	// get list of attachments
	QList<MessageDb::FileData> attachList(
	    messageDb->getFilesFromMessage(msgId.dmId));
	if (attachList.isEmpty()) {
		errStr = QObject::tr("Complete message \"%1\" "
		    "missing!").arg(msgID);
		return EXP_NOT_MSG_DATA;
	}

	// save attachments to target folder
	foreach (const MessageDb::FileData &attach, attachList) {
		QString filename(attach.dmFileDescr);
		if (filename.isEmpty()) {
			Q_ASSERT(0);
			errStr = QObject::tr("Some files of message \"%1\""
			    " were not saved to disk!").arg(msgID);
			attachWriteSuccess = false;
			continue;
		}

		// create new file name with format string
		filename = attachmentSavePathWithFileName(dbSet, targetPath,
		    filename, dbId, userName, msgId);

		QByteArray data(
		    QByteArray::fromBase64(attach.dmEncodedContent));

		// save file to disk
		if (WF_SUCCESS !=
		    writeFile(nonconflictingFileName(filename), data)) {
		    	errStr = QObject::tr("Some files of message \"%1\""
			    " were not saved to disk!").arg(msgID);
			attachWriteSuccess = false;
			continue;
		}

		if (globPref.delivery_info_for_every_file) {
			if (globPref.all_attachments_save_zfo_delinfo) {
				exportAs(0, dbSet, Exports::ZFO_DELIV_ATTACH,
				    targetPath, attach.dmFileDescr, userName,
				    dbId, msgId, false, lastPath, errStr);
			}
			if (globPref.all_attachments_save_pdf_delinfo) {
				exportAs(0, dbSet, Exports::PDF_DELIV_ATTACH,
				    targetPath, attach.dmFileDescr, userName,
				    dbId, msgId, false, lastPath, errStr);
			}
		}
	}

	if (globPref.all_attachments_save_zfo_msg) {
		exportAs(0, dbSet, Exports::ZFO_MESSAGE,
		    targetPath, QString(), userName, dbId, msgId, false,
		    lastPath, errStr);
	}

	if (globPref.all_attachments_save_pdf_msgenvel) {
		exportAs(0, dbSet, Exports::PDF_ENVELOPE,
		    targetPath, QString(), userName, dbId, msgId, false,
		    lastPath, errStr);
	}

	if (!globPref.delivery_info_for_every_file) {
		if (globPref.all_attachments_save_zfo_delinfo) {
			exportAs(0, dbSet, Exports::ZFO_DELIVERY,
			    targetPath, QString(), userName, dbId, msgId, false,
			    lastPath, errStr);
		}
		if (globPref.all_attachments_save_pdf_delinfo) {
			exportAs(0, dbSet, Exports::PDF_DELIVERY,
			    targetPath, QString(), userName, dbId, msgId,
			    false, lastPath, errStr);
		}
	}

	if (attachWriteSuccess) {
		errStr = QObject::tr("All message attachments \"%1\" were "
		    "successfully saved to target folder.").arg(msgID);
		return EXP_SUCCESS;
	} else {
		errStr = QObject::tr("Some attachments of message \"%1\" were "
		    "not successfully saved!").arg(msgID);
		return EXP_WRITE_FILE_ERROR;
	}
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
	return (WF_SUCCESS == writeFile(fileName, data));
}
