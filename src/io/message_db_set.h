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

#ifndef _MESSAGE_DB_SET_H_
#define _MESSAGE_DB_SET_H_

#include <QDateTime>
#include <QMap>
#include <QString>
#include <QStringList>

#include "src/io/message_db.h"

#define DB_SUFFIX ".db"
#define PRIMARY_KEY_RE "[^_]+"
#define SINGLE_FILE_SEC_KEY ""
#define YEARLY_SEC_KEY_RE "[0-9][0-9][0-9][0-9]"
#define YEARLY_SEC_KEY_INVALID INVALID_YEAR

/*
 * Flags used when creating new database file.
 */
#define MDS_FLG_TESTING         0x01 /*!< Create a testing database. */
#define MDS_FLG_CREATE_FILE     0x02 /*!< Create file if does not exist. */
#define MDS_FLG_CHECK_QUICK     0x04 /*!< Perform a quick database check. */
#define MDS_FLG_CHECK_INTEGRITY 0x08 /*!< Perform a full integrity check. */

/*
 * Error codes returned when accessing/creating new database file.
 */
#define MDS_ERR_OK       0 /*!< No error. */
#define MDS_ERR_MISSFILE 1 /*!< Database file does not exist. */
#define MDS_ERR_NOTAFILE 2 /*!< Database file is not a file. */
#define MDS_ERR_ACCESS   3 /*!< Error reading/writing database file. */
#define MDS_ERR_CREATE   4 /*!< Error creating database file. */
#define MDS_ERR_DATA     5 /*!< Data corrupted or not a database file. */
#define MDS_ERR_MULTIPLE 6 /*!< Multiple differently organised databases reside in the same location. */

/*!
 * Organises database files according to secondary keys.
 */
class MessageDbSet : private QMap<QString, MessageDb *> {

public:
	/*!
	 * Style of database organisation.
	 */
	enum Organisation {
		DO_UNKNOWN = 0, /*!< Unsupported type. */
		DO_SINGLE_FILE, /*!< Content is stored within a single file. */
		DO_YEARLY /*!< Content is split according to delivery year. */
	};

	/* Constructor is private. */

	/*!
	 * @brief Destructor.
	 */
	~MessageDbSet(void);

	/*!
	 * @brief Creates a copy of the current database into a given new
	 *     directory.
	 *
	 * @param[in] newLocDir  New location directory.
	 * @return True if database was copied and re-opened.
	 */
	bool copyToLocation(const QString &newLocDir);

	/*!
	 * @brief Move message database into a new directory.
	 *
	 * @param[in] newLocDir New location directory.
	 * @return True if database was moved and re-opened.
	 */
	bool moveToLocation(const QString &newLocDir);

	/*!
	 * @brief Re-open a new empty database file. The old file is left
	 *     untouched.
	 *
	 * @param[in] newLocDir New location directory.
	 * @paran[on] organisation Organisation type.
	 * @return True if database was re-opened.
	 */
	bool reopenLocation(const QString &newLocDir,
	    enum Organisation organisation);

	/*!
	 * @brief Delete associated message db files in location.
	 *
	 * @return True on success.
	 */
	bool deleteLocation(void);

	/*!
	 * @brief Computes the secondary key from supplied date.
	 */
	QString secondaryKey(const QDateTime &time) const;

	/*!
	 * @brief Access already existent message database.
	 *
	 * @param secondaryKey Secondary key.
	 * @return Message database or zero pointer if database does not exist.
	 */
	MessageDb *constAccessMessageDb(const QDateTime &deliveryTime) const;

	/*!
	 * @brief Accesses message database file matching the secondary key.
	 *
	 * @param secondaryKey Secondary key.
	 * @param writeNew     Whether the database should be written if missing.
	 * @return Message database or zero pointer on error.
	 */
	MessageDb *accessMessageDb(const QDateTime &deliveryTime, bool writeNew);

	/*!
	 * @brief Returns list of file paths where the database files reside.
	 *
	 * @note If the database is stored in memory, then a list containing
	 * a single string indicating memory location is returned.
	 * @return List of file location.
	 */
	QStringList fileNames(void) const;

	/*!
	 * @brief Creates a new object.
	 *
	 * @param[in] locDir       Directory that holds the database files.
	 * @param[in] primaryKey   Primary key, usually the user name.
	 * @param[in] testing      True if this is a testing account.
	 * @param[in] organisation How to organise the database files.
	 * @return Pointer to new container or zero pointer on error.
	 */
	static
	MessageDbSet *createNew(const QString &locDir,
	    const QString &primaryKey, bool testing, enum Organisation organisation,
	    bool mustExist);

	/*!
	 * @brief Get the organisation type of the database.
	 *
	 * @param[in] locDir     Directory where the database resides.
	 * @param[in] primaryKey Primary key.
	 * @param[in] testing    True if a testing account.
	 * @return    Organisation type.
	 */
	static
	enum Organisation dbOrganisation(const QString &locDir,
	    const QString &primaryKey, bool testing);

	/*!
	 * @brief Returns the secondary key from a file name.
	 *
	 * @param[in] fileName     File name (without path).
	 * @param[in] organisation Organisation type.
	 * @return Secondary key, null string on error.
	 */
	static
	QString secondaryKeyFromFileName(const QString &fileName,
	    enum Organisation organisation);

	/*!
	 * @brief Returns the database file names stored in the location.
	 *
	 * @note If DO_UNKNOWN is passed via `organisation` then all possible
	 *     matching file names are returned.
	 *
	 * @param[in] locDir       Directory where the database should reside.
	 * @param[in] primaryKey   Primary key.
	 * @param[in] testing      True if a testing account.
	 * @paran[in] organisation Organisation type.
	 * @param[in] fileOnly     True if only files desired (ignores directories etc.).
	 * @return List of database files in the location.
	 */
	static
	QStringList existingDbFileNamesInLocation(const QString &locDir,
	    const QString &primaryKey, bool testing,
	    enum Organisation organisation, bool filesOnly);

	/*!
	 * @brief Construct key from primary and secondary key.
	 *
	 * @param[in] primaryKey   Primary key.
	 * @param[in] secondaryKey Secondary key.
	 * @paran[on] organisation Organisation type.
	 * @return Key or null string on error.
	 */
	static
	QString constructKey(const QString &primaryKey,
	    const QString &secondaryKey, enum Organisation organisation);

	/*!
	 * @brief Check existing databases for basic faults.
	 *
	 * @param[in] primaryKey ISDS user name.
	 * @param[in] locDir     Directory where to store the file.
	 * @param[in] flags      Flags to be passed.
	 * @return Error code.
	 */
	static
	int checkExistingDbFile(const QString &locDir,
	    const QString &primaryKey, int flags);

	/*!
	 * @brief Returns file name according to primary and secondary key.
	 *
	 * @param[in] locDir       Directory where the database should reside.
	 * @param[in] primaryKey   Primary key.
	 * @param[in] secondaryKey Secondary key.
	 * @param[in] testing      True if a testing account.
	 * @paran[on] organisation Organisation type.
	 * @return Full file path or empty string on error.
	 */
	static
	QString constructDbFileName(const QString &locDir,
	    const QString &primaryKey, const QString &secondaryKey,
	    bool testing, enum Organisation organisation);

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

private:
	/*!
	 * @brief Creates a database set.
	 *
	 * @param[in] locDir     Directory that holds the database files.
	 * @param[in] primaryKey Primary key, usually the user name.
	 * @param[in] testing    True if this is a testing account.
	 * @param[in] organisation How to organise the database files.
	 */
	MessageDbSet(const QString &locDir, const QString &primaryKey,
	    bool testing, enum Organisation organisation);

	/*!
	 * @brief Accesses message database file matching the secondary key.
	 *
	 * @param[in] secondaryKey Secondary key.
	 * @param[in] create       Whether to create the file.
	 * @return Message database or zero pointer on error.
	 */
	MessageDb *_accessMessageDb(const QString &secondaryKey, bool create);

	/*!
	 * @brief Test given file.
	 *
	 * @param[in] filePath Full file path.
	 * @param[in] flags    Flags.
	 * @return Error code.
	 */
	static
	int checkGivenDbFile(const QString &filePath, int flags);

	const QString m_primaryKey; /*!< Used for accessing the database. */
	const bool m_testing; /*!< Whether those are a testing databases. */
	QString m_locDir; /*!< Directory where the files reside. */
	enum Organisation m_organisation; /*!< How the database is organised. */

public: /* Database function that have been delegate to the container. */
	/*!
	 * @brief Return all received messages model.
	 *
	 * @return Pointer to model, 0 on failure.
	 *
	 * @note The model must not be freed.
	 */
	DbMsgsTblModel *msgsRcvdModel(void);

	/*!
	 * @brief Return received messages within past 90 days.
	 *
	 * @return Pointer to model, 0 on failure.
	 *
	 * @note The model must not be freed.
	 */
	DbMsgsTblModel *msgsRcvdWithin90DaysModel(void);

	/*!
	 * @brief Return received messages within given year.
	 *
	 * @param[in] year  Year number.
	 * @return Pointer to model, 0 on failure.
	 *
	 * @note The model must not be freed.
	 */
	DbMsgsTblModel *msgsRcvdInYearModel(const QString &year);

	/*!
	 * @brief Return list of years (strings) in database.
	 *
	 * @param[in] type    Whether to obtain sent or received messages.
	 * @param[in] sorting Sorting.
	 * @return List of years.
	 */
	QStringList msgsYears(enum MessageDb::MessageType type,
	    enum Sorting sorting) const;

	/*!
	 * @brief Return list of years and number of messages in database.
	 *
	 * @param[in] type    Whether to obtain sent or received messages.
	 * @param[in] sorting Sorting.
	 * @return List of years and counts.
	 */
	QList< QPair<QString, int> > msgsYearlyCounts(
	    enum MessageDb::MessageType type, enum Sorting sorting) const;

	/*!
	 * @brief Return number of unread messages received within past 90
	 *     days.
	 *
	 * @param[in] type Whether to obtain sent or received messages.
	 * @return Number of unread messages, -1 on error.
	 */
	int msgsUnreadWithin90Days(enum MessageDb::MessageType type) const;

	/*!
	 * @brief Return number of unread received messages in year.
	 *
	 * @param[in] type Whether to obtain sent or received messages.
	 * @param[in] year Year number.
	 * @return Number of unread messages, -1 on error.
	 */
	int msgsUnreadInYear(enum MessageDb::MessageType type,
	    const QString &year) const;

	/*!
	 * @brief Return all sent messages model.
	 *
	 * @return Pointer to model, 0 on failure.
	 *
	 * @note The model must not be freed.
	 */
	DbMsgsTblModel *msgsSntModel(void);

	/*!
	 * @brief Return sent messages within past 90 days.
	 *
	 * @return Pointer to model, 0 on failure.
	 *
	 * @note The model must not be freed.
	 */
	DbMsgsTblModel *msgsSntWithin90DaysModel(void);

	/*!
	 * @brief Return sent messages within given year.
	 *
	 * @param[in] year  Year number.
	 * @return Pointer to model, 0 on failure.
	 *
	 * @note The model must not be freed.
	 */
	DbMsgsTblModel *msgsSntInYearModel(const QString &year);

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
	QList<MessageDb::ContactEntry> uniqueContacts(void) const;

	/*!
	 * @brief Return all message ID from database.
	 *
	 * @return message id list.
	 */
	QList<MessageDb::MsgId> getAllMessageIDsFromDB(void) const;

	/*!
	 * @brief Return list of message ids corresponding to given date
	 *     interval.
	 *
	 * @param[in] fromDate  Start date.
	 * @param[in] toDate    Stop date.
	 * @param[in] sent      True for sent messages, false for received.
	 * @return List of message ids. Empty list on error.
	 */
	QList<MessageDb::MsgId> msgsDateInterval(const QDate &fromDate,
	    const QDate &toDate, enum MessageDirection msgDirect) const;

	/*!
	 * @brief Searches for matching messages according to envelope data.
	 *
	 * @return message item list pass to search query.
	 */
	QList<MessageDb::SoughtMsg> msgsAdvancedSearchMessageEnvelope(
	    qint64 dmId, const QString &dmAnnotation,
	    const QString &dbIDSender, const QString &dmSender,
	    const QString &dmAddress, const QString &dbIDRecipient,
	    const QString &dmRecipient, const QString &dmSenderRefNumber,
	    const QString &dmSenderIdent, const QString &dmRecipientRefNumber,
	    const QString &dmRecipientIdent, const QString &dmToHands,
	    const QString &dmDeliveryTime, const QString &dmAcceptanceTime,
	    enum MessageDirection msgDirect) const;

private:
	/*!
	 * @brief Return list of seconday keys that may be involed in last 90
	 *     days.
	 *
	 * @return List of keys.
	 */
	QStringList _yrly_secKeysIn90Days(void) const;

	inline DbMsgsTblModel *_sf_msgsRcvdModel(void);
	inline DbMsgsTblModel *_yrly_msgsRcvdModel(void);

	inline DbMsgsTblModel *_sf_msgsRcvdWithin90DaysModel(void);
	static
	inline DbMsgsTblModel *_yrly_2dbs_msgsRcvdWithin90DaysModel(MessageDb &db, const QString &attachFileName);
	inline DbMsgsTblModel *_yrly_msgsRcvdWithin90DaysModel(void);

	inline DbMsgsTblModel *_sf_msgsRcvdInYearModel(const QString &year);
	inline DbMsgsTblModel *_yrly_msgsRcvdInYearModel(const QString &year);

	inline QStringList _sf_msgsYears(enum MessageDb::MessageType type, enum Sorting sorting) const;
	inline QStringList _yrly_msgsYears(enum MessageDb::MessageType type, enum Sorting sorting) const;

	inline QList< QPair<QString, int> > _sf_msgsYearlyCounts(enum MessageDb::MessageType type, enum Sorting sorting) const;
	inline QList< QPair<QString, int> > _yrly_msgsYearlyCounts(enum MessageDb::MessageType type, enum Sorting sorting) const;

	inline int _sf_msgsUnreadWithin90Days(enum MessageDb::MessageType type) const;
	inline int _yrly_msgsUnreadWithin90Days(enum MessageDb::MessageType type) const;

	inline int _sf_msgsUnreadInYear(enum MessageDb::MessageType type, const QString &year) const;
	inline int _yrly_msgsUnreadInYear(enum MessageDb::MessageType type, const QString &year) const;

	inline DbMsgsTblModel *_sf_msgsSntModel(void);
	inline DbMsgsTblModel *_yrly_msgsSntModel(void);

	inline DbMsgsTblModel *_sf_msgsSntWithin90DaysModel(void);
	static
	inline DbMsgsTblModel *_yrly_2dbs_msgsSntWithin90DaysModel(MessageDb &db, const QString &attachFileName);
	inline DbMsgsTblModel *_yrly_msgsSntWithin90DaysModel(void);

	inline DbMsgsTblModel *_sf_msgsSntInYearModel(const QString &year);
	inline DbMsgsTblModel *_yrly_msgsSntInYearModel(const QString &year);

	inline bool _sf_smsgdtSetAllReceivedLocallyRead(bool read);
	inline bool _yrly_smsgdtSetAllReceivedLocallyRead(bool read);

	inline bool _sf_smsgdtSetReceivedYearLocallyRead(const QString &year, bool read);
	inline bool _yrly_smsgdtSetReceivedYearLocallyRead(const QString &year, bool read);

	inline bool _sf_smsgdtSetWithin90DaysReceivedLocallyRead(bool read);
	inline bool _yrly_smsgdtSetWithin90DaysReceivedLocallyRead(bool read);

	inline bool _sf_msgSetAllReceivedProcessState(enum MessageProcessState state);
	inline bool _yrly_msgSetAllReceivedProcessState(enum MessageProcessState state);

	inline bool _sf_smsgdtSetReceivedYearProcessState(const QString &year, enum MessageProcessState state);
	inline bool _yrly_smsgdtSetReceivedYearProcessState(const QString &year, enum MessageProcessState state);

	inline bool _sf_smsgdtSetWithin90DaysReceivedProcessState(enum MessageProcessState state);
	inline bool _yrly_smsgdtSetWithin90DaysReceivedProcessState(enum MessageProcessState state);

	inline QList<MessageDb::ContactEntry> _sf_uniqueContacts(void) const;
	inline QList<MessageDb::ContactEntry> _yrly_uniqueContacts(void) const;

	inline QList<MessageDb::MsgId> _sf_getAllMessageIDsFromDB(void) const;
	inline QList<MessageDb::MsgId> _yrly_getAllMessageIDsFromDB(void) const;

	inline QList<MessageDb::MsgId> _sf_msgsDateInterval(const QDate &fromDate, const QDate &toDate, enum MessageDirection msgDirect) const;
	inline QList<MessageDb::MsgId> _yrly_msgsDateInterval(const QDate &fromDate, const QDate &toDate, enum MessageDirection msgDirect) const;

	inline QList<MessageDb::SoughtMsg> _sf_msgsAdvancedSearchMessageEnvelope(qint64 dmId, const QString &dmAnnotation, const QString &dbIDSender,
	    const QString &dmSender, const QString &dmAddress, const QString &dbIDRecipient, const QString &dmRecipient, const QString &dmSenderRefNumber,
	    const QString &dmSenderIdent, const QString &dmRecipientRefNumber, const QString &dmRecipientIdent, const QString &dmToHands,
	    const QString &dmDeliveryTime, const QString &dmAcceptanceTime, enum MessageDirection msgDirect) const;
	inline QList<MessageDb::SoughtMsg> _yrly_msgsAdvancedSearchMessageEnvelope(qint64 dmId, const QString &dmAnnotation, const QString &dbIDSender,
	    const QString &dmSender, const QString &dmAddress, const QString &dbIDRecipient, const QString &dmRecipient, const QString &dmSenderRefNumber,
	    const QString &dmSenderIdent, const QString &dmRecipientRefNumber, const QString &dmRecipientIdent, const QString &dmToHands,
	    const QString &dmDeliveryTime, const QString &dmAcceptanceTime, enum MessageDirection msgDirect) const;
};

#endif /* _MESSAGE_DB_SET_H_ */
