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

#include "src/gov_services/models/gov_form_list_model.h"
#include "src/gov_services/models/gov_service_list_model.h"
#include "src/io/message_db_set.h"
#include "src/models/sort_filter_proxy_model.h"

namespace Ui {
	class DlgGovServices;
}

namespace Gov {
	class Service; /* Forward declaration. */
}

/*!
 * @brief Encapsulated gov services dialogue.
 */
class DlgGovServices : public QDialog {
	Q_OBJECT

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] userName Account login.
	 * @param[in] dbSet Account db set pointer.
	 * @param[in] parent Parent widget.
	 */
	explicit DlgGovServices(const QString &userName, MessageDbSet *dbSet,
	    QWidget *parent = Q_NULLPTR);

	/*!
	 * @brief Destructor.
	 */
	~DlgGovServices(void);

private slots:

	/*!
	 * @brief Apply filter text on the Gov service list.
	 *
	 * @param[in] text Filter text.
	 */
	void filterServices(const QString &text);

	/*!
	 * @brief Any Gov service was double clicked.
	 *
	 * @param[in] index model index of selected service.
	 */
	void onServiceActivated(const QModelIndex &index);

private:

	/*!
	 * @brief Init all Gov services and insert to map.
	 */
	void initGovServices(void);

	/*!
	 * @brief Remove all Gov services from map and delete their objects.
	 */
	void clearGovServices(void);

	/*!
	 * @brief Load Gov services into model.
	 */
	void loadServicesToModel(void) const;

	/*!
	 * @brief Load Gov form data into model.
	 *
	 * @param[in] userName Account login.
	 * @param[in] serviceId Internal service identifier.
	 */
	void loadFormToModel(const QString &userName,
	    const QString &serviceId) const;

	QString m_userName; /*!< Account user name. */
	MessageDbSet *m_dbSet; /*!< Account message database poniter. */
	QMap<QString, const Gov::Service *> m_govServices; /*!< Holds pointers to all available Gov services. */
	SortFilterProxyModel m_govServiceListProxyModel; /*!< Used for Gov service filtering. */
	GovServiceListModel *m_govServiceModel; /*!< Gov service model. */
	GovFormListModel *m_govFormModel; /*!< Gov service form model. */
	Ui::DlgGovServices *m_ui; /*!< UI generated from UI file. */
};
