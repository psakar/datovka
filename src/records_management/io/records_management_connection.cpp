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

#include <QEventLoop>
#include <QFile>
#include <QTimer>
#include <QUrl>

#include "src/records_management/io/records_management_connection.h"

/* Must be set to false for production releases. */
const bool RecordsManagementConnection::ignoreSslErrorsDflt = true;

#define logFuncCall() \
	qDebug("%s()", __func__)

/*!
 * @brief Converts service identifier onto service name.
 */
static
const QString &serviceName(enum RecordsManagementConnection::ServiceId srvcId)
{
	static const QString SrvServiceInfo(QStringLiteral("service_info"));
	static const QString SrvUploadHierarchy(QStringLiteral("upload_hierarchy"));
	static const QString SrvUploadFile(QStringLiteral("upload_file"));
	static const QString SrvStoredFiles(QStringLiteral("stored_files"));
	static const QString InvalidService;

	switch (srvcId) {
	case RecordsManagementConnection::SRVC_SERVICE_INFO:
		return SrvServiceInfo;
		break;
	case RecordsManagementConnection::SRVC_UPLOAD_HIERARCHY:
		return SrvUploadHierarchy;
		break;
	case RecordsManagementConnection::SRVC_UPLOAD_FILE:
		return SrvUploadFile;
		break;
	case RecordsManagementConnection::SRVC_STORED_FILES:
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
QUrl constructUrl(QString baseUrl,
    enum RecordsManagementConnection::ServiceId srvcId)
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

RecordsManagementConnection::RecordsManagementConnection(bool ignoreSslErrors,
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

void RecordsManagementConnection::setConnection(const QString &baseUrl,
    const QString &token)
{
	m_baseUrlStr = baseUrl;
	m_tokenStr = token;
}

bool RecordsManagementConnection::communicate(enum ServiceId srvcId,
    const QByteArray &requestData, QByteArray &replyData)
{
	logFuncCall();

	QNetworkRequest request(createRequest(srvcId));

	QNetworkReply *reply = sendRequest(request, requestData);
	if (reply == Q_NULLPTR) {
		return false;
	}

	/* Set timeout timer */
	QTimer timer;
	timer.setSingleShot(true);
	QEventLoop eventLoop;
	connect(&timer, SIGNAL(timeout()), &eventLoop, SLOT(quit()));
	connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
	timer.start(m_timeOut);
	eventLoop.exec();

	qDebug("Loop exited, reply finished: %d", reply->isFinished());
	QList<QByteArray> headerList(reply->rawHeaderList());
	if (!reply->rawHeaderPairs().isEmpty()) {
		qDebug("%s", "Received raw headers:");
		foreach (const QNetworkReply::RawHeaderPair &pair,
		         reply->rawHeaderPairs()) {
			qDebug("%s: %s", pair.first.constData(),
			    pair.second.constData());
		}
	}

	bool retVal = false;
	replyData.clear();

	if (timer.isActive()) {
		timer.stop();
		retVal = processReply(reply, replyData);
		if (!retVal && (Q_NULLPTR != reply)) {
			emit connectionError(reply->errorString());
		}
	} else {
		/* Timeout expired. */
		disconnect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
		qCritical("Connection timed out. Check your internet connection.");
		reply->abort();
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

bool RecordsManagementConnection::addTrustedCertificate(const QString &filePath)
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
		qDebug("Read PEN certificate '%s'.",
		    filePath.toUtf8().constData());
		return true;
	} else {
		qWarning("Supplied certificate '%s' is not in PEM format.",
		    filePath.toUtf8().constData());
	}
	if (readAndAddCert(certData, QSsl::Der)) {
		qDebug("Read DER certificate '%s'.",
		    filePath.toUtf8().constData());
		return true;
	} else {
		qWarning("Supplied certificate '%s' is not in DER format.",
		    filePath.toUtf8().constData());
	}

	qCritical("Could not read certificate '%s'.",
	    filePath.toUtf8().constData());
	return false;
}

void RecordsManagementConnection::handleSslErrors(QNetworkReply *reply,
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

	qCritical("%s", errMsg.toUtf8().constData());
	emit connectionError(errMsg);

	if (m_ignoreSslErrors) {
		qWarning("Ignoring obtained SSL errors.");
		emit connectionError(QStringLiteral("Ignoring obtained SSL errors."));

		if (reply != Q_NULLPTR) {
			reply->ignoreSslErrors();
		}
	}
}

QNetworkRequest RecordsManagementConnection::createRequest(
    enum ServiceId srvcId) const
{
	logFuncCall();

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

QNetworkReply *RecordsManagementConnection::sendRequest(
    const QNetworkRequest &request, const QByteArray &data)
{
	logFuncCall();

	switch (m_nam.networkAccessible()) {
	case QNetworkAccessManager::UnknownAccessibility:
	case QNetworkAccessManager::NotAccessible:
		qCritical("%s",
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
		qCritical("%s", "No reply.");
		return Q_NULLPTR;
	}

	return reply;
}

bool RecordsManagementConnection::processReply(QNetworkReply *reply,
    QByteArray &replyData)
{
	logFuncCall();

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
		qWarning("Redirection '%s'?",
		    possibleRedirectUrl.toString().toUtf8().constData());
		// possibleRedirectUrl.toString();
		break;
	default: /* Any other error. */
		qCritical("%s", reply->errorString().toUtf8().constData());
		return false;
		break;
	}

	return true;
}
