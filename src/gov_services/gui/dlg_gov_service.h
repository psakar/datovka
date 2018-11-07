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

#include <QDate>
#include <QDialog>
#include <QSet>
#include <QString>

#include "src/datovka_shared/gov_services/service/gov_service.h"
#include "src/datovka_shared/isds/message_interface.h"
#include "src/io/message_db_set.h"

namespace Ui {
	class DlgGovService;
}

/*!
 * @brief Encapsulated Gov service dialogue.
 */
class DlgGovService : public QDialog {
	Q_OBJECT

private:

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] userName Account user name.
	 * @param[in] gs Pointer holding Gov service.
	 * @param[in] dbSet Pointer holding account db set.
	 * @param[in] parent Parent widget.
	 */
	explicit DlgGovService(const QString &userName,
	    Gov::Service *gs, MessageDbSet *dbSet,
	    QWidget *parent = Q_NULLPTR);

public:

	/*!
	 * @brief Destructor.
	 */
	~DlgGovService(void);

	/*!
	 * @brief Open Gov service form dialogue.
	 *
	 * @param[in] userName Account user name.
	 * @param[in] gs Pointer holding Gov service.
	 * @param[in] dbSet Pointer holding account db set.
	 * @param[in] parent Parent widget.
	 */
	static
	void openGovServiceForm(const QString &userName,
	    Gov::Service *gs, MessageDbSet *dbSet,
	    QWidget *parent = Q_NULLPTR);

private slots:

	/*!
	 * @brief Check if all mandatory fields are filled.
	 */
	void haveAllMandatoryFields(void);

	/*!
	 * @brief Is active when line edit text has been changed.
	 *
	 * @param[in] text Text from textedit.
	 */
	void onLineEditTextChanged(QString text);

	/*!
	 * @brief Is active when date has been changed in the calendar widget.
	 *
	 * @param[in] date Date.
	 */
	void onDateChanged(QDate date);

	/*!
	 * @brief Create Gov message and send to isds.
	 */
	void onCreateAndSendMsg(void);

	/*!
	 * @brief Show status after sending Gov message via ISDS interface.
	 *
	 * @param[in] userName User name identifying the sender account.
	 * @param[in] transactId Unique transaction identifier.
	 * @param[in] result Sending status.
	 * @param[in] resultDesc Result description string.
	 * @param[in] dbIDRecipient Recipient data box identifier.
	 * @param[in] recipientName Recipient data box name.
	 * @param[in] isPDZ True if message was attempted to send as commercial
	 *                  message.
	 * @param[in] dmId Sent message identifier.
	 * @param[in] processFlags Message processig flags.
	 */
	void collectSendMessageStatus(const QString &userName,
	    const QString &transactId, int result, const QString &resultDesc,
	    const QString &dbIDRecipient, const QString &recipientName,
	    bool isPDZ, qint64 dmId, int processFlags);

private:

	/*!
	 * @brief Init Gov service dialogue.
	 */
	void initDialog(void);

	/*!
	 * @brief Generate form UI layout from service fields.
	 */
	void generateFormLayoutUi(void);

	/*!
	 * @brief Is active when some form field has invalid value.
	 *
	 * @param[in] errText Error text.
	 */
	void showValidityNotification(const QString &errText);

	/*!
	 * @brief Send Gov message to isds.
	 *
	 * @param[in] msg Message structure.
	 */
	void sendGovMessage(const Isds::Message &msg);

	QString m_userName; /*!< Account user name. */
	Gov::Service *m_gs; /*!< Pointer holding Gov service. */
	MessageDbSet *m_dbSet; /*!< Pointer holding account db set. */
	Ui::DlgGovService *m_ui; /*!< UI generated from UI file. */
	QSet<QString> m_transactIds; /*!< Temporary transaction identifiers. */
};
