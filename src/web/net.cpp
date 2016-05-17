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


#include <QDebug>
#include <QEventLoop>
#include <QFileInfo>
#include <QInputDialog>

#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QNetworkRequest>
#include <QVariant>

#include "src/web/net.h"
#include "src/web/json.h"

QNetworkCookie cookie;
NetManager netmanager;

static
void printRequest(QNetworkRequest request, QByteArray requestContent)
{
	qDebug() << "";
	qDebug() << "====================REQUEST=========================";
	qDebug() << "URL:" << request.url().toString();
	qDebug() << "--------------------Headers-------------------------";
	QList<QByteArray> reqHeaders = request.rawHeaderList();
	foreach (const QByteArray &reqName, reqHeaders) {
		QByteArray reqValue = request.rawHeader(reqName);
		qDebug() << reqName << ":" << reqValue;
	}
	qDebug() << "--------------------Content------------------------";
	qDebug() << requestContent;
	qDebug() << "===================================================";
	qDebug() << "";
}


NetManager::NetManager(QObject *parent)
    : QObject(parent)
{
//	QFile f(COOKIE_FILE_NAME);
//	f.remove();
}

NetManager::~NetManager(void)
{
}

/* ========================================================================= */
/*
 * Func: Create POST request.
 */
bool NetManager::createPostRequest(const QUrl &url, const QByteArray &data,
   QByteArray &outData)
/* ========================================================================= */
{
	qDebug("%s()", __func__);

/*
	QFile f(COOKIE_FILE_NAME);
	QList<QNetworkCookie> list;
	f.open(QIODevice::ReadOnly);
	QDataStream s(&f);
	QByteArray c;
	while(!s.atEnd()){
		s >> c;
		list.append(QNetworkCookie::parseCookies(c));
	}
	f.close();
*/
	QByteArray appName(APP_NAME);
	QNetworkRequest request(url);
	request.setRawHeader("Host", url.host().toUtf8());
	request.setRawHeader("User-Agent", appName);
	request.setRawHeader("Accept", "application/json");
	request.setRawHeader("Connection", "keep-alive");
	request.setRawHeader("Content-Type", "application/json");
	request.setRawHeader("Content-Length", QByteArray::number(data.size()));
	if (!cookie.name().isEmpty()) {
		QVariant var;
		var.setValue(cookie);
		request.setHeader(QNetworkRequest::CookieHeader, var);
	}

#if 1
	printRequest(request, data);
#endif

	return sendRequest(request, data, outData, true);
}


/* ========================================================================= */
/*
 * Func: Create GET request.
 */
bool NetManager::createGetRequest(const QUrl &url, QByteArray &outData)
/* ========================================================================= */
{
	qDebug("%s()", __func__);

	QByteArray appName(APP_NAME);
	QNetworkRequest request(url);
	request.setRawHeader("Host", url.host().toUtf8());
	request.setRawHeader("User-Agent", appName);
	request.setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml");
	request.setRawHeader("Connection", "keep-alive");

	if (!cookie.name().isEmpty()) {
		QVariant var;
		var.setValue(cookie);
		request.setHeader(QNetworkRequest::CookieHeader, var);
	}

#if 1
	printRequest(request, QByteArray());
#endif

	return sendRequest(request, QByteArray(), outData, false);
}


/* ========================================================================= */
/*
 * Func: Send request.
 */
bool NetManager::sendRequest(QNetworkRequest &request,
    const QByteArray &data, QByteArray &outData, bool postRqst)
/* ========================================================================= */
{
	qDebug("%s()", __func__);

	bool ret = true;
	QNetworkReply *reply = NULL;
	QNetworkAccessManager *nam = new QNetworkAccessManager;

	if (postRqst) {
		reply = nam->post(request, data);
	} else {
		reply = nam->get(request);
	}

	/* TODO - add timer, set timeout from settings, run,
	 * abort request if timer = timeout
	 */

	/* this eventloop makes synchronous soap requests */
	QEventLoop eventLoop;
	QObject::connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
	eventLoop.exec();

	ret = getResponse(reply, outData);

	reply->deleteLater();
	nam->deleteLater();

	return ret;
}


/* ========================================================================= */
/*
 * Func: Response.
 */
bool NetManager::getResponse(QNetworkReply *reply, QByteArray &outData)
/* ========================================================================= */
{
	qDebug("%s()", __func__);

	bool ret = true;

	int statusCode = reply->attribute(
	    QNetworkRequest::HttpStatusCodeAttribute).toInt();
	const QString url = reply->url().toString();
	const QString error = reply->errorString();
	const QString reason = reply->attribute(
	    QNetworkRequest::HttpReasonPhraseAttribute).toString();

#if 1
	qDebug() << "";
	qDebug() << "====================REPLY===========================";
	qDebug() << "URL:" << url;
	qDebug() << "CODE:" << statusCode;
	if (reply->error() != QNetworkReply::NoError) {
		qDebug() << "ERROR:" << error;
	}
	qDebug() << "REASON:" << reason;
	qDebug() << "--------------------Headers-------------------------";
	QList<QByteArray> reqHeaders = reply->rawHeaderList();
	foreach (QByteArray reqName, reqHeaders) {
		QByteArray reqValue = reply->rawHeader(reqName);
		qDebug() << reqName << ":" << reqValue;
	}
	qDebug() << "--------------------Content------------------------";
#endif

	switch (statusCode) {

	case 200: /* HTTP status 200 OK */
		{
			outData = reply->readAll();
		}
		break;

	case 302: /* HTTP status 302 Found */
		{
			QVariant variantCookies =
			    reply->header(QNetworkRequest::SetCookieHeader);
			QList<QNetworkCookie> list =
			    qvariant_cast<QList<QNetworkCookie> >(variantCookies);

			//QFile f(COOKIE_FILE_NAME);
			//f.open(QIODevice::ReadWrite);
			for (int i = 0; i < list.size(); ++i) {
				if (list.at(i).name() == COOKIE_SESSION_ID) {
					cookie = list.at(i);
				}
				//QDataStream s(&f);
				//s << list.at(i).toRawForm();
			}
			//f.close();
			//qurl = QUrl::fromPercentEncoding(reply->rawHeader("Location"));
		}
		break;

	case 401: /* HTTP status 401 Unauthorized */
	case 503: /* HTTP status 503 Service Temporarily Unavailable */
	default: /* Any error occurred */
		outData = QByteArray();
		break;
	}

	return ret;
}
