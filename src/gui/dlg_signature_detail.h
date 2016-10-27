/*
 * Copyright (C) 2014-2015 CZ.NIC
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
#include <QSslCertificate>

#include "src/io/message_db.h"
#include "src/io/message_db_set.h"
#include "ui_dlg_signature_detail.h"


class DlgSignatureDetail : public QDialog, public Ui::SignatureDetail {
    Q_OBJECT

public:
	DlgSignatureDetail(const MessageDbSet &dbSet,
	    const MessageDb::MsgId &msgId, QWidget *parent = 0);
	DlgSignatureDetail(const void *msgDER, size_t msgSize,
	    const void *tstDER, size_t tstSize, QWidget *parent = 0);

	/*!
	 * @brief Return whether signing certificate is valid.
	 *
	 * @param[in] DER Raw message or time stamp data.
	 * @return True is signing certificate was verified successfully.
	 */
	static
	bool signingCertValid(const QByteArray &DER,
	    struct crt_verif_outcome &cvo);

	/*!
	 * @brief Returns signing certificate of message.
	 *
	 * @param[in]  DER    Raw message or time stamp data.
	 * @param[out] saId   Signature algorithm identifier.
	 * @param[out] saName Signature algorithm name.
	 * @return Null certificate on failure.
	 */
	static
	QSslCertificate signingCert(const QByteArray &DER,
	     QString &saId, QString &saName);

	/*!
	 * @brief Returns signing certificate inception and expiration date.
	 *
	 * @param[in]  DER     Raw message or time stamp data.
	 * @param[out] incTime Inception time.
	 * @param[out] expTime Expiration time.
	 * @return True on success.
	 */
	static
	bool signingCertTimes(const QByteArray &DER,
	    QDateTime &incTime, QDateTime &expTime);

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

	/*!
	 * @brief Signing certificate issuer information.
	 *
	 * @param[in]  DER   Raw message or time stamp data.
	 * @param[out] oStr  Organisation name.
	 * @param[out] ouStr Organisation unit name.
	 * @param[out] nStr  Common name.
	 * @param[out] cStr  Country name.
	 * @return False on failure.
	 */
	bool signingCertIssuerInfo(const QByteArray &DER,
	    QString &oStr, QString &ouStr, QString &nStr, QString &cStr);

private slots:
	void showCertificateDetail(int);
	void showVerificationDetail(int);

private:
	/* TODO -- Construct these members as constants. */
	QByteArray m_msgDER; /*!< Message CMS. */
	QByteArray m_tstDER; /*!< Time stamp CMS. */
	const bool m_constructedFromDb; /*!< True if constructed from db. */
	bool m_dbIsVerified; /*!< Set if constructed from db. */

	/*!
	 * @brief Check message signature, show result in dialog.
	 */
	void validateMessageSignature(void);

	/*!
	 * @brief Validate signing certificate, show result in dialog.
	 */
	void validateSigningCertificate(void);

	/*!
	 * @brief Check time stamp signature, show detail in dialog.
	 */
	void validateMessageTimestamp(void);

	QSize dSize;
};


#endif /* _DLG_SIGNATURE_DETAIL_H_ */
