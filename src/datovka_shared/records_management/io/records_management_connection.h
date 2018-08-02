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

#pragma once

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QString>

/*!
 * @brief Encapsulates connection to records management service.
 */
class RecordsManagementConnection : public QObject {
	Q_OBJECT

public:
	/*!
	 * @brief Records management service identifiers.
	 */
	enum ServiceId {
		SRVC_SERVICE_INFO,
		SRVC_UPLOAD_HIERARCHY,
		SRVC_UPLOAD_FILE,
		SRVC_STORED_FILES
	};

	/*!
	 * @brief Use for controlling of global behaviour on SSL errors.
	 */
	static
	const bool ignoreSslErrorsDflt;

	/*!
	 * @brief Constructor.
	 */
	explicit RecordsManagementConnection(bool ignoreSslErrors = false,
	    QObject *parent = Q_NULLPTR);

	/*!
	 * @brief Set connection data.
	 *
	 * @param[in] baseUrl Service base URL.
	 * @param[in] token Authentication token.
	 */
	void setConnection(const QString &baseUrl, const QString &token);

	/*!
	 * @brief Send request and wait for reply.
	 *
	 * @param[in] srvcId Srvice identifier.
	 */
	bool communicate(enum ServiceId srvcId, const QByteArray &requestData,
	   QByteArray &replyData);

	/*!
	 * @brief Add certificate to certificate store.
	 *
	 * @param[in] filePath Path to certificate file.
	 * @return True on success.
	 */
	static
	bool addTrustedCertificate(const QString &filePath);

signals:
	/*!
	 * @brief Emitted when some error during communication occurs.
	 *
	 * @param[in] message Message string containing error description.
	 */
	void connectionError(const QString &message);

private slots:
	void handleSslErrors(QNetworkReply *reply,
	    const QList<QSslError> &errors);

private:
	/*!
	 * @brief Create network request.
	 *
	 * @param[in] srvcId Srvice identifier.
	 * @return Created network request.
	 */
	QNetworkRequest createRequest(enum ServiceId srvcId) const;

	/*!
	 * @brief Send request.
	 *
	 * @param[in] request Network request.
	 * @param[in] data Data to be sent along with the request.
	 * @return Null pointer on failure.
	 */
	QNetworkReply *sendRequest(const QNetworkRequest &request,
	    const QByteArray &data);

	/*!
	 * @brief Blocks until all data are sent and received or until timed out.
	 *
	 * @note The reply is aborted when it times out. In this case the reply
	 *     is not deleted.
	 *
	 * @param[in,out] reply Communication context.
	 * @param[in]     timeOut Communication timeout.
	 * @return True if all data have been received,
	 *     false if communication timed out.
	 */
	static
	bool waitReplyFinished(QNetworkReply *reply, unsigned int timeOut);

	/*!
	 * @brief Process reply data.
	 *
	 * @param[in,out] reply Obtained reply.
	 * @param[out]    replyData Obtained reply data.
	 * @return True on success, false on error.
	 */
	static
	bool processReply(QNetworkReply *reply, QByteArray &replyData);

	QString m_baseUrlStr; /*!< Service base URL. */
	QString m_tokenStr; /*!< Authentication token. */
	QString m_agentName; /*!< Usually the application name. */

	unsigned int m_timeOut; /*!< Communication timeout. */
	bool m_ignoreSslErrors; /*!< True if SSL errors should be ignored. */
	QNetworkAccessManager m_nam; /*!< Network access manager. */
};
