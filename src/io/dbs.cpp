

#include <QDateTime>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

#include "dbs.h"


/*!
 * @brief Date/time format stored in db.
 *
 * @note Old implementation of datovka is likely to contain a bug.
 * Milliseconds are probably stored as microseconds.
 */
static
const QString dbFaultyDateTimeFormat("yyyy-MM-dd HH:mm:ss.000zzz");
static
const QString dbDateTimeFormat("yyyy-MM-dd HH:mm:ss.zzz");


/* ========================================================================= */
/*
 * Converts date from database format into desired format if possible.
 */
QDateTime dateTimeFromDbFormat(const QString &dateTimeDbStr)
/* ========================================================================= */
{
	QDateTime dateTime = QDateTime::fromString(dateTimeDbStr,
	    dbDateTimeFormat);
	if (dateTime.isNull() || !dateTime.isValid()) {
		/* Try the faulty format. */
		dateTime = QDateTime::fromString(dateTimeDbStr,
		    dbFaultyDateTimeFormat);
	}
	if (dateTime.isNull() || !dateTime.isValid()) {
		/* Try to ignore 3 rightmost characters. */
		dateTime = QDateTime::fromString(
		    dateTimeDbStr.left(dateTimeDbStr.size() - 3),
		    dbDateTimeFormat);
	}

	return dateTime;
}


/* ========================================================================= */
/*
 * Converts date from database format into desired format if possible.
 */
QString dateTimeStrFromDbFormat(const QString &dateTimeDbStr,
    const QString &tgtFmt)
/* ========================================================================= */
{
	QDateTime dateTime = dateTimeFromDbFormat(dateTimeDbStr);

	if (dateTime.isValid()) {
		return dateTime.toString(tgtFmt);
	} else {
		return QString();
	}
}
