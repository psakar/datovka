

#ifndef _DLG_SIGNATURE_DETAIL_H_
#define _DLG_SIGNATURE_DETAIL_H_


#include <QByteArray>
#include <QDialog>

#include "src/common.h"
#include "src/io/message_db.h"
#include "ui_dlg_signature_detail.h"


class DlgSignatureDetail : public QDialog, public Ui::SignatureDetail {
    Q_OBJECT

public:
	DlgSignatureDetail(const MessageDb &messageDb, int dmId,
	    QWidget *parent = 0);

private:
	const QByteArray m_msgDER; /*!< Message CMS. */
	const QByteArray m_tstDER; /*!< Timestamp CMS. */
	const bool m_constructedFromDb; /*!< True if constructed from db. */
	const bool m_dbIsVerified; /*!< Set if constucted from db. */

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

	/*!
	 * @brief Return whether signing certificate is valid.
	 *
	 * @param[in] dmId Message identifier.
	 * @return True if signing certificate was verified successfully.
	 */
	bool msgSigningCertValid(void) const;

	/*!
	 * @brief Returns signing certificate of message.
	 *
	 * @param[out] saId   Signature algorithm identifier.
	 * @param[out] saName Signature algorithm name.
	 * @return Null certificate on failure.
	 */
	QSslCertificate msgSigningCert(QString &saId, QString &saName) const;

	/*!
	 * @brief Returns signing certificate inception and expiration date.
	 *
	 * @param[out] incTime Inception time.
	 * @param[out] expTime Expiration time.
	 * @return True on success.
	 */
	bool msgSigningCertTimes(QDateTime &incTime, QDateTime &expTime) const;

	/*!
	 * @brief Time stamp certificate information.
	 *
	 * @param[out] oStr  Organisation name.
	 * @param[out] ouStr Organisation unit name.
	 * @param[out] nStr  Common name.
	 * @param[out] cStr  Country name.
	 * @return False on failure.
	 */
	bool tstInfo(QString &oStr, QString &ouStr, QString &nStr,
	    QString &cStr) const;
};


#endif /* _DLG_SIGNATURE_DETAIL_H_ */
