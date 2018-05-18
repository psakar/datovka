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

#ifndef _DLG_SIGNATURE_DETAIL_H_
#define _DLG_SIGNATURE_DETAIL_H_

#include <QByteArray>
#include <QDialog>

#include "src/io/message_db.h"
#include "src/io/message_db_set.h"

namespace Ui {
	class DlgSignatureDetail;
}

/*!
 * @brief Shows information about message signature.
 */
class DlgSignatureDetail : public QDialog {
	Q_OBJECT

private:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] msgDER Raw message data.
	 * @param[in] tstDER Raw time-stamp data.
	 * @param[in] parent Parent widget.
	 */
	DlgSignatureDetail(const QByteArray &msgDER, const QByteArray &tstDER,
	    bool constructedFromDb, bool dbIsVerified,
	    QWidget *parent = Q_NULLPTR);

public:
	/*!
	 * @brief Destructor.
	 */
	~DlgSignatureDetail(void);

	/*!
	 * @brief Views a signature detail dialogue.
	 *
	 * @param[in] dbSet Database set.
	 * @param[in] msgId Message identifier.
	 * @param[in] parent Parent widget.
	 */
	static
	void detail(const MessageDbSet &dbSet, const MessageDb::MsgId &msgId,
	    QWidget *parent = Q_NULLPTR);

	/*!
	 * @brief Views a signature detail dialogue.
	 *
	 * @param[in] msgDER Raw message data.
	 * @param[in] tstDER Raw time-stamp data.
	 * @param[in] parent Parent widget.
	 */
	static
	void detail(const QByteArray &msgDER, const QByteArray &tstDER,
	    QWidget *parent = Q_NULLPTR);

	/*!
	 * @brief Check whether certificate expires before specified limit.
	 *
	 * @param[in] DER   Raw message or time stamp data.
	 * @param[in] days  Number of days preceding the deadline.
	 * @param[in] dDate Deadline date.
	 * @return True if certificate expires before specified number of days
	 *     before specified deadline.
	 */
	static
	bool signingCertExpiresBefore(const QByteArray &DER,
	    int days, QDateTime dDate = QDateTime());

private slots:
	/*!
	 * @brief Show/hide certificate details
	 *
	 * @param[in] checkState State of the checkbox controlling
	 *                       the visibility.
	 */
	void showCertificateDetail(int checkState);

	/*!
	 * @brief Show/hide verification details.
	 *
	 * @param[in] checkState State of the checkbox controlling
	 *                       the visibility.
	 */
	void showVerificationDetail(int checkState);

private:
	/*!
	 * @brief Check message signature, show result in the dialogue.
	 */
	void validateMessageSignature(void);

	/*!
	 * @brief Validate signing certificate, show result in the dialogue.
	 */
	void validateSigningCertificate(void);

	/*!
	 * @brief Check time stamp signature, show detail in the dialogue.
	 */
	void validateMessageTimestamp(void);

	Ui::DlgSignatureDetail *m_ui; /*!< UI generated from UI file. */

	const QByteArray &m_msgDER; /*!< Message CMS. */
	const QByteArray &m_tstDER; /*!< Time stamp CMS. */
	const bool m_constructedFromDb; /*!< True if constructed from database. */
	bool m_dbIsVerified; /*!< Set if constructed from database. */

	QSize m_dlgSize; /*!<
	                  * Remembered dialogue size. Should help with window
	                  * resizing.
	                  */
};

#endif /* _DLG_SIGNATURE_DETAIL_H_ */
