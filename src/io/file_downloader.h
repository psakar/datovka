

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
