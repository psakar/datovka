

#ifndef _MESSAGE_DB_H_
#define _MESSAGE_DB_H_


#include <QJsonDocument>
#include <QSslCertificate>
#include <QSqlDatabase>
#include <QSqlQueryModel>

#include "dbs.h"


/*!
 * @brief Custom model class.
 *
 * Used for data conversion on display.
 *
 * @note setItemDelegate and a custom ItemDelegate would also be the solution.
 */
class dbTableModel : public QSqlQueryModel {
public:
	/*!
	 * @brief Convert viewed data in date/time columns.
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
	 * @brief Return received messages model.
	 */
	QAbstractTableModel * msgsRcvdModel(const QString &recipDbId);

	/*!
	 * @brief Return received messages within past 90 days.
	 */
	QAbstractTableModel * msgsRcvdWithin90DaysModel(
	    const QString &recipDbId);

	/*!
	 * @brief Return received messages within given year.
	 */
	QAbstractTableModel * msgsRcvdInYearModel(const QString &recipDbId,
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
	 * @brief Return sent messages model.
	 */
	QAbstractTableModel * msgsSntModel(const QString &sendDbId);

	/*!
	 * @brief Return sent messages within past 90 days.
	 */
	QAbstractTableModel * msgsSntWithin90DaysModel(
	    const QString &sendDbId);

	/*!
	 * @brief Return sent messages within given year.
	 */
	QAbstractTableModel * msgsSntInYearModel(const QString &sendDbId,
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
	 * @brief Return contacts from message db.
	 */
	QList< QVector<QString> > uniqueContacts(void);

	/*!
	 * @brief Return message HTML formatted description.
	 */
	QString descriptionHtml(int dmId, bool showId = false,
	    bool warnOld = true) const;

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

	QSqlDatabase m_db; /*!< Message database. */
	dbTableModel m_sqlModel; /*!< Model of displayed data. */

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
	 * @brief Close message database related to item.
	 */
	void closeMessageDb(const QString &key);

	/*!
	 * @brief Delete message db.
	 */
	void deleteMessageDb(const QString &key);
};


#endif /* _MESSAGE_DB_H_ */
