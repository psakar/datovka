

#ifndef _DBS_H_
#define _DBS_H_


#include <QMap>
#include <QPair>
#include <QString>
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
 * @brief Table 'files'.
 */
class FlsTbl {
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
	FlsTbl(void);
};


/*!
 * @brief Table 'hashes'.
 */
class HshsTbl {
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
	HshsTbl(void);
};


/*!
 * @brief Table 'events'.
 */
class EvntsTbl {
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
	EvntsTbl(void);
};


/*!
 * @brief Table 'raw_message_data'.
 */
class RwmsgdtTbl {
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
	RwmsgdtTbl(void);
};


/*!
 * @brief Table 'raw_delivery_info_data'.
 */
class RwdlvrinfdtTbl {
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
	RwdlvrinfdtTbl(void);
};


/*!
 * @brief Table 'supplementary_message_data'.
 */
class SmsgdtTbl {
public:
	class Entry {
	public:
		int message_id;
		int message_type;
		bool read_locally;
		QString download_date;
		QString custom_data;
	};

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
	SmsgdtTbl(void);
};


/*!
 * @brief Table 'certificate_data'
 */
class CrtdtTbl {
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
	CrtdtTbl(void);
};


/*!
 * @brief Table 'message_certificate_data'.
 */
class MsgcrtdtTbl {
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
	MsgcrtdtTbl(void);
};


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
