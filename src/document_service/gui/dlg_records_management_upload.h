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

#ifndef _DLG_RECORDS_MANAGEMENT_UPLOAD_H_
#define _DLG_RECORDS_MANAGEMENT_UPLOAD_H_

#include <QByteArray>
#include <QDialog>
#include <QString>
#include <QStringList>

#include "src/document_service/io/records_management_connection.h"
#include "src/document_service/models/upload_hierarchy_model.h"
#include "src/document_service/models/upload_hierarchy_proxy_model.h"
#include "src/settings/records_management.h"

namespace Ui {
	class DlgRecordsManagementUpload;
}

/*!
 * @brief Encapsulated records management service upload dialogue.
 */
class DlgRecordsManagementUpload : public QDialog {
	Q_OBJECT

private:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] urlStr String containing service location URL.
	 * @param[in] tokenStr String containing service token.
	 * @param[in] dmId Message identifier.
	 * @param[in] parent Parent widget.
	 */
	explicit DlgRecordsManagementUpload(const QString &urlStr,
	    const QString &tokenStr, qint64 dmId, QWidget *parent = Q_NULLPTR);

public:
	/*!
	 * @brief Destructor.
	 */
	~DlgRecordsManagementUpload(void);

	/*!
	 * @brief Upload message into records management service.
	 *
	 * @param[in] recMgmtSettings Settings containing URL and token.
	 * @param[in] dmId Message identifier.
	 * @param[in] msgFileName Message file name.
	 * @param[in] msgData Message data.
	 * @param[in] parent Window parent widget.
	 * @return True when data have been updated, false else.
	 */
	static
	bool uploadMessage(const RecordsManagementSettings &recMgmtSettings,
	    qint64 dmId, const QString &msgFileName, const QByteArray &msgData,
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
	 * @brief Checks selection and activates OK button.
	 */
	void uploadHierarchySelectionChanged(void);

	/*!
	 * @brief Notifies the user about communication error.
	 *
	 * @param[in] errMsg Error message.
	 */
	void notifyCommunicationError(const QString &errMsg);

private:
	/*!
	 * @brief Loads records management service logo and sets the logo label.
	 *
	 * @param[in] width Sets the image width (and height).
	 */
	void loadRecordsManagementPixmap(int width);

	/*!
	 * @brief Upload file into records management service.
	 *
	 * @param[in,out] rmc Connection object.
	 * @param[in]     uploadIds Upload location identifiers.
	 * @param[in]     dmId Message identifier.
	 * @param[in]     msgFileName Message file name.
	 * @param[in]     msgData Message data.
	 * @param[in]     parent Window parent widget.
	 * @return True on success.
	 */
	static
	bool uploadFile(RecordsManagementConnection &rmc, qint64 dmId,
	    const QStringList &uploadIds, const QString &msgFileName,
	    const QByteArray &msgData, QWidget *parent = Q_NULLPTR);

	Ui::DlgRecordsManagementUpload *m_ui; /*!< UI generated from UI file. */

	const QString m_url; /*!< Service URL. */
	const QString m_token; /*!< Service token. */

	RecordsManagementConnection m_rmc; /*!< Connection to records management service. */

	UploadHierarchyModel m_uploadModel; /*!< Upload hierarchy model. */
	UploadHierarchyProxyModel m_uploadProxyModel; /*!< Used for filtering. */

	QStringList m_selectedUploadIds; /*!< Upload location identifiers. */
};

#endif /* _DLG_RECORDS_MANAGEMENT_UPLOAD_H_ */
