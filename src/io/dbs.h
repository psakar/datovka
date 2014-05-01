

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


#endif /* _DBS_H_ */
