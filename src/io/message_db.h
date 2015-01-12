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
	 * @brief Return list of years (strings) in database.
	 *
	 * @param recipDbId Recipient identifier.
	 * @param sorting   Sorting.
	 * @return List of years.
	 */
	QList<QString> msgsRcvdYears(const QString &recipDbId,
	    enum sorting sorting) const;

	/*!
	 * @brief Return list of years and number of messages in database.
	 *
	 * @param recipDbId Recipient identifier.
	 * @param sorting   Sorting.
	 * @return List of years and counts.
	 */
	QList< QPair<QString, int> > msgsRcvdYearlyCounts(
	    const QString &recipDbId, enum sorting sorting) const;

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
	 *
	 * @param sendDbId Sender identifier.
	 * @param sorting  Sorting.
	 * @return List of years.
	 */
	QList<QString> msgsSntYears(const QString &sendDbId,
	    enum sorting sorting) const;

	/*!
	 * @brief Return list of years and number of messages in database.
	 *
	 * @param sendDbId Sender identifier.
	 * @param sorting  Sorting.
	 * @return List of years and counts.
	 */
	QList< QPair<QString, int> > msgsSntYearlyCounts(
	    const QString &sendDbId, enum sorting sorting) const;

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
	 * @brief Returns whether message is verified.
	 *
	 * @param[in] dmId Message identifier.
	 * @return True if message was verified successfully.
	 */
	bool msgsVerified(int dmId) const;

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
	QList< QVector<QString> > uniqueContacts(void) const;

	/*!
	 * @brief Return message HTML formatted description.
	 */
	QString descriptionHtml(int dmId, QAbstractButton *verifySignature,
	    bool showId = false, bool warnOld = true) const;

	/*!
	 * @brief Return message envelope HTML to PDF.
	 */
	QString envelopeInfoHtmlToPdf(int dmId, QString dbType) const;

	/*!
	 * @brief Return message delivery info HTML to PDF.
	 */
	QString deliveryInfoHtmlToPdf(int dmId) const;

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
	    const QString &dmQTimestamp, const QString &dmDeliveryTime,
	    const QString &dmAcceptanceTime, int dmMessageStatus,
	    int dmAttachmentSize, const QString &_dmType,
	    const QString messtype);

	/*!
	 * @brief Update message envelope into messages table.
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
	    const QString &dmQTimestamp, const QString &dmDeliveryTime,
	    const QString &dmAcceptanceTime, int dmMessageStatus,
	    int dmAttachmentSize, const QString &_dmType,
	    const QString messtype);

	/*!
	 * @brief Update exist message envelope delivery info in db.
	 */
	bool msgsUpdateMessageDeliveryInfo(int dmId,
	    const QString &dmDeliveryTime, const QString &dmAcceptanceTime,
	    int dmMessageStatus);

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
	bool msgsInsertUpdateMessageRaw(int dmId, const QByteArray &raw,
	    int message_type);

	/*!
	 * @brief get raw message data from raw_message_data table.
	 */
	QString msgsMessageBase64(int dmId) const;

	/*!
	 * @brief Get message data in DER format.
	 */
	QByteArray msgsMessageDER(int dmId) const;

	/*!
	 * @brief Get delivery info raw from raw_delivery_info_data table.
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
	 * @brief Return hash of message from db.
	 */
	QList<QString> msgsGetHashFromDb(int dmId) const;

	/*!
	 * @brief Delete all message records from db.
	 */
	bool msgsDeleteMessageData(int dmId) const;

	/*!
	 * @brief Return id of messages in database correspond with
	 * date interval.
	 */
	QList<int> msgsDateInterval(QDate fromDate,
	    QDate toDate, bool sent) const;

	/*!
	 * @brief Return some message items for export correspondence to html.
	 */
	QList<QString> getMsgForHtmlExport(int dmId) const;

	/*!
	 * @brief Return some message items for export correspondence to csv.
	 */
	QList<QString> getMsgForCsvExport(int dmId) const;

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
	 * @return True if update/insert was successful.
	 */
	bool msgSetProcessState(int dmId, int state, bool insert);

	/*!
	 * @brief Get process state of received message.
	 */
	int msgGetProcessState(int dmId);

	/*!
	 * @brief Returns time stamp in DER format.
	 *
	 * @param[in] dmId Message identifier.
	 * @return Qualified time stamp in DER format.
	 */
	QByteArray msgsTimestampDER(int dmId) const;

	static
	const QVector<QString> receivedItemIds;
	static
	const QVector<QString> sentItemIds;


protected:
	/*!
	 * @brief Adds _dmType column.
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
	    const QString &crtBase64);

private:
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
