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

#ifndef _DLG_RECORDS_MANAGEMENT_STORED_H_
#define _DLG_RECORDS_MANAGEMENT_STORED_H_

#include <QDialog>
#include <QList>
#include <QString>

#include "src/datovka_shared/settings/records_management.h"

namespace Ui {
	class DlgRecordsManagementStored;
}

class MessageDbSet; /* Forward declaration. */

/*!
 * @brief Encapsulated records management service stored messages dialogue.
 */
class DlgRecordsManagementStored : public QDialog {
	Q_OBJECT

public:
	/*!
	 * @brief Describes account information.
	 */
	class AcntData {
	public:
		AcntData(const QString &aName, const QString &uName,
		    const MessageDbSet *dSet)
		    : accountName(aName), userName(uName), dbSet(dSet)
		{
		}

		QString accountName; /*!< Account name. */
		QString userName; /*!< User name (login). */
		const MessageDbSet *dbSet; /*!< Database set related to account. */
	};

private:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] urlStr String containing service location URL.
	 * @param[in] tokenStr String containing service token.
	 * @param[in] accounts List of accounts to take message identifiers from.
	 * @param[in] parent Parent widget.
	 */
	explicit DlgRecordsManagementStored(const QString &urlStr,
	    const QString &tokenStr, const QList<AcntData> &accounts,
	    QWidget *parent = Q_NULLPTR);

public:
	/*!
	 * @brief Destructor.
	 */
	~DlgRecordsManagementStored(void);

	/*!
	 * @brief Updates stored information about messages uploaded into
	 *     records management service.
	 *
	 * @param[in] recMgmtSettings Settings containing URL and token.
	 * @param[in] accounts List of accounts to take message identifiers from.
	 * @param[in] parent Parent widget.
	 */
	static
	bool updateStoredInformation(
	    const RecordsManagementSettings &recMgmtSettings,
	    const QList<AcntData> &accounts, QWidget *parent = Q_NULLPTR);

private slots:
	/*!
	 * @brief Starts the download action.
	 */
	void downloadAndStoreStart(void);

	/*!
	 * @brief Continues with the download action.
	 */
	void downloadAndStoreContinue(void);

	/*!
	 * @brief Set variable to cancel the download loop.
	 */
	void cancelLoop(void);

private:
	/*!
	 * @brief Loads records management service logo and sets the logo label.
	 *
	 * @param[in] width Sets the image width (and height).
	 */
	void loadRecordsManagementPixmap(int width);

	Ui::DlgRecordsManagementStored *m_ui; /*!< UI generated from UI file. */

	const QString m_url; /*!< Records management service URL. */
	const QString m_token; /*!< Records management service access token. */
	const QList<AcntData> &m_accounts; /*!< Account to use. */
	int m_accIdx; /*!< Index of next account to be processed. */

	int m_taskIncr; /*!< Task progress bar increment. */

	bool m_cancel; /*!< Set by slot to cancel loop. */
};

#endif /* _DLG_RECORDS_MANAGEMENT_STORED_H_ */
