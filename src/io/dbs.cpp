

#include <time.h>
#include <QDateTime>
#include <QDebug>
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
static
const QString dbShortDateTimeFormat("yyyy-MM-dd HH:mm:ss");


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


/* ========================================================================= */
/*
 * Converts date to format to be stored in database.
 */
QString timevalToDbFormat(const struct timeval *tv)
/* ========================================================================= */
{
	Q_ASSERT(NULL != tv);
	QDateTime timeStamp;

	timeStamp.setTime_t(tv->tv_sec);

	QString ret = timeStamp.toString(dbShortDateTimeFormat) + ".%1";
	Q_ASSERT(tv->tv_usec < 1000000);
	ret = ret.arg(QString::number(tv->tv_usec), 6, '0');

	//qDebug() << "timeStamp" << ret;

	return ret;
}


/* ========================================================================= */
/*
 * Converts date format to be stored in database.
 */
QString tmToDbFormat(const struct tm *t)
/* ========================================================================= */
{
	Q_ASSERT(NULL != t);

	QString ret = "%1-%2-%3 %4:%5:%6.%7";

	ret = ret.arg(QString::number(t->tm_year + 1900))
	    .arg(QString::number(t->tm_mon, 2, '0'))
	    .arg(QString::number(t->tm_mday, 2, '0'))
	    .arg(QString::number(t->tm_hour), 2, '0')
	    .arg(QString::number(t->tm_min), 2, '0')
	    .arg(QString::number(t->tm_sec), 2, '0')
	    .arg(QString::number(0), 3, '0');

	return ret;
}

/* ========================================================================= */
/*
 * Converts date format to be stored in database.
 */
QString tmBirthToDbFormat(const struct tm *t)
/* ========================================================================= */
{
	Q_ASSERT(NULL != t);

	QString ret = "%1-%2-%3";

	QString month = QString::number(t->tm_mon+1, 10);
	if (month.size() == 1) {
		month = "0" + month;
	}

	QString mday = QString::number(t->tm_mday, 10);
	if (mday.size() == 1) {
		mday = "0" + mday;
	}

	ret = ret.arg(QString::number(t->tm_year + 1900))
	    .arg(month)
	    .arg(mday);

	return ret;
}


/* ========================================================================= */
/*
 * Converts date to format to be stored in database.
 */
QString qDateTimeToDbFormat(const QDateTime &dateTime)
/* ========================================================================= */
{
	QString ret = dateTime.toString(dbDateTimeFormat) + "000";

	return ret;
}
