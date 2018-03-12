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

#include <QSettings>

#include "src/log/log.h"
#include "src/settings/registry.h"

/*
 * System wide registry entries have precedence. If system-wide and user
 * settings conflict, then system settings should be taken.
 */

/* System-wide registry root. */
#define SYS_REG_SW_ROOT "HKEY_LOCAL_MACHINE\\Software"

/* User registry root. */
#define USR_REG_SW_ROOT "HKEY_CURRENT_USER\\Software"

/* Application location. */
#define APP_LOC "CZ.NIC\\Datovka"

#define REG_FMT QSettings::NativeFormat

/*
 * 32- and 64- bit registry are separated. The 64-bit regedit displays the
 * 32-bit registry entries in "HKEY_LOCAL_MACHINE\\Software\\WOW6432Node".
 * Datovka for Windows is a 32-bit application, therefore you must use the
 * aforementioned registry path (containing WOW6432Node).
 * https://support.microsoft.com/en-us/help/305097/how-to-view-the-system-registry-by-using-64-bit-versions-of-windows
 */

/*!
 * @brief Converts entry enum into registry name.
 */
static
QString entryName(enum RegPreferences::Entry entry)
{
	switch (entry) {
	case RegPreferences::ENTR_NEW_VER_NOTIF:
		return "NewVersionNotification";
		break;
	default:
		Q_ASSERT(0);
		return QString();
		break;
	}
}

bool RegPreferences::haveSys(enum Entry entry)
{
	QString name(entryName(entry));
	if (name.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	QSettings settings(SYS_REG_SW_ROOT "\\" APP_LOC, REG_FMT);

	//bool ret = settings.childKeys().contains(name, Qt::CaseInsensitive);
	bool ret = settings.contains(name);

	logInfoNL("Registry entry '%s': %s",
	    (SYS_REG_SW_ROOT "\\" APP_LOC "\\" + name).toUtf8().constData(),
	    ret ? "present" : "missing");

	return ret;
}

bool RegPreferences::haveUsr(enum Entry entry)
{
	QString name(entryName(entry));
	if (name.isEmpty()) {
		Q_ASSERT(0);
		return false;
	}

	QSettings settings(USR_REG_SW_ROOT "\\" APP_LOC, REG_FMT);

	//bool ret = settings.childKeys().contains(name, Qt::CaseInsensitive);
	bool ret = settings.contains(name);

	logInfoNL("Registry entry '%s': %s",
	    (USR_REG_SW_ROOT "\\" APP_LOC "\\" + name).toUtf8().constData(),
	    ret ? "present" : "missing");

	return ret;
}

bool RegPreferences::sysNewVersionNotification(void)
{
	if (!haveSys(ENTR_NEW_VER_NOTIF)) {
		Q_ASSERT(0);
		return true;
	}
	QString name(entryName(ENTR_NEW_VER_NOTIF));
	Q_ASSERT(!name.isEmpty());

	QSettings settings(SYS_REG_SW_ROOT "\\" APP_LOC, REG_FMT);

	bool ret = settings.value(name, true).toBool();

	logInfoNL("registry entry '%s': %s",
	    (SYS_REG_SW_ROOT "\\" APP_LOC + name).toUtf8().constData(),
	    ret ? "true" : "false");

	return ret;
}

bool RegPreferences::usrNewVersionNotification(void)
{
	if (!haveUsr(ENTR_NEW_VER_NOTIF)) {
		Q_ASSERT(0);
		return true;
	}
	QString name(entryName(ENTR_NEW_VER_NOTIF));
	Q_ASSERT(!name.isEmpty());

	QSettings settings(USR_REG_SW_ROOT "\\" APP_LOC, REG_FMT);

	bool ret = settings.value(name, true).toBool();

	logInfoNL("registry entry '%s': %s",
	    (USR_REG_SW_ROOT "\\" APP_LOC "\\" + name).toUtf8().constData(),
	    ret ? "true" : "false");

	return ret;
}
