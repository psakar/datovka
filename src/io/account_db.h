

#ifndef _ACCOUNT_DB_H_
#define _ACCOUNT_DB_H_


#include <QObject>
#include <QSqlDatabase>


class AccountEntry {
public:
	QString dbID;
	QString dbType;
	int ic;
	QString pnFirstName;
	QString pnMiddleName;
	QString pnLastName;
	QString pnLastNameAtBirth;
	QString firmName;
	QString biDate;
	QString biCity;
	QString biCounty;
	QString biState;
	QString adCity;
	QString adStreet;
	QString adNumberInStreet;
	QString adNumberInMunicipality;
	QString adZipCode;
	QString adState;
	QString nationality;
	QString identifier;
	QString registryCode;
	int dbState;
	bool dbEffectiveOVM;
	bool dbOpenAddressing;
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
	AccountEntry accountEntry(const QString &key);

private:
	QSqlDatabase m_db; /*!< Account database. */
};


#endif /* _ACCOUNT_DB_H_ */
