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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
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

#include <QCommandLineParser>

#include "src/datovka_shared/log/log_device.h"
#include "src/settings/preferences.h"

/*!
 * @brief Set default locale.
 */
void setDefaultLocale(void);

/*!
 * @brief Sets preferences and log device according to command line parameters.
 *
 * @param[in]     parser Command line parser.
 * @param[in,out] prefs Preferences structure to be set.
 * @param[in,out] log Log device to be set.
 * @return 0 on success, -1 on failure.
 */
int preferencesSetUp(const QCommandLineParser &parser, Preferences &prefs,
    LogDevice &log);

/*!
 * @brief Downloads CRL files and installs them into OpenSSL context.
 */
void downloadCRL(void);

/*!
 * @brief Loads application localisation.
 *
 * @param[in] prefs Preferences structure.
 */
void loadLocalisation(const Preferences &prefs);

/*!
 * @brief Log application version.
 */
void logAppVersion(void);

/*!
 * @brief Writes Qt versions to log.
 */
void logQtVersion(void);

/*!
 * @brief Writes time-zone information to log.
 */
void logTimeZone(void);

/*!
 * @brief Allocates global log facility.
 */
int allocGlobLog(void);

/*!
 * @brief Deallocates global log facility.
 */
void deallocGlobLog(void);

/*!
 * @brief Allocates global infrastructure providing basic functionality.
 */
int allocGlobInfrastruct(void);

/*!
 * @brief Deallocates global infrastructure.
 */
void deallocGlobInfrastruct(void);

/*!
 * @brief Allocates global settings instances.
 */
int allocGlobSettings(void);

/*!
 * @brief Deallocates global settings instances.
 */
void deallocGlobSettings(void);

/*!
 * @brief Allocates global database objects and containers.
 *
 * @param[in] prefs Preferences structure.
 * @return 0 on success, -1 on failure.
 */
int allocGlobContainers(const Preferences &prefs);

/*!
 * @brief Deallocates global database objects and containers.
 */
void deallocGlobContainers(void);
