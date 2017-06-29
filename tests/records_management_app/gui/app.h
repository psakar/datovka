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

#pragma once

#include <QMainWindow>
#include <QString>

#include "src/records_management/io/records_management_connection.h"
#include "src/records_management/models/upload_hierarchy_model.h"
#include "src/records_management/models/upload_hierarchy_proxy_model.h"
#include "ui_app.h"

/*!
 * @brief Main window for testing.
 */
class MainWindow : public QMainWindow, public Ui::MainWindow {
	Q_OBJECT
public:
	MainWindow(const QString &baseUrl, const QString &token,
	    const QString &caCertPath, bool ignoreSslErrors);

private slots:
	void filterHierarchy(const QString &text);

	void callSrvcServiceInfo(void);

	void callSrvcUploadHierarchy(void);

	void callSrvcUploadFile(void);

	void callSrvcStoredFiles(void);

	void callAbout(void);

	/*!
	 * @brief Enables upload file operation.
	 */
	void uploadHierarchySelectionChanged(const QItemSelection &selected,
	    const QItemSelection &deselected);

	/*!
	 * @brief Adds message into text output field.
	 */
	void writeMessage(const QString &message);

private:
	/*!
	 * @brief Connect top menu entries to appropriate actions.
	 */
	void connectTopMenuBarActions(void);

	/*!
	 * @brief Initialises actions.
	 */
	void initialiseActions(void);

	UploadHierarchyModel m_uploadModel;
	UploadHierarchyProxyModel m_uploadProxyModel;

	RecordsManagementConnection m_rmc; /*!< Connection to records management service. */
};
