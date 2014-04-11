

#include <QDateTime>
#include <QString>

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
QString dateTimeFromDbFormat(const QString &dateTimeStr, const QString &tgtFmt)
/* ========================================================================= */
{
	QDateTime date = QDateTime::fromString(dateTimeStr, dbDateTimeFormat);
	if (date.isNull() || !date.isValid()) {
		/* Try the faulty format. */
		date = QDateTime::fromString(dateTimeStr,
		    dbFaultyDateTimeFormat);
	}
	if (date.isValid()) {
		return date.toString(tgtFmt);
	} else {
		return dateTimeStr;
	}
}
