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

#include <QEventLoop>
#include <QFile>
#include <QTimer>
#include <QUrl>

#include "src/datovka_shared/log/log.h"
#include "src/datovka_shared/records_management/io/records_management_connection.h"

/* Must be set to false for production releases. */
const bool RecMgmt::Connection::ignoreSslErrorsDflt = false;

/*!
 * @brief Converts service identifier onto service name.
 */
static
const QString &serviceName(enum RecMgmt::Connection::ServiceId srvcId)
{
	static const QString SrvServiceInfo(QStringLiteral("service_info"));
	static const QString SrvUploadHierarchy(QStringLiteral("upload_hierarchy"));
	static const QString SrvUploadFile(QStringLiteral("upload_file"));
	static const QString SrvStoredFiles(QStringLiteral("stored_files"));
	static const QString InvalidService;

	switch (srvcId) {
	case RecMgmt::Connection::SRVC_SERVICE_INFO:
		return SrvServiceInfo;
		break;
	case RecMgmt::Connection::SRVC_UPLOAD_HIERARCHY:
		return SrvUploadHierarchy;
		break;
	case RecMgmt::Connection::SRVC_UPLOAD_FILE:
		return SrvUploadFile;
		break;
	case RecMgmt::Connection::SRVC_STORED_FILES:
		return SrvStoredFiles;
		break;
	default:
		Q_ASSERT(0);
		return InvalidService;
		break;
	}
}

/*!
 * @brief Create URL from base URL and from service identifier.
 */
static
QUrl constructUrl(QString baseUrl, enum RecMgmt::Connection::ServiceId srvcId)
{
	const QString &srvcName(serviceName(srvcId));

	if (baseUrl.isEmpty() || srvcName.isEmpty()) {
		return QUrl();
	}

	if (baseUrl.at(baseUrl.length() - 1) != '/') {
		baseUrl += '/';
	}
	return QUrl(baseUrl + srvcName);
}

RecMgmt::Connection::Connection(bool ignoreSslErrors,
    QObject *parent)
    : QObject(parent),
    m_baseUrlStr(),
    m_tokenStr(),
    m_agentName(),
    m_timeOut(60000), /* Milliseconds. */
    m_ignoreSslErrors(ignoreSslErrors),
    m_nam(this)
{
	connect(&m_nam, SIGNAL(sslErrors(QNetworkReply *, const QList<QSslError>)),
	    this, SLOT(handleSslErrors(QNetworkReply *, const QList<QSslError>)));
}

void RecMgmt::Connection::setConnection(const QString &baseUrl,
    const QString &token)
{
	m_baseUrlStr = baseUrl;
	m_tokenStr = token;
}

bool RecMgmt::Connection::communicate(enum ServiceId srvcId,
    const QByteArray &requestData, QByteArray &replyData, QObject *cbObj)
{
	debugFuncCall();

	QNetworkRequest request(createRequest(srvcId));

	QNetworkReply *reply = sendRequest(request, requestData);
	if (reply == Q_NULLPTR) {
		return false;
	}

	if (cbObj != Q_NULLPTR) {
		connect(cbObj, SIGNAL(callAbort()), reply, SLOT(abort()));

		connect(reply, SIGNAL(downloadProgress(qint64, qint64)),
		    cbObj, SLOT(onDownloadProgress(qint64, qint64)));
		connect(reply, SIGNAL(uploadProgress(qint64, qint64)),
		    cbObj, SLOT(onUploadProgress(qint64, qint64)));
	}

	bool retVal = waitReplyFinished(reply, m_timeOut, cbObj);

	if (cbObj != Q_NULLPTR) {
		cbObj->disconnect(SIGNAL(callAbort()), reply, SLOT(abort()));

		reply->disconnect(SIGNAL(downloadProgress(qint64, qint64)),
		    cbObj, SLOT(onDownloadProgress(qint64, qint64)));
		reply->disconnect(SIGNAL(uploadProgress(qint64, qint64)),
		    cbObj, SLOT(onUploadProgress(qint64, qint64)));
	}

	logDebugLv0NL("Loop exited, reply finished: %d", reply->isFinished());
	QList<QByteArray> headerList(reply->rawHeaderList());
	if (!reply->rawHeaderPairs().isEmpty()) {
		logDebugLv0NL("%s", "Received raw headers:");
		foreach (const QNetworkReply::RawHeaderPair &pair,
		         reply->rawHeaderPairs()) {
			logDebugLv0NL("%s: %s", pair.first.constData(),
			    pair.second.constData());
		}
	}

	replyData.clear();

	if (retVal) {
		/* Finished successfully. */
		retVal = processReply(reply, replyData);
		if (!retVal && (Q_NULLPTR != reply)) {
			emit connectionError(reply->errorString());
		}
	}

	reply->deleteLater(); reply = Q_NULLPTR;

	return retVal;
}

static
bool readAndAddCert(const QByteArray &certData, QSsl::EncodingFormat fmt)
{
	if (certData.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	QSslCertificate cert(certData, fmt);
	if (certData.isNull()) {
		return false;
	}

	QSslSocket::addDefaultCaCertificate(cert);

	return true;
}

bool RecMgmt::Connection::addTrustedCertificate(const QString &filePath)
{
	QByteArray certData;

	{
		QFile certFile(filePath);
		if (!certFile.open(QIODevice::ReadOnly)) {
			return false;
		}
		certData = certFile.readAll();
		certFile.close();
	}
	if (certData.isEmpty()) {
		return false;
	}

	if (readAndAddCert(certData, QSsl::Pem)) {
		logDebugLv0NL("Read PEN certificate '%s'.",
		    filePath.toUtf8().constData());
		return true;
	} else {
		logWarningNL("Supplied certificate '%s' is not in PEM format.",
		    filePath.toUtf8().constData());
	}
	if (readAndAddCert(certData, QSsl::Der)) {
		logDebugLv0NL("Read DER certificate '%s'.",
		    filePath.toUtf8().constData());
		return true;
	} else {
		logWarningNL("Supplied certificate '%s' is not in DER format.",
		    filePath.toUtf8().constData());
	}

	logErrorNL("Could not read certificate '%s'.",
	    filePath.toUtf8().constData());
	return false;
}

void RecMgmt::Connection::handleSslErrors(QNetworkReply *reply,
    const QList<QSslError> &errors)
{
	Q_UNUSED(reply);

	QString errMsg("Unspecified SSL error.");

	if (!errors.isEmpty()) {
		QStringList errList;
		foreach (const QSslError &error, errors) {
			errList.append(error.errorString());
		}
		errMsg = errList.join(QStringLiteral("; "));
	}

	logErrorNL("%s", errMsg.toUtf8().constData());
	emit connectionError(errMsg);

	if (m_ignoreSslErrors) {
		logWarningNL("%s", "Ignoring obtained SSL errors.");
		emit connectionError(QStringLiteral("Ignoring obtained SSL errors."));

		if (reply != Q_NULLPTR) {
			reply->ignoreSslErrors();
		}
	}
}

QNetworkRequest RecMgmt::Connection::createRequest(enum ServiceId srvcId) const
{
	debugFuncCall();

	QNetworkRequest request;

	request.setUrl(constructUrl(m_baseUrlStr, srvcId));

	/* Fill request header. */
	request.setRawHeader("User-Agent", m_agentName.toUtf8());
	request.setRawHeader("Host", request.url().host().toUtf8());
	request.setRawHeader("Authentication", m_tokenStr.toUtf8());
	request.setRawHeader("Accept", "application/json");
	request.setRawHeader("Content-Type", "application/json");

	return request;
}

QNetworkReply *RecMgmt::Connection::sendRequest(const QNetworkRequest &request,
    const QByteArray &data)
{
	debugFuncCall();

	switch (m_nam.networkAccessible()) {
	case QNetworkAccessManager::UnknownAccessibility:
	case QNetworkAccessManager::NotAccessible:
		logErrorNL("%s",
		    "Internet connection is probably not available. Check your network settings.");
		return Q_NULLPTR;
		break;
	default:
		break;
	}

	QNetworkReply *reply = Q_NULLPTR;

	if (data.isEmpty()) {
		reply = m_nam.get(request);
	} else {
		reply = m_nam.post(request, data);
	}

	if (reply == Q_NULLPTR) {
		logErrorNL("%s", "No reply.");
		return Q_NULLPTR;
	}

	return reply;
}

bool RecMgmt::Connection::waitReplyFinished(QNetworkReply *reply,
    unsigned int timeOut, QObject *cbObj)
{
	if (Q_UNLIKELY(reply == Q_NULLPTR)) {
		Q_ASSERT(0);
		return false;
	}

	/* Set timeout timer */
	QTimer timer;
	timer.setSingleShot(true);
	QEventLoop eventLoop;
	if (cbObj != Q_NULLPTR) {
		connect(&timer, SIGNAL(timeout()), cbObj, SLOT(onTimeout()));
	}
	connect(&timer, SIGNAL(timeout()), &eventLoop, SLOT(quit()));
	connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));

	do {
		timer.start(timeOut);
		eventLoop.exec();

		/*
		 * Repeat if a callback object is present and while
		 * communication has not been stopped.
		 * Exit if no callback object is specified.
		 */
	} while ((cbObj != Q_NULLPTR) && reply->isRunning());

	if (cbObj != Q_NULLPTR) {
		timer.disconnect(SIGNAL(timeout()), cbObj, SLOT(onTimeout()));
	}
	timer.disconnect(SIGNAL(timeout()), &eventLoop, SLOT(quit()));
	reply->disconnect(SIGNAL(finished()), &eventLoop, SLOT(quit()));

	if (reply->isFinished()) {
		timer.stop();
	} else {
		/* Timeout expired. */
		logErrorNL("%s",
		    "Connection timed out. Check your internet connection.");
		reply->abort();
	}

	/*
	 * The value QNetworkReply::OperationCanceledError means cancelled via
	 * abort() or close().
	 */
	return reply->error() == QNetworkReply::NoError;
}

bool RecMgmt::Connection::processReply(QNetworkReply *reply,
    QByteArray &replyData)
{
	debugFuncCall();

	if (reply == Q_NULLPTR) {
		Q_ASSERT(0);
		return false;
	}

	/* Response status code */
	int statusCode = reply->attribute(
	    QNetworkRequest::HttpStatusCodeAttribute).toInt();
#if 0
	/* Store cookies */
	QVariant variantCookies =
	    reply->header(QNetworkRequest::SetCookieHeader);
	QList<QNetworkCookie> listOfCookies(
	    qvariant_cast< QList<QNetworkCookie> >(variantCookies));
#endif

	replyData = reply->readAll();

	QVariant possibleRedirectUrl;

	switch (statusCode) {
	case 200: /* 200 OK */
		break;
	case 302: /* 302 Found */
		possibleRedirectUrl = reply->attribute(
		    QNetworkRequest::RedirectionTargetAttribute);
		logWarningNL("Redirection '%s'?",
		    possibleRedirectUrl.toString().toUtf8().constData());
		// possibleRedirectUrl.toString();
		break;
	default: /* Any other error. */
		logErrorNL("%s", reply->errorString().toUtf8().constData());
		return false;
		break;
	}

	return true;
}
