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
#include <QDateTime>
#include <QJsonDocument>
#include <QList>
#include <QObject>
#include <QPair>
#include <QStringList>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

#include "src/common.h"
#include "src/models/files_model.h"
#include "src/models/messages_model.h"

#define INVALID_YEAR "inv"

enum Sorting {
	UNSORTED = 0,
	ASCENDING,
	DESCENDING
};


/*!
 * @brief Encapsulates message database.
 */
class MessageDb : public QObject {

public:
	/*!
	 * @brief Used to distinguish between sent and received messages in db.
	 *
	 * @note This value cannot be changed without breaking backward
	 *     compatibility.
	 */
	enum MessageType {
		TYPE_RECEIVED = 1, /*!< One is received. */
		TYPE_SENT = 2 /*!< Two is sent. */
	};

	/*!
	 * @brief Message identifier.
	 *
	 * @note Messages are identified according to their id and delivery
	 *     time.
	 */
	class MsgId {
	public:
		qint64 dmId; /*!< Message identifier. */
		QDateTime deliveryTime; /*!< Message delivery time. */

		MsgId(void) : dmId(-1), deliveryTime()
		{ }
		MsgId(const MsgId &id)
		    : dmId(id.dmId), deliveryTime(id.deliveryTime)
		{ }
		MsgId(qint64 id, const QDateTime &dTime)
		    : dmId(id), deliveryTime(dTime)
		{ }
		~MsgId(void)
		{ }

		bool isValid(void) const
		{
			return (dmId >= 0) && (deliveryTime.isValid());
		}
	};

	/*!
	 * @brief Basic message information for search dialogue.
	 */
	class SoughtMsg {
	public:
		MsgId mId; /*!< Message identifier. */
		int type; /*!< Matches enum MessageType. */
		QString dmAnnotation; /*!< Message annotation. */
		QString dmSender; /*!< Message sender. */
		QString dmRecipient; /*!< Recipient. */

		SoughtMsg(void)
		    : mId(), type(0), dmAnnotation(), dmSender(), dmRecipient()
		{ }
		SoughtMsg(const MsgId &id, int t, const QString &annot,
		    const QString &sen, const QString &rec)
		    : mId(id), type(t), dmAnnotation(annot),
		    dmSender(sen), dmRecipient(rec)
		{ }
		SoughtMsg(qint64 id, const QDateTime &dTime, int t,
		    const QString &annot, const QString &sen,
		    const QString &rec)
		    : mId(id, dTime), type(t), dmAnnotation(annot),
		    dmSender(sen), dmRecipient(rec)
		{ }
		~SoughtMsg(void)
		{ }

		bool isValid(void) const
		{
			return mId.isValid() &&
			    ((type == TYPE_RECEIVED) || (type == TYPE_SENT)) &&
			    (!dmAnnotation.isEmpty()) &&
			    (!dmSender.isEmpty()) && (!dmRecipient.isEmpty());
		}
	};

	class ContactEntry {
	public:
		qint64 dmId; /*!< Message id. */
		QString boxId;
		QString name;
		QString address;
	};


	/* holds some additional entries for filename creation */
	class FilenameEntry {
	public:
		QDateTime dmDeliveryTime;
		QDateTime dmAcceptanceTime;
		QString dmAnnotation;
		QString dmSender;
	};

	MessageDb(const QString &dbDriverType, const QString &connectionName,
	    QObject *parent = 0);
	virtual ~MessageDb(void);

	/*!
	 * @brief Open database file.
	 *
	 * @param[in] fileName       File name.
	 * @param[in] createMissing  Whether to create missing tables.
	 * @return True on success.
	 */
	bool openDb(const QString &fileName, bool createMissing = true);

	static
	const QString memoryLocation;

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
	 * @brief Generate information for reply dialogue.
	 *
	 * @param[in] dmId  Message id.
	 * @return Vector containing title, senderId, sender, senderAddress,
	 *     mesageType, senderRefNumber.
	 *     Returns empty vector in failure.
	 */
	QVector<QString> msgsReplyData(qint64 dmId) const;

	/*!
	 * @brief Returns true if verification attempt was performed.
	 *
	 * @param[in] dmId  Message id.
	 * @return True is message has been verified. False may be returned
	 *     also on error.
	 */
	bool msgsVerificationAttempted(qint64 dmId) const;

	/*!
	 * @brief Returns whether message is verified.
	 *
	 * @param[in] dmId  Message identifier.
	 * @return True if message was verified successfully. False may be
	 *     returned also on error.
	 */
	bool msgsVerified(qint64 dmId) const;

	/*!
	 * @brief Returns whether message was read locally.
	 *
	 * @param[in] dmId  Message id.
	 * @retunrn False if not read or on failure.
	 */
	bool smsgdtLocallyRead(qint64 dmId) const;

	/*!
	 * @brief Set message read locally status.
	 *
	 * @param[in] dmId  Message id.
	 * @param[in] read  New read status.
	 * @return True on success.
	 */
	bool smsgdtSetLocallyRead(qint64 dmId, bool read = true);

	/*!
	 * @brief Return HTML formatted message description.
	 *
	 * @param[in]     dmId          Message identifier.
	 * @param[in,out] verSigButton  Button to activate/deactivate
	 *                              according to message content.
	 * @param[in]     showId        Whether to also show the message id.
	 * @param[in]     verSignature  Whether to show verification details.
	 * @param[in]     warnOld
	 * @return HTML formatted string containing message information.
	 *     Empty string is returned on error.
	 */
	QString descriptionHtml(qint64 dmId, QAbstractButton *verSigButton,
	    bool showId = true, bool verSignature = true,
	    bool warnOld = true) const;

	/*!
	 * @brief Return message envelope HTML to be used to generate a PDF.
	 *
	 * @param[in] dmId    Message identifier.
	 * @param[in] dbType  Data-box type string.
	 * @return HTML formatted string generated from message envelope.
	 *     Empty string is returned on error.
	 */
	QString envelopeInfoHtmlToPdf(qint64 dmId,
	    const QString &dbType) const;

	/*!
	 * @brief Return message delivery info HTML to be used to generate
	 *     a PDF.
	 *
	 * @paramp[in] dmId  Message identifier.
	 * @return HTML formatted string generated from message delivery
	 *     information. Empty string is returned on error.
	 */
	QString deliveryInfoHtmlToPdf(qint64 dmId) const;

	/*!
	 * @brief Return fileList related to given message.
	 *
	 * @param[in] msgId  Message identifier.
	 * @return List of files and their attributes.
	 */
	QList<QStringList> getFilesFromMessage(qint64 msgId) const;

	/*!
	 * @brief Return files related to given message.
	 *
	 * @param[in] msgId  Message identifier.
	 * @return Pointer to model, 0 on failure.
	 *
	 * @note The model must not be freed.
	 */
	QAbstractTableModel * flsModel(qint64 msgId);

	/*!
	 * @brief Check if any message with given id exists in database.
	 *
	 * @param[in] dmId  Message identifier.
	 * @return Message status if message exists, on error or if message
	 *     does not exist in database.
	 */
	int msgsStatusIfExists(qint64 dmId) const;

	/*!
	 * @brief Check if delivery info exists in the table.
	 *
	 * @param[in] dmId  Message identifier.
	 * @return True if delivery information exist in database.
	 *     Fail is also returned on error.
	 */
	bool isDeliveryInfoRawDb(qint64 dmId) const;

	/*!
	 * @brief Insert newly sent message into messages table.
	 *
	 * @return True on success.
	 */
	bool msgsInsertNewlySentMessageEnvelope(qint64 dmId,
	    const QString &dbIDSender, const QString &dmSender,
	    const QString &dbIDRecipient, const QString &dmRecipient,
	    const QString &dmRecipientAddress, const QString &dmAnnotation);

	/*!
	 * @brief Insert message envelope into messages table.
	 *
	 * @return True on success.
	 */
	bool msgsInsertMessageEnvelope(qint64 dmId,
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
	    enum MessageDirection msgDirect);

	/*!
	 * @brief Update message envelope into messages table.
	 *
	 * @return True on success.
	 */
	bool msgsUpdateMessageEnvelope(qint64 dmId,
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
	    enum MessageDirection msgDirect);

	/*!
	 * @brief Get message state.
	 *
	 * @param[in] dmId  Message identifier.
	 * @return Message state number or -1 on error.
	 */
	int messageState(qint64 dmId) const;

	/*!
	 * @brief Update message envelope delivery information.
	 *
	 * @param[in] dmId              Message identifier.
	 * @param[in] dmDeliveryTime    Delivery time in database format.
	 * @param[in] dmAcceptanceTime  Acceptance time in database format.
	 * @return True on success.
	 */
	bool msgsUpdateMessageState(qint64 dmId,
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
	bool msgsInsertUpdateMessageFile(qint64 dmId,
	    const QString &dmFileDescr, const QString &dmUpFileGuid,
	    const QString &dmFileGuid, const QString &dmMimeType,
	    const QString &dmFormat, const QString &dmFileMetaType,
	    const QByteArray &dmEncodedContentBase64);

	/*!
	 * @brief Insert/update message hash into hashes table.
	 *
	 * @param[in] dmId         Message identifier.
	 * @param[in] valueBase64  Base64-encoded hash value.
	 * @param[in] algorithm    Algorithm identifier.
	 * @return True on success.
	 */
	bool msgsInsertUpdateMessageHash(qint64 dmId,
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
	bool msgsInsertUpdateMessageEvent(qint64 dmId,
	    const QString &dmEventTime, const QString &dmEventType,
	    const QString &dmEventDescr);

	/*!
	 * @brief Insert/update raw (DER) message data into raw_message_data
	 *     table.
	 *
	 * @param[in] dmId         Message identifier.
	 * @param[in] raw          Raw (non-base64 encoded) message data.
	 * @param[in] messageType  Message type.
	 * @return True on success.
	 */
	bool msgsInsertUpdateMessageRaw(qint64 dmId, const QByteArray &raw,
	    int messageType);

	/*!
	 * @brief Check whether whole message is stored in database.
	 *
	 * @param[in] dmId  Message identifier.
	 * @return True if whole message exists.
	 */
	bool msgsStoredWhole(qint64 dmId) const;

	/*!
	 * @brief Get base64 encoded raw message data.
	 *
	 * @param[in] dmId  Message identifier.
	 * @return Empty byte array on error.
	 */
	QByteArray msgsMessageBase64(qint64 dmId) const;

	/*!
	 * @brief Get message data in DER (raw) format.
	 *
	 * @param[in] dmId  Message identifier.
	 * @return Empty byte array on error.
	 */
	QByteArray msgsMessageRaw(qint64 dmId) const;

	/*!
	 * @brief Get base64-encoded delivery info from
	 *     raw_delivery_info_data table.
	 *
	 * @param[in] dmId  Message identifier.
	 * @return Empty byte array on error.
	 */
	QByteArray msgsGetDeliveryInfoBase64(qint64 dmId) const;

	/*!
	 * @brief Insert/update raw (DER) delivery info into
	 *     raw_delivery_info_data.
	 *
	 * @param[in] dmId  Message identifier.
	 * @param[in] raw   Raw (in DER format) delivery information.
	 * @return True on success.
	 */
	bool msgsInsertUpdateDeliveryInfoRaw(qint64 dmId,
	    const QByteArray &raw);

	/*!
	 * @brief Update information about author (sender).
	 *
	 * @param[in] dmId        Message identifier.
	 * @param[in] senderType  Type of sender.
	 * @param[in] senderName  Name of sender.
	 * @return True on success.
	 */
	bool updateMessageAuthorInfo(qint64 dmId, const QString &senderType,
	    const QString &senderName);

	/*!
	 * @brief Return hash of message from db.
	 *
	 * @param[in] dmId  Message identifier.
	 * @return List of string containing base64-encoded hash value and
	 *     algorithm identifier. Empty list is returned on error.
	 */
	QStringList msgsGetHashFromDb(qint64 dmId) const;

	/*!
	 * @brief Delete all message records from db.
	 *
	 * @param[in] dmId  Message identifier.
	 * @return True on success.
	 */
	bool msgsDeleteMessageData(qint64 dmId) const;

	/*!
	 * @brief Return some message items in order to export correspondence
	 *     to HTML.
	 *
	 * @param[in] dmId  Message identifier.
	 * @return String list containing sender, recipient, annotation, ...
	 *    Empty list is returned on error.
	 */
	QStringList getMsgForHtmlExport(qint64 dmId) const;

	/*!
	 * @brief Return some message items for export correspondence to csv.
	 *
	 * @param[in] dmId  Message identifier.
	 * @return String containing message status, message type, ...
	 *    Empty list is returned on error.
	 */
	QStringList getMsgForCsvExport(qint64 dmId) const;

	/*!
	 * @brief Set the verification result.
	 *
	 * @param[in] dmId Message identifier.
	 * @param[in] verified True is message was successfully verified,
	 *                     False if verification failed.
	 * @return True if update was successful.
	 */
	bool msgsSetVerified(qint64 dmId, bool verified);

	/*!
	 * @brief Set process state of received message.
	 *
	 * @param[in] dmId    Message identifier.
	 * @param[in] state   Message state to be set.
	 * @param[in] insert  Whether to insert or update an information.
	 * @return True if update/insert was successful.
	 */
	bool msgSetProcessState(qint64 dmId, enum MessageProcessState state,
	    bool insert);

	/*!
	 * @brief Get process state of received message.
	 *
	 * @param[in] dmId  Message identifier.
	 * @return Message processing state, -1 on error.
	 */
	int msgGetProcessState(qint64 dmId) const;

	/*!
	 * @brief Returns time stamp in raw (DER) format.
	 *
	 * @param[in] dmId  Message identifier.
	 * @return Qualified time stamp in DER format.
	 *     Empty byte array on error.
	 */
	QByteArray msgsTimestampRaw(qint64 dmId) const;

	/*!
	 * @brief Return some additional filename entries as
	 *        (dmDeliveryTime, dmAcceptanceTime, dmAnnotation, dmSender)
	 *
	 * @param[in] dmId  Message identifier.
	 * @return FilenameEntry struct.
	 */
	FilenameEntry msgsGetAdditionalFilenameEntry(qint64 dmId) const;

	/*!
	 * @brief Test if imported message is relevent to account db.
	 *
	 * @param[in] dmId  Message identifier.
	 * @param[in] databoxId  Databox ID where message should be imported.
	 * @return Message is relevant for import to db or not.
	 */
	bool isRelevantMsgForImport(qint64 msgId, const QString databoxId) const;

protected: /* These function are used from within a database container. */
	/*!
	 * @brief Return all received messages model.
	 *
	 * @return Pointer to model, 0 on failure.
	 *
	 * @note The model must not be freed.
	 */
	DbMsgsTblModel * msgsRcvdModel(void);

	/*!
	 * @brief Return received messages within past 90 days.
	 *
	 * @return Pointer to model, 0 on failure.
	 *
	 * @note The model must not be freed.
	 */
	DbMsgsTblModel * msgsRcvdWithin90DaysModel(void);

	/*!
	 * @brief Return received messages within given year.
	 *
	 * @param[in] year  Year number.
	 * @return Pointer to model, 0 on failure.
	 *
	 * @note The model must not be freed.
	 */
	DbMsgsTblModel * msgsRcvdInYearModel(const QString &year);

	/*!
	 * @brief Return list of years (strings) in database.
	 *
	 * @param[in] type    Whether to obtain sent or received messages.
	 * @param[in] sorting Sorting.
	 * @return List of years.
	 */
	QStringList msgsYears(enum MessageType type,
	    enum Sorting sorting) const;

	/*!
	 * @brief Return list of years and number of messages in database.
	 *
	 * @param[in] type    Whether to obtain sent or received messages.
	 * @param[in] sorting Sorting.
	 * @return List of years and counts.
	 */
	QList< QPair<QString, int> > msgsYearlyCounts(enum MessageType type,
	    enum Sorting sorting) const;

	/*!
	 * @brief Return number of unread messages received within past 90
	 *     days.
	 *
	 * @param[in] type Whether to obtain sent or received messages.
	 * @return Number of unread messages, -1 on error.
	 */
	int msgsUnreadWithin90Days(enum MessageType type) const;

	/*!
	 * @brief Return number of unread received messages in year.
	 *
	 * @param[in] type Whether to obtain sent or received messages.
	 * @param[in] year Year number.
	 * @return Number of unread messages, -1 on error.
	 */
	int msgsUnreadInYear(enum MessageType type,
	    const QString &year) const;

	/*!
	 * @brief Return all sent messages model.
	 *
	 * @return Pointer to model, 0 on failure.
	 *
	 * @note The model must not be freed.
	 */
	DbMsgsTblModel * msgsSntModel(void);

	/*!
	 * @brief Return sent messages within past 90 days.
	 *
	 * @return Pointer to model, 0 on failure.
	 *
	 * @note The model must not be freed.
	 */
	DbMsgsTblModel * msgsSntWithin90DaysModel(void);

	/*!
	 * @brief Return sent messages within given year.
	 *
	 * @param[in] year  Year number.
	 * @return Pointer to model, 0 on failure.
	 *
	 * @note The model must not be freed.
	 */
	DbMsgsTblModel * msgsSntInYearModel(const QString &year);

	/*!
	 * @brief Set message read locally for all received messages.
	 *
	 * @param[in] read  New read status.
	 * @return True on success.
	 */
	bool smsgdtSetAllReceivedLocallyRead(bool read = true);

	/*!
	 * @brief Set message read locally for received messages in given year.
	 *
	 * @param[in] year  Year number.
	 * @param[in] read  New read status.
	 * @return True on success.
	 */
	bool smsgdtSetReceivedYearLocallyRead(const QString &year,
	    bool read = true);

	/*!
	 * @brief Set message read locally for recently received messages.
	 *
	 * @param[in] read  New read status.
	 * @return True on success.
	 */
	bool smsgdtSetWithin90DaysReceivedLocallyRead(bool read = true);

	/*!
	 * @brief Set process state of received messages.
	 *
	 * @param[in] state  Message state to be set.
	 * @return True if operation successful.
	 */
	bool msgSetAllReceivedProcessState(enum MessageProcessState state);

	/*!
	 * @brief Set process state of received messages in given year.
	 *
	 * @param[in] year   Year.
	 * @param[in] state  Message state to be set.
	 * @return True if operation successful.
	 */
	bool smsgdtSetReceivedYearProcessState(const QString &year,
	    enum MessageProcessState state);

	/*!
	 * @brief Set process state of recently received messages.
	 *
	 * @param[in] state  Message state to be set.
	 * @return True if operation successful.
	 */
	bool smsgdtSetWithin90DaysReceivedProcessState(
	    enum MessageProcessState state);

	/*!
	 * @brief Return contacts from message db.
	 *
	 * @return List of vectors containing recipientId, recipientName,
	 *     recipentAddress.
	 */
	QList<ContactEntry> uniqueContacts(void) const;

	/*!
	 * @brief Return all message ID from database.
	 *
	 * @return message id list.
	 */
	QList<MsgId> getAllMessageIDsFromDB(void) const;

	/*!
	 * @brief Return list of message ids corresponding to given date
	 *     interval.
	 *
	 * @param[in] fromDate  Start date.
	 * @param[in] toDate    Stop date.
	 * @param[in] sent      True for sent messages, false for received.
	 * @return List of message ids. Empty list on error.
	 */
	QList<MsgId> msgsDateInterval(const QDate &fromDate,
	    const QDate &toDate, enum MessageDirection msgDirect) const;

	/*!
	 * @brief Advance message envelope search.
	 *
	 * @return message item list pass to search query.
	 */
	QList<SoughtMsg> msgsAdvancedSearchMessageEnvelope(
	    qint64 dmId,
	    const QString &dmAnnotation,
	    const QString &dbIDSender, const QString &dmSender,
	    const QString &dmAddress,
	    const QString &dbIDRecipient, const QString &dmRecipient,
	    const QString &dmSenderRefNumber,
	    const QString &dmSenderIdent,
	    const QString &dmRecipientRefNumber,
	    const QString &dmRecipientIdent,
	    const QString &dmToHands,
	    const QString &dmDeliveryTime, const QString &dmAcceptanceTime,
	    enum MessageDirection msgDirect) const;

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
	 *
	 * @param[in] newFileName  New location name.
	 * @return True on success.
	 */
	bool moveDb(const QString &newFileName);

	/*!
	 * @brief Open a new empty database file.
	 *
	 * @note The old database file is left untouched.
	 */
	bool reopenDb(const QString &newFileName);

	/*!
	 * @brief Perform a db integrity check.
	 *
	 * @return False if check fails.
	 */
	bool checkDb(bool quick);

protected:
	QSqlDatabase m_db; /*!< Message database. */
	DbMsgsTblModel m_sqlMsgsModel; /*!< Model of displayed messages. */

private:
	static
	const QVector<QString> msgPrintedAttribs;
	static
	const QVector<QString> msgStatus;
	static
	const QVector<QString> fileItemIds;

	DbFlsTblModel m_sqlFilesModel; /*!< Model of displayed files. */

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
	 * @brief Create empty tables if tables do not already exist.
	 *
	 * @return True on success.
	 */
	bool createEmptyMissingTables(void);

	/*!
	 * @brief This method ensures that the process_state table
	 *     contains a PRIMARY KEY. This table might be created without any
	 *     primary key reference due to a bug in a previous version.
	 *
	 * @return True on success.
	 *
	 * TODO -- This method may be removed in some future version
	 *     of the programme.
	 */
	bool ensurePrimaryKeyInProcessStateTable(void);

	/*!
	 * @brief Returns verification date (in local time).
	 *
	 * @param[in] dmId  Message identifier.
	 * @return Message verification date. Invalid value is returned on
	 *     error.
	 */
	QDateTime msgsVerificationDate(qint64 dmId) const;

	/*!
	 * @brief Read data from supplementary message data table.
	 *
	 * @brief msgId  Message identifier.
	 * @return Stored json document data. Returns empty document on error.
	 */
	QJsonDocument smsgdCustomData(qint64 msgId) const;

	/*!
	 * @brief Check whether message signature was valid at given date
	 *     (local time).
	 *
	 * @param[in] dmId                  Message id.
	 * @param[in] dateTime              Local time identifier.
	 * @param[in] ignoreMissingCrlCheck Ignore CRL check if set to true.
	 * @return True if date check succeeds.
	 */
	bool msgCertValidAtDate(qint64 dmId, const QDateTime &dateTime,
	    bool ignoreMissingCrlCheck = false) const;

	/*!
	 * @brief Add/update message certificate in database.
	 *
	 * @brief[in] dmId       Message identifier.
	 * @brief[in] crtBase64  Base64-encoded certificate.
	 * @return True on success.
	 */
	bool msgsInsertUpdateMessageCertBase64(qint64 dmId,
	    const QByteArray &crtBase64);

	friend class MessageDbSet;
};


#endif /* _MESSAGE_DB_H_ */
