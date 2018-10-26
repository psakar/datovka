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
#include <QString>

#include "src/gov_services/models/gov_form_list_model.h"
#include "src/worker/task_send_message.h"

namespace Ui {
	class DlgGovService;
}

/*!
 * @brief Encapsulated Gov service dialogue.
 */
class DlgGovService : public QDialog {
	Q_OBJECT

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] userName Account user name.
	 * @param[in] govFormModel Pointer holding Gov form model to be set.
	 * @param[in] dbSet Account db set pointer.
	 * @param[in] parent Parent widget.
	 */
	explicit DlgGovService(const QString &userName,
	    GovFormListModel *govFormModel, MessageDbSet *dbSet,
	    QWidget *parent = Q_NULLPTR);

	/*!
	 * @brief Destructor.
	 */
	~DlgGovService(void);

private slots:

	/*!
	 * @brief Send Gov request to isds.
	 */
	void sendGovRequest(void);

	/*!
	 * @brief Check if all mandatory fields are filled.
	 */
	void haveAllMandatoryFields(void);

private:

	/*!
	 * @brief Init service dialogue.
	 */
	void initDialog(void);

	QString m_userName; /*!< Account user name. */
	GovFormListModel *m_govFormModel; /*!< Pointer holding Gov form model. */
	MessageDbSet *m_dbSet; /*!< Account message database pointer. */
	Ui::DlgGovService *m_ui; /*!< UI generated from UI file. */

	/* Used to collect sending results. */
	QSet<QString> m_transactIds; /*!< Temporary transaction identifiers. */
	QList<TaskSendMessage::ResultData> m_sentMsgResultList; /*!< Send status list. */
};
