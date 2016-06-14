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

#ifndef _NET_H_
#define _NET_H_

#include <QObject>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkCookie>
#include <QUrl>

#include "src/web/net_consts.h"

class NetManager : public QObject {
	Q_OBJECT

public:
	NetManager(QObject *parent = 0);
	~NetManager(void);

public:

	QString newUrl;

	/*!
	 * @brief Create POST request to WebDatovka.
	 *
	 * @param[in] url       - url of request.
	 * @param[in] sessionid - cookie of request.
	 * @param[in] data      - request content data.
	 * @param[out] outData  - reply data.
	 * @return true if success.
	 */
	bool createPostRequestWebDatovka(const QUrl &url,
	    const QNetworkCookie &sessionid, const QByteArray &data,
	    QByteArray &outData);

	/*!
	 * @brief Create POST request for file sending to WebDatovka.
	 *
	 * @param[in] url       - url of request.
	 * @param[in] sessionid - cookie of request.
	 * @param[in] draftId   - draftId.
	 * @param[in] filename  - file name.
	 * @param[in] filedata  - file content data.
	 * @param[out] outData  - reply data.
	 * @return true if success.
	 */
	bool createPostRequestWebDatovkaSendFile(const QUrl &url,
	    const QNetworkCookie &sessionid, int &draftId,
	    const QString &filename, const QByteArray &filedata,
	    QByteArray &outData);

	/*!
	 * @brief Create GET request to WebDatovka.
	 *
	 * @param[in] url       - url of request.
	 * @param[in] sessionid - cookie of request.
	 * @param[out] outData  -reply data.
	 * @return true if success.
	 */
	bool createGetRequestWebDatovka(const QUrl &url,
	    const QNetworkCookie &sessionid, QByteArray &outData);

	/*!
	 * @brief Create POST request to MojeID.
	 *
	 * @param[in] url      - current url of request.
	 * @param[in] prevUrl  - previous url.
	 * @param[in] data     - request content data.
	 * @param[out] outData - reply data.
	 * @return true if success.
	 */
	bool createPostRequestMojeId(const QUrl &url, const QUrl &prevUrl,
	    const QByteArray &data, QByteArray &outData);

	/*!
	 * @brief Create GET request to MojeID.
	 *
	 * @param[in] url      - current url of request.
	 * @param[in] prevUrl  - previous url.
	 * @param[out] outData - reply data.
	 * @return true if success.
	 */
	bool createGetRequestMojeId(const QUrl &url, const QUrl &prevUrl,
	    QByteArray &outData);

private:

	/*!
	 * @brief Send request and run eventloop.
	 *
	 * @param[in] request   - hold request data.
	 * @param[in] data      - hold content data (may be NULL).
	 * @param[out] outData  - reply data.
	 * @param[in] postRqst  - it is POST request (POST = TRUE).
	 * @return true if success.
	 */
	bool sendRequest(QNetworkRequest &request, const QByteArray &data,
	    QByteArray &outData, bool postRqst);

	/*!
	 * @brief Parse response.
	 *
	 * @param[in] reply    - pointer on the reply data.
	 * @param[out] outData - reply data.
	 * @return true if success.
	 */
	bool getResponse(QNetworkReply *reply, QByteArray &outData);
};

extern QList<QNetworkCookie> cookieList;
extern NetManager netmanager;

#endif /* _NET_H_ */
