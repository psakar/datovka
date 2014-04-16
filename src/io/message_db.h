

#ifndef _MESSAGE_DB_H_
#define _MESSAGE_DB_H_


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
	QAbstractTableModel * receivedModel(const QString &recipDbId);

	/*!
	 * @brief Return received messages within past 90 days.
	 */
	QAbstractTableModel * receivedWithin90DaysModel(
	    const QString &recipDbId);

	/*!
	 * @brief Return received messages within given year.
	 */
	QAbstractTableModel * receivedInYearModel(const QString &recipDbId,
	    const QString &year);

	/*!
	 * @brief Return list of yearly counts in database.
	 */
	QList<QString> receivedYears(const QString &recipDbId) const;

	/*!
	 * @brief Return list of years and number of messages in database.
	 */
	QList< QPair<QString, int> > receivedYearlyCounts(
	    const QString &recipDbId) const;

	/*!
	 * @brief Return sent messages model.
	 */
	QAbstractTableModel * sentModel(const QString &sendDbId);

	/*!
	 * @brief Return sent messages within past 90 days.
	 */
	QAbstractTableModel * sentWithin90DaysModel(
	    const QString &sendDbId);

	/*!
	 * @brief Return sent messages within given year.
	 */
	QAbstractTableModel * sentInYearModel(const QString &sendDbId,
	    const QString &year);

	/*!
	 * @brief Return list of years (strings) in database.
	 */
	QList<QString> sentYears(const QString &sendDbId) const;

	/*!
	 * @brief Return list of years and number of messages in database.
	 */
	QList< QPair<QString, int> > sentYearlyCounts(
	    const QString &sendDbId) const;

	/*!
	 * @brief Return message HTML formatted description.
	 */
	QString messageDescriptionHtml(int dmId) const;

	/*!
	 * List of know entries in messages db and their types.
	 */
	static
	const QVector< QPair<QString, dbEntryType> > messagesTblKnownAttrs;

	/*!
	 * Mapping between entry identifiers an their description.
	 */
	static
	const QMap<QString, QString> messagesTblAttrNames;

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
	const QVector<dbEntryType> receivedItemTypes;
	static
	const QVector<QString> sentItemIds;
	static
	const QVector<dbEntryType> sentItemTypes;

	QSqlDatabase m_db; /*!< Message database. */
	dbTableModel m_sqlModel; /*!< Model of displayed data. */
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
