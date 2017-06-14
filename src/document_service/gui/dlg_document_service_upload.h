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

#ifndef _DLG_DOCUMENT_SERVICE_UPLOAD_H_
#define _DLG_DOCUMENT_SERVICE_UPLOAD_H_

#include <QDialog>

#include "src/document_service/io/document_service_connection.h"
#include "src/document_service/models/upload_hierarchy_model.h"
#include "src/document_service/models/upload_hierarchy_proxy_model.h"
#include "src/settings/document_service.h"

namespace Ui {
	class DlgDocumentServiceUpload;
}

/*!
 * @brief Encapsulated document service upload dialogue.
 */
class DlgDocumentServiceUpload : public QDialog {
	Q_OBJECT

private:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] urlStr String containing service location URL.
	 * @param[in] tokenStr String containing service token.
	 * @param[in] parent Parent widget.
	 */
	explicit DlgDocumentServiceUpload(const QString &urlStr,
	    const QString &tokenStr, QWidget *parent = Q_NULLPTR);

public:
	/*!
	 * @brief Destructor.
	 */
	~DlgDocumentServiceUpload(void);

	/*!
	 * @brief Update document service settings.
	 *
	 * @param[in] docSrvcSettings Settings containing URL and token.
	 * @param[in] parent Window parent widget.
	 * @return True when data have been updated, false else.
	 */
	static
	bool uploadMessage(const DocumentServiceSettings &docSrvcSettings,
	    QWidget *parent = Q_NULLPTR);

private slots:
	/*!
	 * @brief Download upload hierarchy and set model.
	 */
	void callUploadHierarchy(void);

	/*!
	 * @brief Filter upload hierarchy.
	 *
	 * @param[in] text Filter text.
	 */
	void filterHierarchy(const QString &text);

	/*!
	 * @brief Notifies the user about communication error.
	 *
	 * @param[in] errMsg Error message.
	 */
	void notifyCommunicationError(const QString &errMsg);

private:

	Ui::DlgDocumentServiceUpload *m_ui; /*!< UI generated from UI file. */

	const QString m_url; /*!< Service URL. */
	const QString m_token; /*!< Service token. */

	DocumentServiceConnection m_dsc; /*!< Connection to document service. */

	UploadHierarchyModel m_uploadModel; /*!< Upload hierarchy model. */
	UploadHierarchyProxyModel m_uploadProxyModel; /*!< Used for filtering. */
};

#endif /* _DLG_DOCUMENT_SERVICE_UPLOAD_H_ */
