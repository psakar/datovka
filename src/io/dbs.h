

#ifndef _DBS_H_
#define _DBS_H_


#include <QDateTime>
#include <QString>


/*!
 * @brief Converts date from database format into desired format if possible.
 */
QDateTime dateTimeFromDbFormat(const QString &dateTimeDbStr);


/*!
 * @brief Converts date from database format into desired format if possible.
 */
QString dateTimeStrFromDbFormat(const QString &dateTimeDbStr,
    const QString &tgtFmt);


/*!
 * @brief Converts date to format to be stored in database.
 */
QString timevalToDbFormat(const struct timeval *tv);


#endif /* _DBS_H_ */
