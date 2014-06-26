

#ifndef _MESSAGE_DB_H_
#define _MESSAGE_DB_H_


#include <QAbstractTableModel>
#include <QJsonDocument>
#include <QList>
#include <QMap>
#include <QModelIndex>
#include <QObject>
#include <QPair>
#include <QSqlDatabase>
#include <QSqlQueryModel>
#include <QSslCertificate>
#include <QString>
#include <QVariant>
#include <QVector>


/*!
 * @brief Custom message model class.
 *
 * Used for data conversion on display. (Use QIdentityProxyModel?)
 *
 * @note setItemDelegate and a custom ItemDelegate would also be the solution.
 */
class DbMsgsTblModel : public QSqlQueryModel {
public:
	/*!
	 * @brief Constructor.
	 */
	DbMsgsTblModel(QObject *parent = 0);

	/*!
	 * @brief Convert viewed data in date/time columns.
	 */
	virtual QVariant data(const QModelIndex &index, int role) const;

	/*!
	 * @brief Override message as being read.
	 *
	 * @param[in] dmId      Message id.
	 * @param[in] forceRead Set whether to force read state.
	 */
	virtual bool overideRead(int dmId, bool forceRead = true);
	/*
	 * The view's proxy model cannot be accessed, so the message must be
	 * addressed via its id rather than using the index.
	 */
private:
	QMap<int, bool> m_overridden; /*!< Holds overriding information. */
};


/*!
 * @brief Custom file model class.
 *
 * Used for data conversion on display. (Use QIdentityProxyModel?)
 */
class DbFlsTblModel : public QSqlQueryModel {
public:
	/*!
	 * @brief Compute viewed data in file size column.
	 */
	virtual QVariant data(const QModelIndex &index, int role) const;
};


/*!
 * @brief Encapsulates message database.
 */
class MessageDb : public QObject {

public:
	MessageDb(const QString &connectionName, QObject *parent = 0);
	virtual ~MessageDb(void);

	/*!
	 * @brief Open database file.
	 */
	bool openDb(const QString &fileName);

	/*!
	 * @brief Get file name.
	 */
	QString fileName(void) const;

	/*!
	 * @brief Return received messages model.
	 */
	DbMsgsTblModel * msgsRcvdModel(const QString &recipDbId);

	/*!
	 * @brief Return received messages within past 90 days.
	 */
	DbMsgsTblModel * msgsRcvdWithin90DaysModel(
	    const QString &recipDbId);

	/*!
	 * @brief Return received messages within given year.
	 */
	DbMsgsTblModel * msgsRcvdInYearModel(const QString &recipDbId,
	    const QString &year);

	/*!
	 * @brief Return list of yearly counts in database.
	 */
	QList<QString> msgsRcvdYears(const QString &recipDbId) const;

	/*!
	 * @brief Return list of years and number of messages in database.
	 */
	QList< QPair<QString, int> > msgsRcvdYearlyCounts(
	    const QString &recipDbId) const;

	/*!
	 * @brief Return number of unread messages received within past 90
	 *     days.
	 */
	int msgsRcvdUnreadWithin90Days(const QString &recipDbId) const;

	/*!
	 * @brief Return number of unread received messages in year.
	 */
	int msgsRcvdUnreadInYear(const QString &recipDbId,
	    const QString &year) const;

	/*!
	 * @brief Return sent messages model.
	 */
	DbMsgsTblModel * msgsSntModel(const QString &sendDbId);

	/*!
	 * @brief Return sent messages within past 90 days.
	 */
	DbMsgsTblModel * msgsSntWithin90DaysModel(
	    const QString &sendDbId);

	/*!
	 * @brief Return sent messages within given year.
	 */
	DbMsgsTblModel * msgsSntInYearModel(const QString &sendDbId,
	    const QString &year);

	/*!
	 * @brief Return list of years (strings) in database.
	 */
	QList<QString> msgsSntYears(const QString &sendDbId) const;

	/*!
	 * @brief Return list of years and number of messages in database.
	 */
	QList< QPair<QString, int> > msgsSntYearlyCounts(
	    const QString &sendDbId) const;

	/*!
	 * @brief Return number of unread messages sent within past 90
	 *     days.
	 */
	int msgsSntUnreadWithin90Days(const QString &sendDbId) const;

	/*!
	 * @brief Return number of unread sent messages in year.
	 */
	int msgsSntUnreadInYear(const QString &sendDbId,
	    const QString &year) const;

	/*!
	 * @brief Generate information for reply dialog.
	 *
	 * @note title, senderId, sender, senderAddress
	 */
	QVector<QString> msgsReplyDataTo(int dmId) const;

	/*!
	 * @brief Returns true if verification attempt was performed.
	 */
	bool msgsVerificationAttempted(int dmId) const;

	/*!
	 * @brief Was message locally read.
	 * 
	 * @param[in] dmId Message id.
	 * @retunrn False if not read or on failure.
	 */
	bool smsgdtLocallyRead(int dmId) const;

	/*!
	 * @brief Set message status to locally read.
	 *
	 * @param[in] dmId Message id.
	 * @param[in] read New read status.
	 * @return True on success.
	 */
	bool smsgdtSetLocallyRead(int dmId, bool read = true);

	/*!
	 * @brief Return contacts from message db.
	 */
	QList< QVector<QString> > uniqueContacts(void);

	/*!
	 * @brief Return message HTML formatted description.
	 */
	QString descriptionHtml(int dmId, bool showId = false,
	    bool warnOld = true) const;

	/*!
	 * @brief Return files related to given message.
	 */
	QAbstractTableModel * flsModel(int msgId);

	/*!
	 * @brief Check if any message (dmID) exists in the table.
	 */
	bool isInMessageDb(int dmId) const;

	/*!
	 * @brief Insert message envelope into messages table.
	 */
	bool msgsInsertMessageEnvelope(int dmId, bool is_verified,
	    const QString &_origin, const QString &dbIDSender,
	    const QString &dmSender, const QString &dmSenderAddress,
	    int dmSenderType, const QString &dmRecipient,
	    const QString &dmRecipientAddress,
	    const QString &dmAmbiguousRecipient,
	    const QString &dmSenderOrgUnit, const QString &dmSenderOrgUnitNum,
	    const QString &dbIDRecipient, const QString &dmRecipientOrgUnit,
	    const QString &dmRecipientOrgUnitNum, const QString &dmToHands,
	    const QString &dmAnnotation, const QString &dmRecipientRefNumber,
	    const QString &dmSenderRefNumber, const QString &dmRecipientIdent,
	    const QString &dmSenderIdent, const QString &dmLegalTitleLaw,
	    const QString &dmLegalTitleYear, const QString &dmLegalTitleSect,
	    const QString &dmLegalTitlePar, const QString &dmLegalTitlePoint,
	    bool dmPersonalDelivery, bool dmAllowSubstDelivery,
	    const QString &dmQTimestamp, const QString &dmDeliveryTime,
	    const QString &dmAcceptanceTime, int dmMessageStatus,
	    int dmAttachmentSize, const QString &_dmType,
	    const QString messtype);

	/*!
	 * @brief Update message envelope into messages table.
	 */
	bool msgsUpdateMessageEnvelope(int dmId, bool is_verified,
	    const QString &_origin, const QString &dbIDSender,
	    const QString &dmSender, const QString &dmSenderAddress,
	    int dmSenderType, const QString &dmRecipient,
	    const QString &dmRecipientAddress,
	    const QString &dmAmbiguousRecipient,
	    const QString &dmSenderOrgUnit, const QString &dmSenderOrgUnitNum,
	    const QString &dbIDRecipient, const QString &dmRecipientOrgUnit,
	    const QString &dmRecipientOrgUnitNum, const QString &dmToHands,
	    const QString &dmAnnotation, const QString &dmRecipientRefNumber,
	    const QString &dmSenderRefNumber, const QString &dmRecipientIdent,
	    const QString &dmSenderIdent, const QString &dmLegalTitleLaw,
	    const QString &dmLegalTitleYear, const QString &dmLegalTitleSect,
	    const QString &dmLegalTitlePar, const QString &dmLegalTitlePoint,
	    bool dmPersonalDelivery, bool dmAllowSubstDelivery,
	    const QString &dmQTimestamp, const QString &dmDeliveryTime,
	    const QString &dmAcceptanceTime, int dmMessageStatus,
	    int dmAttachmentSize, const QString &_dmType,
	    const QString messtype);

	/*!
	 * @brief Insert/update message files into files table.
	 */
	bool msgsInsertUpdateMessageFile(int dmId,
	    const QString &dmFileDescr, const QString &dmUpFileGuid,
	    const QString &dmFileGuid, const QString &dmMimeType,
	    const QString &dmFormat, const QString &dmFileMetaType,
	    const QString &dmEncodedContent);

	/*!
	 * @brief Insert/update message hash into hashes table.
	 */
	bool msgsInsertUpdateMessageHash(int dmId, const QString &value,
	    const QString &algorithm);

	/*!
	 * @brief Insert/update message event into events table.
	 */
	bool msgsInsertUpdateMessageEvent(int dmId, const QString &dmEventTime,
	    const QString &dmEventType, const QString &dmEventDescr);

	/*!
	 * @brief Insert/update raw message data into raw_message_data table.
	 */
	bool msgsInsertUpdateMessageRaw(int dmId, const QString &raw,
	    int message_type);

	/*!
	 * @brief get raw message data from raw_message_data table.
	 */
	QString msgsGetMessageRaw(int dmId);

	/*!
	 * @brief get deliveri info raw from raw_delivery_info_data table.
	 */
	QString msgsGetDeliveryInfoRaw(int dmId);

	/*!
	 * @brief Insert/update raw delivery info into raw_delivery_info_data.
	 */
	bool msgsInsertUpdateDeliveryRaw(int dmId, const QString &raw);

	/*!
	 * @brief Insert additional info about author (sender) into db.
	 */
	bool addMessageAuthorInfo(int dmID, const QString &sender_type,
	    const QString &sender_name);

	/*!
	 * @brief Retrun hash of message froms db.
	 */
	QList<QString> msgsGetHashFromDb(int dmId) const;

	/*!
	 * @brief Delete all message records from db.
	 */
	bool msgsDeleteMessageData(int dmId) const;

protected:
	/*!
	 * @brief Adds _dmType column.
	 *
	 * @note This code may be needed to update database between different
	 * versions.
	 */
	bool addDmtypeColumn(void);

	/* TODO -- Move db. */
	/* TODO -- Copy db. */
	/* TODO -- Delete db. */

private:
	static
	const QVector<QString> receivedItemIds;
	static
	const QVector<QString> sentItemIds;
	static
	const QVector<QString> msgAttribs2;
	static
	const QVector<QString> msgStatus;
	static
	const QVector<QString> fileItemIds;

	QSqlDatabase m_db; /*!< Message database. */
	DbMsgsTblModel m_sqlMsgsModel; /*!< Model of displayed messages. */
	DbFlsTblModel m_sqlFilesModel; /*!< Model of displayed files. */

	/*!
	 * @brief Create empty tables if tables do not already exist.
	 */
	void createEmptyMissingTables(void);

	/*!
	 * @brief Returns whether message is verified.
	 */
	bool msgsVerified(int dmId) const;

	/*!
	 * @brief Returns verification date.
	 */
	QDateTime msgsVerificationDate(int dmId) const;

	/*!
	 * @brief Returns time-stamp validity.
	 *
	 * @return Return time-stamp validity check, tst is invalid if none
	 *     found.
	 */
	bool msgsCheckTimestamp(int dmId, QDateTime &tst) const;

	/*!
	 * @brief Read data from supplementary message data table.
	 */
	QJsonDocument smsgdCustomData(int msgId) const;

	/*!
	 * @brief Certificates related to given message.
	 */
	QList<QSslCertificate> msgCerts(int dmId) const;

	/*!
	 * @brief Check whether message signature was valid at given date.
	 */
	bool msgCertValidAtDate(int dmId, const QDateTime &dateTime,
	    bool ignoreMissingCrlCheck = false) const;
};


/*!
 * @breif Database container.
 */
class dbContainer : public QMap<QString, MessageDb *> {

public:
	dbContainer(void);
	~dbContainer(void);

	/*!
	 * @brief Access/create+open message database related to item.
	 */
	MessageDb * accessMessageDb(const QString &key,
	    const QString &locDir, bool testing);

	/*!
	 * @brief Delete message db file.
	 *
	 * @return True on success.
	 */
	bool deleteMessageDb(MessageDb * deleted);

#if 0
	/*!
	 * @brief Close message database related to item.
	 */
	void closeMessageDb(const QString &key);

	/*!
	 * @brief Delete message db.
	 */
	void deleteMessageDb(const QString &key);
#endif
};


#endif /* _MESSAGE_DB_H_ */
