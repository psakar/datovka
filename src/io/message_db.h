

#ifndef _MESSAGE_DB_H_
#define _MESSAGE_DB_H_


#include <QStandardItemModel>
#include <QSqlDatabase>


/*!
 * @brief Provides a model for received messages.
 */
class ReceivedMesagesDbModel : public QStandardItemModel {

public:
	ReceivedMesagesDbModel(QSqlDatabase &db, QObject *parent = 0);

private:
	bool addMessage(int row, const QString &id, const QString &title,
	    const QString &sender, const QString &delivered,
	    const QString &accepted);
	void fillContent(void);

	QSqlDatabase &m_dbRef; /*!< Reference to database. */
};


/*
 * @brief On-line accessible messages.
 */
class SentMesagesDbModel : public QStandardItemModel {

public:
	SentMesagesDbModel(QSqlDatabase &db, QObject *parent = 0);

private:
	bool addMessage(int row, const QString &id, const QString &title,
	    const QString &recipient, const QString &status,
	    const QString &delivered, const QString &accepted);
	void fillContent(void);

	QSqlDatabase &m_dbRef; /*!< Reference to database. */
};


/*!
 * @brief Encapsulates message database.
 */
class MessageDb : public QObject {

public:
	MessageDb(const QString &connectionName, QObject *parent = 0);
	~MessageDb(void);

	/*!
	 * @brief Open database file.
	 */
	bool openDb(const QString &fileName);

	/*!
	 * @brief Return received messages model.
	 */
	QStandardItemModel * receivedModel(void);

	/*!
	 * @brief Return received messages model.
	 */
	QStandardItemModel * sentModel(void);

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
	QSqlDatabase m_db; /*!< Message database. */
	ReceivedMesagesDbModel m_receivedModel;
	SentMesagesDbModel m_sentModel;
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
	    const QString &locDir);

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
