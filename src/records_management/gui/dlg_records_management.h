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

#ifndef _DLG_RECORDS_MANAGEMENT_H_
#define _DLG_RECORDS_MANAGEMENT_H_

#include <QByteArray>
#include <QDialog>

#include "src/datovka_shared/records_management/io/records_management_connection.h"
#include "src/datovka_shared/settings/records_management.h"

namespace Ui {
	class DlgRecordsManagement;
}

/*!
 * @brief Encapsulated records management service settings dialogue.
 */
class DlgRecordsManagement : public QDialog {
	Q_OBJECT

private:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] urlStr String containing service location URL.
	 * @param[in] tokenStr String containing service token.
	 * @param[in] parent Parent widget.
	 */
	explicit DlgRecordsManagement(const QString &urlStr,
	    const QString &tokenStr, QWidget *parent = Q_NULLPTR);

public:
	/*!
	 * @brief Destructor.
	 */
	~DlgRecordsManagement(void);

	/*!
	 * @brief Update record management settings.
	 *
	 * @param[in] recMgmtSettings Settings structure to be modified.
	 * @param[in] parent Window parent widget.
	 * @return True when data have been updated, false else.
	 */
	static
	bool updateSettings(RecordsManagementSettings &recMgmtSettings,
	    QWidget *parent = Q_NULLPTR);

private slots:
	/*!
	 * @brief Enables service-related buttons.
	 */
	void activateServiceButtons(void);

	/*!
	 * @brief calls service info and displays results.
	 */
	void callServiceInfo(void);

	/*!
	 * @brief Erases all entries.
	 */
	void eraseContent(void);

	/*!
	 * @brief Notifies the user about communication error.
	 *
	 * @param[in] errMsg Error message.
	 */
	void notifyCommunicationError(const QString &errMsg);

private:
	/*!
	 * @brief Loads service information from storage.
	 */
	void loadStoredServiceInfo(void);

	/*!
	 * @brief Set window content according to service info response.
	 *
	 * @param[in] logoSvg Raw SVG data.
	 * @param[in] name Service name.
	 * @param[in] tokenName Access token name.
	 */
	void setResponseContent(const QByteArray &logoSvg, const QString &name,
	    const QString &tokenName);

	Ui::DlgRecordsManagement *m_ui; /*!< UI generated from UI file. */

	RecordsManagementConnection m_rmc; /*!< Connection to records management service. */
	QByteArray m_logoSvg; /*!< Raw SVG data. */
};

#endif /* _DLG_RECORDS_MANAGEMENT_H_ */
