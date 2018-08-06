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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
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

#include <QDialog>

namespace Ui {
	class DlgRecordsManagementProgress;
}

/*!
 * @brief Records management upload progress dialogue.
 */
class DlgRecordsManagementUploadProgress : public QDialog {
	Q_OBJECT

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] dmId Message identifier.
	 * @param[in] parent Parent widget.
	 */
	DlgRecordsManagementUploadProgress(qint64 dmId,
	    QWidget *parent = Q_NULLPTR);

	/*!
	 * !brief Destructor.
	 */
	~DlgRecordsManagementUploadProgress(void);

signals:
	void callAbort(void);

public slots:
	void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

	void onUploadProgress(qint64 bytesSent, qint64 bytesTotal);

private slots:
	void emitAbort(void);

private:
	/*!
	 * @brief Loads records management service logo and sets the logo label.
	 *
	 * @param[in] width Sets the image width (and height).
	 */
	void loadRecordsManagementPixmap(int width);

	Ui::DlgRecordsManagementProgress *m_ui; /*!< UI generated from UI file. */
};
