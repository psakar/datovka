

#ifndef _ACCOUNT_DB_H_
#define _ACCOUNT_DB_H_


#include <QMap>
#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QVariant>


/*!
 * @brief Account information.
 */
class AccountEntry : private QMap<QString, QVariant> {

public:
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

	/*!
	 * @brief Return pwd expiration info from db.
	 */
	const QString getPwdExpirFromDb(const QString &key) const;

 	/*!
	 * @brief Set pwd expiration to password_expiration_date table
	 */
	bool setPwdExpirIntoDb(const QString &key, QString &date)
	    const;

private:
	QSqlDatabase m_db; /*!< Account database. */
};


#endif /* _ACCOUNT_DB_H_ */
