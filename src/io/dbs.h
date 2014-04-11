

#ifndef _DBS_H_
#define _DBS_H_


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
 * @brief Converts date from database format into desired format if possible.
 */
QString dateTimeFromDbFormat(const QString &dateTimeStr, const QString &fmt);

#endif /* _DBS_H_ */
