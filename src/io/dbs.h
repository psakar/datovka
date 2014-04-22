

#ifndef _DBS_H_
#define _DBS_H_


#include <QMap>
#include <QPair>
#include <QString>
#include <QVector>


/*!
 * @brief Used datatypes in databases.
 */
typedef enum {
	DB_INTEGER = 1,
	DB_TEXT,
	DB_BOOLEAN,
	DB_DATETIME
} dbEntryType;


/*!
 * @brief Table attribute property.
 */
class AttrProp {
public:
	dbEntryType type; /*!< Attribute type. */
	QString desc; /*!< Attribute description. */
};


/*!
 * @brief Table 'messages'.
 */
class MsgsTbl {
public:
	/*! Table name. */
	static
	const QString tabName;

	/*! Known attributes. */
	static
	const QVector< QPair<QString, dbEntryType> > knownAttrs;

	/*! Attribute properties. */
	static
	const QMap<QString, AttrProp> attrProps;

private:
	/* Prohibit all instances. */
	MsgsTbl(void);
};


/*!
 * @brief Table 'supplementary_message_data'.
 */
class SmsgdTbl {
public:
	/*! Table name. */
	static
	const QString tabName;

	/*! Known attributes. */
	static
	const QVector< QPair<QString, dbEntryType> > knownAttrs;

	/*! Attribute properties. */
	static
	const QMap<QString, AttrProp> attrProps;

private:
	/* Prohibit all instances. */
	SmsgdTbl(void);
};


/*!
 * @brief Converts date from database format into desired format if possible.
 */
QString dateTimeFromDbFormat(const QString &dateTimeStr, const QString &fmt);

#endif /* _DBS_H_ */
