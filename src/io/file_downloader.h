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


#ifndef _FILE_DOWNLOADER_H_
#define _FILE_DOWNLOADER_H_


#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QUrl>


/*!
 * @brief Encapsulates network connection.
 *
 * @note The class uses its own event loop so its instances can be used outside
 *     the main event loop.
 */
class FileDownloader : public QObject {
	Q_OBJECT

signals:
	/*!
	 * @brief This signal is emitted to signalise the event loop to quit.
	 */
	void quitLoop(void);

public:
	/*!
	 * @brief Constructor.
	 */
	explicit FileDownloader(bool useGlobalProxySettings,
	    QObject *parent = 0);

	/*!
	 * @brief Download a file. The function iterates through the list of
	 *    URLs.
	 *
	 * @param[in] urlList List of URLs to try.
	 * @param[in] msec    Timeout for a single connection.
	 * @return Downloaded data. Data are empty on error.
	 */
	QByteArray download(const QList<QUrl> &urlList, int msec);


	/*!
	 * @brief Set up a HTTP proxy according to global settings.
	 *
	 * @param[in,out] netMngr Network access manager to be set up.
	 * @return True if proxy has been set up.
	 */
	static
	bool setUpHttpProxyAccordingToGlobals(QNetworkAccessManager &netMngr);

private slots:
	/*!
	 * @brief Process received reply.
	 *
	 * @param[in] reply Received reply.
	 */
	void fileDownloaded(QNetworkReply *reply);

	/*!
	 * @brief Called if connection times out.
	 */
	void downloadTimeout(void);

private:
	/*!
	 * @brief Processes next URL in list.
	 */
	void startPendingDownload(void);

	QNetworkAccessManager m_netMngr;
	QByteArray m_downloadedData;
	QList<QUrl> m_urlList;
	int m_urlIdx;
	int m_msec;
	QNetworkReply *m_currentReply;
};


#endif /* _FILE_DOWNLOADER_H_ */
