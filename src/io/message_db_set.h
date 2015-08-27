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
	 * @brief Accesses message database file matching the secondary key.
	 *
	 * @param secondaryKey Secondary key.
	 * @param write        Whether the database should be written.
	 * @return Message database or zero pointer on error.
	 */
	MessageDb *accessMessageDb(const QDateTime &deliveryTime, bool write);

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
	 * @param[in] sorting  Sorting.
	 * @return List of years.
	 */
	QStringList msgsRcvdYears(enum Sorting sorting) const;

	/*!
	 * @brief Return list of years and number of messages in database.
	 *
	 * @param[in] sorting  Sorting.
	 * @return List of years and counts.
	 */
	QList< QPair<QString, int> > msgsRcvdYearlyCounts(
	    enum Sorting sorting) const;

	/*!
	 * @brief Return number of unread messages received within past 90
	 *     days.
	 *
	 * @return Number of unread messages, -1 on error.
	 */
	int msgsRcvdUnreadWithin90Days(void) const;

	/*!
	 * @brief Return number of unread received messages in year.
	 *
	 * @param[in] year  Year number.
	 * @return Number of unread messages, -1 on error.
	 */
	int msgsRcvdUnreadInYear(const QString &year) const;

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
	 * @brief Return list of years (strings) in database.
	 *
	 * @param[in] sorting  Sorting.
	 * @return List of years.
	 */
	QStringList msgsSntYears(enum Sorting sorting) const;

	/*!
	 * @brief Return list of years and number of messages in database.
	 *
	 * @param[in] sorting  Sorting.
	 * @return List of years and counts.
	 */
	QList< QPair<QString, int> > msgsSntYearlyCounts(
	    enum Sorting sorting) const;

	/*!
	 * @brief Return number of unread messages sent within past 90
	 *     days.
	 *
	 * @return Number of unread messages, -1 on error.
	 */
	int msgsSntUnreadWithin90Days(void) const;

	/*!
	 * @brief Return number of unread sent messages in year.
	 *
	 * @param year  Year number.
	 * @return Number of unread messages, -1 on error.
	 */
	int msgsSntUnreadInYear(const QString &year) const;

private:
	inline DbMsgsTblModel *_sf_msgsRcvdModel(void);
	inline DbMsgsTblModel *_yrly_msgsRcvdModel(void);

	inline DbMsgsTblModel *_sf_msgsRcvdWithin90DaysModel(void);
	inline DbMsgsTblModel *_yrly_msgsRcvdWithin90DaysModel(void);

	inline DbMsgsTblModel *_sf_msgsRcvdInYearModel(const QString &year);
	inline DbMsgsTblModel *_yrly_msgsRcvdInYearModel(const QString &year);

	inline QStringList _sf_msgsRcvdYears(enum Sorting sorting) const;
	inline QStringList _yrly_msgsRcvdYears(enum Sorting sorting) const;

	inline QList< QPair<QString, int> > _sf_msgsRcvdYearlyCounts(enum Sorting sorting) const;
	inline QList< QPair<QString, int> > _yrly_msgsRcvdYearlyCounts(enum Sorting sorting) const;

	inline int _sf_msgsRcvdUnreadWithin90Days(void) const;
	inline int _yrly_msgsRcvdUnreadWithin90Days(void) const;

	inline int _sf_msgsRcvdUnreadInYear(const QString &year) const;
	inline int _yrly_msgsRcvdUnreadInYear(const QString &year) const;

	inline DbMsgsTblModel *_sf_msgsSntModel(void);
	inline DbMsgsTblModel *_yrly_msgsSntModel(void);

	inline DbMsgsTblModel *_sf_msgsSntWithin90DaysModel(void);
	inline DbMsgsTblModel *_yrly_msgsSntWithin90DaysModel(void);

	inline DbMsgsTblModel *_sf_msgsSntInYearModel(const QString &year);
	inline DbMsgsTblModel *_yrly_msgsSntInYearModel(const QString &year);

	inline QStringList _sf_msgsSntYears(enum Sorting sorting) const;
	inline QStringList _yrly_msgsSntYears(enum Sorting sorting) const;

	inline QList< QPair<QString, int> > _sf_msgsSntYearlyCounts(enum Sorting sorting) const;
	inline QList< QPair<QString, int> > _yrly_msgsSntYearlyCounts(enum Sorting sorting) const;

	inline int _sf_msgsSntUnreadWithin90Days(void) const;
	inline int _yrly_msgsSntUnreadWithin90Days(void) const;

	inline int _sf_msgsSntUnreadInYear(const QString &year) const;
	inline int _yrly_msgsSntUnreadInYear(const QString &year) const;
};

#endif /* _MESSAGE_DB_SET_H_ */
