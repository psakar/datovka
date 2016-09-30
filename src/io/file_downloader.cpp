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


#include <QEventLoop>
#include <QNetworkProxy>
#include <QTimer>

#include "src/io/file_downloader.h"
#include "src/log/log.h"
#include "src/settings/proxy.h"


/* ========================================================================= */
/*
 * Constructor.
 */
FileDownloader::FileDownloader(bool useGlobalProxySettings, QObject *parent)
/* ========================================================================= */
    : QObject(parent),
    m_urlIdx(0),
    m_msec(0),
    m_currentReply(0)
{
	connect(&m_netMngr, SIGNAL(finished(QNetworkReply *)),
	    this, SLOT(fileDownloaded(QNetworkReply *)));

	if (useGlobalProxySettings) {
		if (setUpHttpProxyAccordingToGlobals(m_netMngr)) {
			logDebugLv0NL("%s", "Using proxy.");
		} else {
			logDebugLv0NL("%s", "Using no proxy.");
		}
	} else {
		logDebugLv0NL("%s", "Forcing no proxy.");
	}
}


/* ========================================================================= */
/*
 * Download a file. The function iterates through the list of
 *    URLs.
 */
QByteArray FileDownloader::download(const QList<QUrl> &urlList, int msec)
/* ========================================================================= */
{
	debugFuncCall();

	m_downloadedData.clear();
	m_msec = msec;
	m_urlList = urlList;
	m_currentReply = 0;

	if (0 == urlList.size()) {
		return QByteArray();
	}

	QEventLoop loop;

	connect(this, SIGNAL(quitLoop()), &loop, SLOT(quit()),
	    Qt::QueuedConnection);

	m_urlIdx = 0;

	startPendingDownload();

	loop.exec();

	return m_downloadedData;
}


/* ========================================================================= */
/*
 * Set up a HTTP proxy according to global settings.
 */
bool FileDownloader::setUpHttpProxyAccordingToGlobals(
    QNetworkAccessManager &netMngr)
/* ========================================================================= */
{
	ProxiesSettings::ProxySettings proxySettings;
	QNetworkProxy proxy;

	/* Use proxy according to configuration. */
	proxySettings = globProxSet.proxySettings(ProxiesSettings::HTTP);

	/* If something detected or set up. */
	if (ProxiesSettings::ProxySettings::NO_PROXY != proxySettings.usage) {
		proxy.setHostName(proxySettings.hostName);
		proxy.setPort(proxySettings.port);
//		proxy.setType(QNetworkProxy::DefaultProxy);
//		proxy.setType(QNetworkProxy::Socks5Proxy);
		proxy.setType(QNetworkProxy::HttpProxy);
		if (!proxySettings.userName.isEmpty()) {
			proxy.setUser(proxySettings.userName);
		}
		if (!proxySettings.password.isEmpty()) {
			proxy.setPassword(proxySettings.password);
		}

		netMngr.setProxy(proxy);

		logDebugLv0NL("Using proxy host='%s' port='%d' "
		    "user='%s' password='%s'",
		    proxy.hostName().toUtf8().constData(),
		    proxy.port(),
		    proxy.user().isEmpty() ? "" :
		        proxy.user().toUtf8().constData(),
		    proxy.password().isEmpty() ? "" :
		        proxy.password().toUtf8().constData());

		return true;
	}

	return false;
}


/* ========================================================================= */
/*
 * Process received reply.
 */
void FileDownloader::fileDownloaded(QNetworkReply *reply)
/* ========================================================================= */
{
	debugFuncCall();

	if (0 == reply) {
		Q_ASSERT(0);
		return;
	}

	m_downloadedData = reply->readAll();
	/* Emit a signal. */
	reply->deleteLater();
	emit quitLoop();
}


/* ========================================================================= */
/*
 * Called if connection times out.
 */
void FileDownloader::downloadTimeout(void)
/* ========================================================================= */
{
	debugFuncCall();

	if (m_urlIdx < m_urlList.size()) {
		/* Start download with next url. */
		Q_ASSERT(0 != m_currentReply);
		if ((0 != m_currentReply) && !m_currentReply->isFinished()) {
			m_currentReply->close();
			m_currentReply->deleteLater();
			m_currentReply = 0;
			startPendingDownload();

			return;
		}
	}

	emit quitLoop();
}


/* ========================================================================= */
/*
 * Processes next URL in list.
 */
void FileDownloader::startPendingDownload(void)
/* ========================================================================= */
{
	debugFuncCall();

	if (m_urlList.size() <= m_urlIdx) {
		return;
	}

	m_currentReply = m_netMngr.get(QNetworkRequest(m_urlList[m_urlIdx++]));

	QTimer::singleShot(m_msec, this, SLOT(downloadTimeout()));
}
