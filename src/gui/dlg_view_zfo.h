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

#pragma once

#include <QDialog>

#include "src/datovka_shared/isds/message_interface.h"
#include "src/isds/message_functions.h"
#include "src/models/attachments_model.h"

namespace Ui {
	class DlgViewZfo;
}

/*!
 * @brief Dialogue for ZFO content viewing.
 */
class DlgViewZfo : public QDialog {
	Q_OBJECT

private:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] message Message.
	 * @param[in] zfoType Specifies type of data.
	 * @param[in] errMsg Message to be displayed if null message passed.
	 * @param[in] parent Parent widget.
	 */
	DlgViewZfo(const Isds::Message &message, enum Isds::LoadType zfoType,
	    const QString &errMsg, QWidget *parent = Q_NULLPTR);

public:
	/*!
	 * @brief Destructor.
	 */
	~DlgViewZfo(void);

	/*!
	 * @brief View content of ZFO file.
	 *
	 * @param[in] zfoFileName Name of ZFO file.
	 * @param[in] parent Parent widget.
	 */
	static
	void view(const QString &zfoFileName, QWidget *parent = Q_NULLPTR);

	/*!
	 * @brief View content of ZFO.
	 *
	 * @param[in] zfoData ZFO content.
	 * @param[in] parent Parent widget.
	 */
	static
	void view(const QByteArray &zfoData, QWidget *parent = Q_NULLPTR);

private slots:
	/*!
	 * @brief Generates menu to selected message item.
	 *
	 * @param[in] point Right-click position.
	 */
	void attachmentItemRightClicked(const QPoint &point);

	/*!
	 * @brief Saves selected attachment to file.
	 */
	void saveSelectedAttachmentsToFile(void);

	/*!
	 * @brief Saves selected attachments to directory.
	 */
	void saveSelectedAttachmentsIntoDirectory(void);

	/*!
	 * @brief Open attachment in default application.
	 */
	void openSelectedAttachment(const QModelIndex &index = QModelIndex());

	/*!
	 * @brief View signature details dialogue.
	 */
	void showSignatureDetailsDlg(void);

private:
	/*!
	 * @brief Loads ZFO data.
	 *
	 * @param[in]  zfoData Raw ZFO data.
	 * @param[out] message Newly created message.
	 * @param[out] zfoType Detected ZFO file type.
	 * @return True on success, false on failure.
	 */
	static
	bool parseZfoData(const QByteArray &zfoData,
	    Isds::Message &message, enum Isds::LoadType &zfoType);

	/*!
	 * @brief Loads ZFO file.
	 *
	 * @param[in]  zfoFileName Path to the ZFO file.
	 * @param[out] message Newly created message.
	 * @param[out] zfoType Detected ZFO file type.
	 * @return True on success, false on failure.
	 */
	static
	bool parseZfoFile(const QString &zfoFileName,
	    Isds::Message &message, enum Isds::LoadType &zfoType);

	/*!
	 * @brief Performs dialogue set-up after the message has been loaded.
	 */
	void setUpDialogue(void);

	/*!
	 * @brief Generate description from supplied message.
	 *
	 * @param[in] attachmentCount Number of attached files.
	 * @param[in] msgDER Raw message data.
	 * @param[in] tstDer Time stamp data.
	 * @return String containing description in HTML format.
	 */
	QString messageDescriptionHtml(int attachmentCount,
	    const QByteArray &msgDER, const QByteArray &tstDER) const;

	/*!
	 * @brief Generate description for supplied delivery information.
	 *
	 * @param[in] msgDER Raw message data.
	 * @param[in] tstDer Time stamp data.
	 * @return String containing description in HTML format.
	 */
	QString deliveryDescriptionHtml(const QByteArray &msgDER,
	    const QByteArray &tstDER) const;

	/*!
	 * @brief Generates header description according to the supplied
	 *     envelope.
	 *
	 * @param[out] html HTML text to which the data should be appended.
	 * @param[in]  envelope Message envelope.
	 * @return True on success.
	 */
	static
	bool envelopeHeaderDescriptionHtml(QString &html,
	    const Isds::Envelope &envelope);

	/*!
	 * @brief Generates footer description according to the supplied
	 *     message data.
	 *
	 * @param[out] html HTML text to which the data should be appended.
	 * @param[in]  msgDER Raw message data.
	 * @param[in]  tstDer Time stamp data.
	 * @return True on success.
	 */
	static
	bool signatureFooterDescription(QString &html, const QByteArray &msgDER,
	    const QByteArray &tstDER);

	Ui::DlgViewZfo *m_ui; /*!< UI generated from UI file. */

	const Isds::Message &m_message; /*!< Message reference. */
	/*
	 * (char *) m_message->raw
	 *     m_message->raw_length
	 * (char *) m_message->envelope->timestamp
	 *     m_message->envelope->timestamp_length
	 */

	enum Isds::LoadType m_zfoType; /*!< Type of message. */
	AttachmentTblModel m_attachmentModel; /*!< Attachment model. */
};
