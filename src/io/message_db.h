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


#ifndef _MESSAGE_DB_H_
#define _MESSAGE_DB_H_


#include <QAbstractButton>
#include <QAbstractTableModel>
#include <QJsonDocument>
#include <QList>
#include <QMap>
#include <QModelIndex>
#include <QObject>
#include <QPair>
#include <QStringList>
#include <QSqlDatabase>
#include <QSqlQueryModel>
#include <QSslCertificate>
#include <QString>
#include <QVariant>
#include <QVector>

#include "src/common.h"


enum sorting {
	UNSORTED = 0,
	ASCENDING,
	DESCENDING
};


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
	 * @brief Convert viewed header data.
	 */
	virtual QVariant headerData(int section, Qt::Orientation orientation,
	    int role) const;

	/*!
	 * @brief Override message as being read.
	 *
	 * @param[in] dmId      Message id.
	 * @param[in] forceRead Set whether to force read state.
	 */
	virtual bool overrideRead(int dmId, bool forceRead = true);

	/*!
	 * @brief Override message as having its attachments having downloaded.
	 *
	 * @param[in] dmId            Message id.
	 * @param[in] forceDownloaded Set whether to force attachments
	 *                            downloaded state.
	 */
	virtual bool overrideDownloaded(int dmId, bool forceDownloaded = true);

	/*!
	 * @brief Override message processing state.
	 *
	 * @param[in] dmId       Message id.
	 * @param[in] forceState Set forced value.
	 */
	virtual bool overrideProcessing(int dmId,
	    MessageProcessState forceState);
	/*
	 * The view's proxy model cannot be accessed, so the message must be
	 * addressed via its id rather than using the index.
	 */
private:
	QMap<int, bool> m_overriddenRL; /*!<
	                                 * Holds overriding information for
	                                 * read locally.
	                                 */
	QMap<int, bool> m_overriddenAD; /*!<
	                                 * Holds overriding information for
	                                 * downloaded attachments.
	                                 */
	QMap<int, int> m_overriddenPS; /*!<
	                                * Holds overriding information for
	                                * message processing state.
	                                */
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
	 *
	 * @param[in] fileName  File name.
	 * @return True on success.
	 */
	bool openDb(const QString &fileName);

	/*!
	 * @brief Get file name.
	 *
	 * @return Database file name.
	 */
	QString fileName(void) const;

	/*!
	 * @brief Begin a transaction.
	 *
	 * @return True on success.
	 */
	bool beginTransaction(void);

	/*!
	 * @brief End transaction.
	 */
	bool commitTransaction(void);

	/*!
	 * @brief Begin named transaction.
	 *
	 * @param[in] savePointName  Name of the save point.
	 * @return True on success.
	 */
	bool savePoint(const QString &savePointName);

	/*!
	 * @brief End named transaction.
	 *
	 * @param[in] savePointName  Name of the save point.
	 * @return True on success.
	 */
	bool releaseSavePoint(const QString &savePointName);

	/*!
	 * @brief Roll back transaction.
	 *
	 * @param[in] savePointName  Name of the save point.
	 * @return True on success.
	 *
	 * @note If no save-point name is supplied then a complete roll-back is
	 *     performed.
	 */
	bool rollbackTransaction(const QString &savePointName = QString());

	/*!
	 * @brief Return all received messages model.
	 *
	 * @param[in] recipDbId  Recipient data box identifier.
	 * @return Pointer to model, 0 on failure.
	 *
	 * @note The model must not be freed.
	 */
	DbMsgsTblModel * msgsRcvdModel(const QString &recipDbId);

	/*!
	 * @brief Return received messages within past 90 days.
	 *
	 * @param[in] recipDbId  Recipient data box identifier.
	 * @return Pointer to model, 0 on failure.
	 *
	 * @note The model must not be freed.
	 */
	DbMsgsTblModel * msgsRcvdWithin90DaysModel(const QString &recipDbId);

	/*!
	 * @brief Return received messages within given year.
	 *
	 * @param[in] recipDbId  Recipient data box identifier.
	 * @param[in] year       Year number.
	 * @return Pointer to model, 0 on failure.
	 *
	 * @note The model must not be freed.
	 */
	DbMsgsTblModel * msgsRcvdInYearModel(const QString &recipDbId,
	    const QString &year);

	/*!
	 * @brief Return list of years (strings) in database.
	 *
	 * @param[in] recipDbId  Recipient data box identifier.
	 * @param[in] sorting    Sorting.
	 * @return List of years.
	 */
	QStringList msgsRcvdYears(const QString &recipDbId,
	    enum sorting sorting) const;

	/*!
	 * @brief Return list of years and number of messages in database.
	 *
	 * @param[in] recipDbId  Recipient identifier.
	 * @param[in] sorting    Sorting.
	 * @return List of years and counts.
	 */
	QList< QPair<QString, int> > msgsRcvdYearlyCounts(
	    const QString &recipDbId, enum sorting sorting) const;

	/*!
	 * @brief Return number of unread messages received within past 90
	 *     days.
	 *
	 * @param[in] recipDbId  Recipient identifier.
	 * @return Number of unread messages, -1 on error.
	 */
	int msgsRcvdUnreadWithin90Days(const QString &recipDbId) const;

	/*!
	 * @brief Return number of unread received messages in year.
	 *
	 * @param[in] recipDbId  Recipient identifier.
	 * @param[in] year       Year number.
	 * @return Number of unread messages, -1 on error.
	 */
	int msgsRcvdUnreadInYear(const QString &recipDbId,
	    const QString &year) const;

	/*!
	 * @brief Return all sent messages model.
	 *
	 * @param[in] sendDbId  Sender data box identifier.
	 * @return Pointer to model, 0 on failure.
	 *
	 * @note The model must not be freed.
	 */
	DbMsgsTblModel * msgsSntModel(const QString &sendDbId);

	/*!
	 * @brief Return sent messages within past 90 days.
	 *
	 * @param[in] sendDbId  Sender data box identifier.
	 * @return Pointer to model, 0 on failure.
	 *
	 * @note The model must not be freed.
	 */
	DbMsgsTblModel * msgsSntWithin90DaysModel(const QString &sendDbId);

	/*!
	 * @brief Return sent messages within given year.
	 *
	 * @param[in] sendDbId  Sender data box identifier.
	 * @param[in] year      Year number.
	 * @return Pointer to model, 0 on failure.
	 *
	 * @note The model must not be freed.
	 */
	DbMsgsTblModel * msgsSntInYearModel(const QString &sendDbId,
	    const QString &year);

	/*!
	 * @brief Return list of years (strings) in database.
	 *
	 * @param[in] sendDbId  Sender identifier.
	 * @param[in] sorting   Sorting.
	 * @return List of years.
	 */
	QStringList msgsSntYears(const QString &sendDbId,
	    enum sorting sorting) const;

	/*!
	 * @brief Return list of years and number of messages in database.
	 *
	 * @param[in] sendDbId  Sender identifier.
	 * @param[in] sorting   Sorting.
	 * @return List of years and counts.
	 */
	QList< QPair<QString, int> > msgsSntYearlyCounts(
	    const QString &sendDbId, enum sorting sorting) const;

	/*!
	 * @brief Return number of unread messages sent within past 90
	 *     days.
	 *
	 * @param sendDbId  Sender identifier.
	 * @return Number of unread messages, -1 on error.
	 */
	int msgsSntUnreadWithin90Days(const QString &sendDbId) const;

	/*!
	 * @brief Return number of unread sent messages in year.
	 *
	 * @param sendDbId  Sender identifier.
	 * @param year      Year number.
	 * @return Number of unread messages, -1 on error.
	 */
	int msgsSntUnreadInYear(const QString &sendDbId,
	    const QString &year) const;

	/*!
	 * @brief Generate information for reply dialogue.
	 *
	 * @param[in] dmId  Message id.
	 * @return Vector containing title, senderId, sender, senderAddress,
	 *     mesageType, senderRefNumber.
	 *     Returns empty vector in failure.
	 */
	QVector<QString> msgsReplyDataTo(int dmId) const;

	/*!
	 * @brief Returns true if verification attempt was performed.
	 *
	 * @param[in] dmId  Message id.
	 * @return True is message has been verified. False may be returned
	 *     also on error.
	 */
	bool msgsVerificationAttempted(int dmId) const;

	/*!
	 * @brief Returns whether message is verified.
	 *
	 * @param[in] dmId  Message identifier.
	 * @return True if message was verified successfully. False may be
	 *     returned also on error.
	 */
	bool msgsVerified(int dmId) const;

	/*!
	 * @brief Returns whether message was read locally.
	 * 
	 * @param[in] dmId  Message id.
	 * @retunrn False if not read or on failure.
	 */
	bool smsgdtLocallyRead(int dmId) const;

	/*!
	 * @brief Set message read locally status.
	 *
	 * @param[in] dmId  Message id.
	 * @param[in] read  New read status.
	 * @return True on success.
	 */
	bool smsgdtSetLocallyRead(int dmId, bool read = true);

	/*!
	 * @brief Return contacts from message db.
	 *
	 * @return List of vectors containing recipientId, recipientName,
	 *     recipentAddress.
	 */
	QList< QVector<QString> > uniqueContacts(void) const;

	/*!
	 * @brief Return HTML formatted message description.
	 *
	 * @param[in]     dmId             Message identifier.
	 * @param[in,out] verifySignature  Button to activate/deactivate
	 *                                 according to message content.
	 * @param[in]     showId           Whether to also show the message id.
	 * @param[in]     warnOld
	 * @return HTML formatted string containing message information.
	 *     Empty string is returned on error.
	 */
	QString descriptionHtml(int dmId, QAbstractButton *verifySignature,
	    bool showId = true, bool warnOld = true) const;

	/*!
	 * @brief Return message envelope HTML to be used to generate a PDF.
	 *
	 * @param[in] dmId    Message identifier.
	 * @param[in] dbType  Data-box type string.
	 * @return HTML formatted string generated from message envelope.
	 *     Empty string is returned on error.
	 */
	QString envelopeInfoHtmlToPdf(int dmId, const QString &dbType) const;

	/*!
	 * @brief Return message delivery info HTML to be used to generate
	 *     a PDF.
	 *
	 * @paramp[in] dmId  Message identifier.
	 * @return HTML formatted string generated from message delivery
	 *     information. Empty string is returned on error.
	 */
	QString deliveryInfoHtmlToPdf(int dmId) const;

	/*!
	 * @brief Return files related to given message.
	 *
	 * @param[in] msgId  Message identifier.
	 * @return Pointer to model, 0 on failure.
	 *
	 * @note The model must not be freed.
	 */
	QAbstractTableModel * flsModel(int msgId);

	/*!
	 * @brief Check if any message with given id exists in database.
	 *
	 * @param[in] dmId  Message identifier.
	 * @return Message status if message exists, on error or if message
	 *     does not exist in database.
	 */
	int msgsStatusIfExists(int dmId) const;

	/*!
	 * @brief Insert message envelope into messages table.
	 *
	 * @return True on success.
	 */
	bool msgsInsertMessageEnvelope(int dmId,
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
	    const QByteArray &dmQTimestampBase64,
	    const QString &dmDeliveryTime, const QString &dmAcceptanceTime,
	    int dmMessageStatus, int dmAttachmentSize, const QString &_dmType,
	    const QString &messtype);

	/*!
	 * @brief Update message envelope into messages table.
	 *
	 * @return True on success.
	 */
	bool msgsUpdateMessageEnvelope(int dmId,
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
	    const QByteArray &dmQTimestampBase64,
	    const QString &dmDeliveryTime, const QString &dmAcceptanceTime,
	    int dmMessageStatus, int dmAttachmentSize, const QString &_dmType,
	    const QString &messtype);

	/*!
	 * @brief Update message envelope delivery information.
	 *
	 * @param[in] dmId              Message identifier.
	 * @param[in] dmDeliveryTime    Delivery time in database format.
	 * @param[in] dmAcceptanceTime  Acceptance time in database format.
	 * @return True on success.
	 */
	bool msgsUpdateMessageState(int dmId,
	    const QString &dmDeliveryTime, const QString &dmAcceptanceTime,
	    int dmMessageStatus);

	/*!
	 * @brief Insert/update message files into files table.
	 *
	 * @param[in] dmId                    Message identifier.
	 * @param[in] dmFileDescr             File name.
	 * @param[in] dmUpFileGuid
	 * @param[in] dmMimeType
	 * @param[in] dmFormat
	 * @param[in] dmFileMetaType
	 * @param[in] dmEncodedContentBase64  Base64-encoded file content.
	 * @return True on success.
	 */
	bool msgsInsertUpdateMessageFile(int dmId,
	    const QString &dmFileDescr, const QString &dmUpFileGuid,
	    const QString &dmFileGuid, const QString &dmMimeType,
	    const QString &dmFormat, const QString &dmFileMetaType,
	    const QByteArray &dmEncodedContentBase64);

	/*!
	 * @brief Insert/update message hash into hashes table.
	 *
	 * @param[in] dmId         Message identifier.
	 * @param[in] valueBase64  Base64-encoded has value.
	 * @param[in] algorithm    Algorithm identifier.
	 * @return True on success.
	 */
	bool msgsInsertUpdateMessageHash(int dmId,
	    const QByteArray &valueBase64, const QString &algorithm);

	/*!
	 * @brief Insert/update message events into events table.
	 *
	 * @param[in] dmId          Message identifier.
	 * @param[in] dmEventTime   Event time in database format.
	 * @param[in] dmEventType   Event type identifier.
	 * @param[in] dmEventDescr  Event description.
	 * @return True on success.
	 */
	bool msgsInsertUpdateMessageEvent(int dmId, const QString &dmEventTime,
	    const QString &dmEventType, const QString &dmEventDescr);

	/*!
	 * @brief Insert/update raw (DER) message data into raw_message_data
	 *     table.
	 *
	 * @param[in] dmId         Message identifier.
	 * @param[in] raw          Raw (non-base64 encoded) message data.
	 * @param[in] messageType  Message type.
	 * @return True on success.
	 */
	bool msgsInsertUpdateMessageRaw(int dmId, const QByteArray &raw,
	    int messageType);

	/*!
	 * @brief Get base64 encoded raw message data.
	 *
	 * @param[in] dmId  Message identifier.
	 * @return Empty byte array on error.
	 */
	QByteArray msgsMessageBase64(int dmId) const;

	/*!
	 * @brief Get message data in DER (raw) format.
	 *
	 * @param[in] dmId  Message identifier.
	 * @return Empty byte array on error.
	 */
	QByteArray msgsMessageRaw(int dmId) const;

	/*!
	 * @brief Get base64-encoded delivery info from
	 *     raw_delivery_info_data table.
	 *
	 * @param[in] dmId  Message identifier.
	 * @return Empty byte array on error.
	 */
	QByteArray msgsGetDeliveryInfoBase64(int dmId) const;

	/*!
	 * @brief Insert/update raw (DER) delivery info into
	 *     raw_delivery_info_data.
	 *
	 * @param[in] dmId  Message identifier.
	 * @param[in] raw   Raw (in DER format) delivery information.
	 * @return True on success.
	 */
	bool msgsInsertUpdateDeliveryInfoRaw(int dmId, const QByteArray &raw);

	/*!
	 * @brief Update information about author (sender).
	 *
	 * @param[in] dmId        Message identifier.
	 * @param[in] senderType  Type of sender.
	 * @param[in] senderName  Name of sender.
	 * @return True on success.
	 */
	bool updateMessageAuthorInfo(int dmId, const QString &senderType,
	    const QString &senderName);

	/*!
	 * @brief Return hash of message from db.
	 *
	 * @param[in] dmId  Message identifier.
	 * @return List of string containing base64-encoded hash value and
	 *     algorithm identifier. Empty list is returned on error.
	 */
	QStringList msgsGetHashFromDb(int dmId) const;

	/*!
	 * @brief Delete all message records from db.
	 *
	 * @param[in] dmId  Message identifier.
	 * @return True on success.
	 */
	bool msgsDeleteMessageData(int dmId) const;

	/*!
	 * @brief Return list of message ids corresponding to given date
	 *     interval.
	 *
	 * @param[in] fromDate  Start date.
	 * @param[in] toDate    Stop date.
	 * @param[in] sent      True for sent messages, false for received.
	 * @return List of message ids. Empty list on error.
	 */
	QList<int> msgsDateInterval(const QDate &fromDate, const QDate &toDate,
	    bool sent) const;

	/*!
	 * @brief Return some message items in order to export correspondence
	 *     to HTML.
	 *
	 * @param[in] dmId  Message identifier.
	 * @return String list containing sender, recipient, annotation, ...
	 *    Empty list is returned on error.
	 */
	QStringList getMsgForHtmlExport(int dmId) const;

	/*!
	 * @brief Return some message items for export correspondence to csv.
	 *
	 * @param[in] dmId  Message identifier.
	 * @return String containing message status, message type, ...
	 *    Empty list is returned on error.
	 */
	QStringList getMsgForCsvExport(int dmId) const;

	/*!
	 * @brief Set the verification result.
	 *
	 * @param[in] dmId Message identifier.
	 * @param[in] verified True is message was successfully verified,
	 *                     False if verification failed.
	 * @return True if update was successful.
	 */
	bool msgsSetVerified(int dmId, bool verified);

	/*!
	 * @brief Set process state of received message.
	 *
	 * @param[in] dmId    Message identifier.
	 * @param[in] state   Message state to be set.
	 * @param[in] insert  Whether to insert or update an information.
	 * @return True if update/insert was successful.
	 */
	bool msgSetProcessState(int dmId, int state, bool insert);

	/*!
	 * @brief Get process state of received message.
	 *
	 * @param[in] dmId  Message identifier.
	 * @return Message processing state, -1 on error.
	 */
	int msgGetProcessState(int dmId) const;

	/*!
	 * @brief Returns time stamp in raw (DER) format.
	 *
	 * @param[in] dmId  Message identifier.
	 * @return Qualified time stamp in DER format.
	 *     Empty byte array on error.
	 */
	QByteArray msgsTimestampRaw(int dmId) const;

	static
	const QVector<QString> receivedItemIds;
	static
	const QVector<QString> sentItemIds;


protected:
	/*!
	 * @brief Adds _dmType column.
	 *
	 * @return True on success.
	 *
	 * @note This code may be needed to update database between different
	 * versions.
	 */
	bool addDmtypeColumn(void);

	/*!
	 * @brief Close database file.
	 */
	void closeDb(void);

	/*!
	 * @brief Copy db.
	 *
	 * @param[in] newFileName  New location name.
	 * @return True on success.
	 *
	 * @note The copy is continued to be used. Original is closed.
	 */
	bool copyDb(const QString &newFileName);

	/*!
	 * @brief Move db.
	 */
	bool moveDb(const QString &newFileName);

	/*!
	 * @brief Re-open a different database file.
	 *
	 * @note The old database file is left untouched.
	 */
	bool reopenDb(const QString &newFileName);

	/*!
	 * @brief Add/update message certificate in database.
	 */
	bool msgsInsertUpdateMessageCertBase64(int dmId,
	    const QByteArray &crtBase64);

private:
	static
	const QVector<QString> msgPrintedAttribs;
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
	 * @brief Returns verification date (in local time).
	 */
	QDateTime msgsVerificationDate(int dmId) const;

	/*!
	 * @brief Read data from supplementary message data table.
	 */
	QJsonDocument smsgdCustomData(int msgId) const;

	/*!
	 * @brief Certificates related to given message.
	 */
	QList<QSslCertificate> msgCerts(int dmId) const;

	/*!
	 * @brief Check whether message signature was valid at given date
	 *     (local time).
	 *
	 * @param[in] dmId                  Message id.
	 * @param[in] dateTime              Local time identifier.
	 * @param[in] ignoreMissingCrlCheck Ignore CRL check if set to true.
	 * @return True if date check succeeds.
	 */
	bool msgCertValidAtDate(int dmId, const QDateTime &dateTime,
	    bool ignoreMissingCrlCheck = false) const;

	friend class dbContainer;
};


/*!
 * @brief Database container.
 *
 * TODO -- Should there be a single globally accessible instance?
 *     (Actually no singleton.)
 */
class dbContainer : public QMap<QString, MessageDb *> {

public:
	dbContainer(void);
	~dbContainer(void);

	/*!
	 * @brief Access/create+open message database related to item.
	 */
	MessageDb * accessMessageDb(const QString &key, const QString &locDir,
	    bool testing);

	/*!
	 * @brief Creates a copy of the current database into a given new
	 *     directory.
	 *
	 * @param[in] newLocDir New location directory.
	 * @return True if database was copied and re-opened.
	 */
	bool copyMessageDb(MessageDb *db, const QString &newLocDir);

	/*!
	 * @brief Move message database into a new directory.
	 *
	 * @param[in] newLocDir New location directory.
	 * @return True if database was moved and re-opened.
	 */
	bool moveMessageDb(MessageDb *db, const QString &newLocDir);

	/*!
	 * @brief Re-open a new database file. The old file is left untouched.
	 *
	 * @param[in] newLocDir New location directory.
	 * @return True if database was re-opened.
	 */
	bool reopenMessageDb(MessageDb *db, const QString &newLocDir);

	/*!
	 * @brief Delete message db file.
	 *
	 * @param db Deleted database.
	 * @return True on success.
	 */
	bool deleteMessageDb(MessageDb *db);

	/*!
	 * @brief Database driver name.
	 */
	static
	const QString dbDriverType;

	/*!
	 * @brief Check whether required SQL driver is present.
	 *
	 * @return True if database driver is present.
	 */
	static
	bool dbDriverSupport(void);

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
private:
	/*!
	 * @brief Creates the database name from supplied information.
	 *
	 * @param[in] key     ISDS user name.
	 * @param[in] locDir  Directory where to store the file.
	 * @param[in] testing Whether it is a testing account.
	 * @return Path to database file.
	 */
	QString constructDbFileName(const QString &key, const QString &locDir,
	    bool testing);
};


#endif /* _MESSAGE_DB_H_ */
