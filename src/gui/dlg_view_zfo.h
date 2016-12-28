/*
 * Copyright (C) 2014-2016 CZ.NIC
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

#ifndef _DLG_VIEW_ZFO_H_
#define _DLG_VIEW_ZFO_H_


#include <QDialog>

#include "src/models/files_model.h"
#include "ui_dlg_view_zfo.h"

/*!
 * @brief Dialog for ZFO content viewing.
 */
class DlgViewZfo : public QDialog, public Ui::ViewZfo {
	Q_OBJECT

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] zfoFileName Path to the ZFO file.
	 * @param[in] parent      Pointer to parent object.
	 */
	explicit DlgViewZfo(const QString &zfoFileName, QWidget *parent = 0);

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] zfoData Raw ZFO data.
	 * @param[in] parent      Pointer to parent object.
	 */
	explicit DlgViewZfo(const QByteArray &zfoData, QWidget *parent = 0);

	/*!
	 * @brief Destructor.
	 */
	~DlgViewZfo(void);

	/*
	 * TODO -- Signature checking.
	 */

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
	 * @brief View signature details.
	 */
	void showSignatureDetails(void);

private:
	/*!
	 * @brief Loads ZFO data.
	 *
	 * @param[in] zfoData Raw ZFO data.
	 */
	void parseZfoData(const QByteArray &zfoData);

	/*!
	 * @brief Loads ZFO file.
	 *
	 * @param[in] zfoFileName Path to the ZFO file.
	 */
	void parseZfoFile(const QString &zfoFileName);

	/*!
	 * @brief Performs dialogue set-up after the message has been loaded.
	 */
	void setUpDialogue(void);

	/*!
	 * @brief Generate description from supplied message.
	 *
	 * @param[in] attachmentCount Number of attached files.
	 * @param[in] msgDER          Raw message data.
	 * @param[in] msgSize         Raw message size.
	 * @param[in] tstDer          Time stamp data.
	 * @param[in] tstSize         Time stamp size.
	 * @return String containing description in HTML format.
	 */
	QString messageDescriptionHtml(int attachmentCount,
	    const void *msgDER, size_t msgSize,
	    const void *tstDER, size_t tstSize) const;

	/*!
	 * @brief Generate description for supplied delivery information.
	 *
	 * @param[in] msgDER          Raw message data.
	 * @param[in] msgSize         Raw message size.
	 * @param[in] tstDer          Time stamp data.
	 * @param[in] tstSize         Time stamp size.
	 * @return String containing description in HTML format.
	 */
	QString deliveryDescriptionHtml(const void *msgDER, size_t msgSize,
	    const void *tstDER, size_t tstSize) const;

	/*!
	 * @brief Generates header description according to the supplied
	 *     envelope.
	 *
	 * @param[out] html     HTML text to which the data should be appended.
	 * @param[in]  envelope Message envelope structure.
	 * @return True on success.
	 */
	static
	bool envelopeHeaderDescriptionHtml(QString &html,
	    const struct isds_envelope *envelope);

	/*!
	 * @brief Generates footer description according to the supplied
	 *     message data.
	 *
	 * @param[out] html     HTML text to which the data should be appended.
	 * @param[in]  msgDER   Raw message data.
	 * @param[in]  msgSize  Raw message size.
	 * @param[in]  tstDer   Time stamp data.
	 * @param[in]  tstSize  Time stamp size.
	 * @return True on success.
	 */
	static
	bool signatureFooterDescription(QString &html, const void *msgDER,
	    size_t msgSize, const void *tstDER, size_t tstSize);

	struct isds_message *m_message; /*!< ISDS message pointer copy. */
	/*
	 * (char *) m_message->raw
	 *     m_message->raw_length
	 * (char *) m_message->envelope->timestamp
	 *     m_message->envelope->timestamp_length
	 */

	int m_zfoType; /*!< Type of message (enum ImportZFODialog::ZFOtype). */
	DbFlsTblModel m_attachmentModel; /*!< Attachment model. */
};

#endif /* _DLG_VIEW_ZFO_H_ */
