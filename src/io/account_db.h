

#ifndef _ACCOUNT_DB_H_
#define _ACCOUNT_DB_H_

#include <QPair>
#include <QObject>
#include <QSqlDatabase>


/*!
 * @brief Account information.
 */
class AccountEntry : private QMap<QString, QVariant> {

public:
	typedef enum {
		STRING = 1,
		INTEGER,
		BOOL
	} m_dbEntryType;

	AccountEntry(void);
	~AccountEntry(void);

	/*!
	 * @brief Set value.
	 */
	bool setValue(const QString &key, const QVariant &value);

	/*!
	 * @brief Check whether value is stored.
	 */
	bool hasValue(const QString &key) const;

	/*!
	 * @brief Return stored value.
	 */
	const QVariant value(const QString &key,
	    const QVariant &defaultValue = QVariant()) const;

	/*!
	 * List of know entries and their types.
	 */
	static
	const QVector< QPair<QString, m_dbEntryType> > entryNames;

	/*!
	 * Mapping between entry identifiers an their description.
	 */
	static
	const QMap<QString, QString> entryNameMap;

private:
	typedef QMap<QString, QVariant> m_parentType;
};


/*!
 * @brief Encapsulates account database.
 */
class AccountDb : public QObject {

public:
	AccountDb(const QString &connectionName, QObject *parent = 0);
	~AccountDb(void);

	/*!
	 * @brief Open database file.
	 */
	bool openDb(const QString &fileName);

	/*!
	 * @brief Return account entry.
	 *
	 * @note Key is in format 'login___True'
	 */
	AccountEntry accountEntry(const QString &key) const;

	/*!
	 * @brief Return data box identifier.
	 */
	const QString dbId(const QString &key,
	    const QString defaultValue = QString()) const;

private:
	QSqlDatabase m_db; /*!< Account database. */
};


#endif /* _ACCOUNT_DB_H_ */
