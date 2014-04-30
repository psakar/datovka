

#ifndef _DBS_H_
#define _DBS_H_


#include <QMap>
#include <QPair>
#include <QString>
#include <QSqlDatabase>
#include <QVector>


/*!
 * @brief Used data types in databases.
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
 * @brief Table prototype.
 */
class Tbl {
public:
	/*
	 * Constructor.
	 */
	Tbl(const QString &name,
	    const QVector< QPair<QString, dbEntryType> > &attrs,
	    const QMap<QString, AttrProp> &props);

	/*! Table name. */
	const QString &tabName;

	/*! Known attributes. */
	const QVector< QPair<QString, dbEntryType> > &knownAttrs;

	/*! Attribute properties. */
	const QMap<QString, AttrProp> &attrProps;

	/*!
	 * @brief Return true if table in database exists.
	 */
	bool existsInDb(const QSqlDatabase &db) const;
};


extern const Tbl msgsTbl; /*!< Table 'messages'. */
extern const Tbl flsTbl; /*!< Table 'files'. */
extern const Tbl hshsTbl; /*!< Table 'hashes'. */
extern const Tbl evntsTbl; /*!< Table 'events'. */
extern const Tbl rwmsgdtTbl; /*!< Table 'raw_message_data'. */
extern const Tbl rwdlvrinfdtTbl; /*!< Table 'raw_delivery_info_data'. */
extern const Tbl smsgdtTbl; /*!< Table 'supplementary_message_data'. */
extern const Tbl crtdtTbl; /*!< Table 'certificate_data'. */
extern const Tbl msgcrtdtTbl; /*!< Table 'message_certificate_data'. */


/*!
 * @brief Converts date from database format into desired format if possible.
 */
QDateTime dateTimeFromDbFormat(const QString &dateTimeDbStr);


/*!
 * @brief Converts date from database format into desired format if possible.
 */
QString dateTimeStrFromDbFormat(const QString &dateTimeDbStr,
    const QString &tgtFmt);


#endif /* _DBS_H_ */
