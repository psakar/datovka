

#include <QEventLoop>
#include <QNetworkProxy>
#include <QTimer>

#include "src/common.h"
#include "src/io/file_downloader.h"
#include "src/log/log.h"


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
		ProxiesSettings::ProxySettings proxySettings;
		QNetworkProxy proxy;

		if (ProxiesSettings::autoProxyStr ==
		    globProxSet.http.hostName) {
			proxySettings = ProxiesSettings::detectHttpProxy();

			/*
			 * TODO -- Is it better to handle
			 * proxyAuthenticationRequired() ?
			 */

			/*
			 * Add user name and password if those were not
			 * detected but were supplied by the user.
			 */
			if (proxySettings.userName.isEmpty()) {
				proxySettings.userName =
				    globProxSet.http.userName;
			}
			if (proxySettings.password.isEmpty()) {
				proxySettings.password =
				    globProxSet.http.password;
			}
		} else if (!globProxSet.http.hostName.isEmpty() &&
		           (globProxSet.http.port >= 0)) {
			proxySettings = globProxSet.http;
		}

		/* If something detected or set up. */
		if (ProxiesSettings::noProxyStr != proxySettings.hostName) {
			proxy.setHostName(proxySettings.hostName);
			proxy.setPort(proxySettings.port);
//			proxy.setType(QNetworkProxy::DefaultProxy);
//			proxy.setType(QNetworkProxy::Socks5Proxy);
			proxy.setType(QNetworkProxy::HttpProxy);
			if (!proxySettings.userName.isEmpty()) {
				proxy.setUser(proxySettings.userName);
			}
			if (!proxySettings.password.isEmpty()) {
				proxy.setPassword(proxySettings.password);
			}

			m_netMngr.setProxy(proxy);

			logDebugLv0NL("Using proxy host='%s' port='%d' "
			    "user='%s' password='%s'",
			    proxy.hostName().toStdString().c_str(),
			    proxy.port(),
			    proxy.user().isEmpty() ? "" :
			        proxy.user().toStdString().c_str(),
			    proxy.password().isEmpty() ? "" :
			        proxy.password().toStdString().c_str());
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
 * Process received reply.
 */
void FileDownloader::fileDownloaded(QNetworkReply *reply)
/* ========================================================================= */
{
	debugFuncCall();

	Q_ASSERT(0 != reply);
	if (0 == reply) {
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
