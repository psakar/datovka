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
#include <QMap>
#include <QModelIndex>
#include <QString>

#include "src/gov_services/models/gov_service_list_model.h"
#include "src/io/message_db_set.h"
#include "src/models/sort_filter_proxy_model.h"

namespace Gov {
	class Service; /* Forward declaration. */
}

namespace Ui {
	class DlgGovServices;
}

/*!
 * @brief Encapsulated e-gov service list dialogue.
 */
class DlgGovServices : public QDialog {
	Q_OBJECT

private:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] userName Account login.
	 * @param[in] dbSet Account db set pointer.
	 * @param[in] parent Parent widget.
	 */
	explicit DlgGovServices(const QString &userName, MessageDbSet *dbSet,
	    QWidget *parent = Q_NULLPTR);

public:
	/*!
	 * @brief Destructor.
	 */
	virtual
	~DlgGovServices(void);

	/*!
	 * @brief Open dialogue and show e-gov services.
	 *
	 * @param[in] userName Account login.
	 * @param[in] dbSet Account db set pointer.
	 * @param[in] parent Parent widget.
	 */
	static
	void showGovServices(const QString &userName, MessageDbSet *dbSet,
	    QWidget *parent = Q_NULLPTR);

private slots:
	/*!
	 * @brief Apply filter text in the e-gov service list.
	 *
	 * @param[in] text Filter text.
	 */
	void filterServices(const QString &text);

	/*!
	 * @brief Any e-gov service was double clicked.
	 *
	 * @param[in] index model index of selected service.
	 */
	void onServiceActivated(const QModelIndex &index);

private:
	/*!
	 * @brief Load e-gov services into model.
	 */
	void loadServicesToModel(void);

	Ui::DlgGovServices *m_ui; /*!< UI generated from UI file. */

	QString m_userName; /*!< Account user name. */
	MessageDbSet *m_dbSet; /*!< Holds pointer to message database. */
	QMap<QString, const Gov::Service *> m_govServices; /*!< Holds pointers to all available e-gov services. */
	SortFilterProxyModel m_govServiceListProxyModel; /*!< Used for e-gov service filtering. */
	GovServiceListModel m_govServiceModel; /*!< E-gov service model. */
};
