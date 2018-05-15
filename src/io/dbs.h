/*
 * Copyright (C) 2014-2018 CZ.NIC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations including
 * the two.
 */

#pragma once

#include <QDate>
#include <QDateTime>
#include <QString>

/*!
 * @brief Converts date and time from database format into desired format if
 *     possible.
 */
QDateTime dateTimeFromDbFormat(const QString &dateTimeDbStr);

/*!
 * @brief Converts date and time to format to be stored in database.
 */
QString qDateTimeToDbFormat(const QDateTime &dateTime);

/*!
 * @brief Converts date and time from database format into desired format if
 *     possible.
 */
QString dateTimeStrFromDbFormat(const QString &dateTimeDbStr,
    const QString &tgtFmt);

/*!
 * @brief Converts date from database format into desired format if possible.
 */
QDate dateFromDbFormat(const QString &dateDbStr);

/*!
 * @brief Converts date to format to be stored in database.
 */
QString qDateToDbFormat(const QDate &date);

/*!
 * @brief Converts date from database format into desired format if possible.
 */
QString dateStrFromDbFormat(const QString &dateDbStr, const QString &tgtFmt);

/*!
 * @brief Converts time to format to be stored in database.
 */
QString timevalToDbFormat(const struct timeval *tv);

/*!
 * @brief Converts time to QDateTime.
 */
QDateTime timevalToDateTime(const struct timeval *tv);

/*!
 * @brief Converts date format to be stored in database.
 */
QString tmToDbFormat(const struct tm *t);

/*!
 * @brief Converts date format to be stored in database.
 */
QString tmBirthToDbFormat(const struct tm *t);

/*!
 * @brief Converts ISO datetime to format to be stored in database.
 */
QString fromIsoDatetimetoDbformat(const QString &dateTimeStr);

/*!
 * @brief Converts ISO datetime to datetime.
 */
QDateTime fromIsoDatetimetoDateTime(const QString &dateTimeStr);
