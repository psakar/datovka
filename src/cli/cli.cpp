/*
 * Copyright (C) 2014-2015 CZ.NIC
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


#include "src/cli/cli.h"

/* ========================================================================= */
int parseService(const QString &service, const QString &paramString)
/* ========================================================================= */
{
	qDebug() << service << ":" << paramString;

	struct sendMsgStruct sendMsg;
	QString errmsg;
	QStringList itemList;
	QString label;
	QString value;

	QStringList paramList = paramString.split(",");
	if (paramList.isEmpty()) {
		errmsg = "Parameter list missing or its format is wrong.";
		return -1;
	}

	for (int i = 0; i < paramList.count(); ++i) {

		itemList = paramList.at(i).split("=");

		if (itemList.count() != 2) {
			errmsg = QString("Couple [%1] of parameter string has "
			    "wrong format or more tokens.").arg(i);
			qDebug() << errmsg;
			return -1;
		}

		label = itemList.at(0);
		if (label.isEmpty()) {
			errmsg = QString("Parameter name missing in "
			    "the couple [%1].").arg(i);
			qDebug() << errmsg;
			return -1;
		}

		value = itemList.at(1);
		if (value.isEmpty()) {
			errmsg = QString("Value string missing in "
			    "the couple [%1].").arg(i);
			qDebug() << errmsg;
			return -1;
		}

		itemList.clear();
		itemList = value.split("'");

		int cnt = itemList.count();

		if (cnt != 3) {
			errmsg = QString("Value string has a wrong format in "
			    "the couple [%1].").arg(i);
			qDebug() << errmsg;
			return -1;
		}

		if (!itemList.at(0).isEmpty()) {
			errmsg = QString("Wrong symbol(s) before value in the "
			    "couple [%1].").arg(i);
			qDebug() << errmsg;
			return -1;
		}

		if (itemList.at(1).isEmpty()) {
			errmsg = QString("Value of parameter '%1' missing in the "
			    "couple [%2].").arg(label).arg(i);
			qDebug() << errmsg;
			return -1;
		}

		if (!itemList.at(2).isEmpty()) {
			errmsg = QString("Wrong symbol(s) after value in the "
			    "couple [%1].").arg(i);
			qDebug() << errmsg;
			return -1;
		}

		value = itemList.at(1);

		qDebug() << "Label: " << label;
		qDebug() << "Value: " << value;


		if (itemList.isEmpty()) {
			errmsg = QString("Value string missing in the "
			    "couple [%1].").arg(i);
			qDebug() << errmsg;
			return -1;
		}

		if ("username" == label) {
			sendMsg.username = value;
		} else if ("dbIDSender" == label) {
			sendMsg.dbIDSender = value;
		}
	}

	return 0;
}

/* ========================================================================= */
int checkAndSetValues(const QString &service, const QString &label,
    const QString &value)
/* ========================================================================= */
{
	qDebug() << label  << value;
	return 0;
}

/* ========================================================================= */
int parseService2(const QString &service, const QString &paramString)
/* ========================================================================= */
{
	qDebug() << service << ":" << paramString;

	QString label = "";
	QString value = "";
	bool newLabel = true;
	bool newValue = false;
	bool special = false;

	for (int i = 0; i < paramString.length(); ++i) {
		if (paramString.at(i) == ',') {
			if (newValue) {
				value = value + paramString.at(i);
			} else {
				checkAndSetValues(service, label, value);
				label.clear();
				value.clear();
				newLabel = true;
				newValue = false;
			}
		} else if (paramString.at(i) == '=') {
			if (newValue) {
				value = value + paramString.at(i);
			} else {
				newLabel = false;
			}
		} else if (paramString.at(i) == '\'') {
			if (special) {
				value = value + paramString.at(i);
				special = false;
			} else {
				newValue = !newValue;
			}
		} else if (paramString.at(i) == '\\') {
			if (special) {
				value = value + paramString.at(i);
				special = false;
			} else {
				special = true;
			}
		} else {
			if (newLabel) {
				label = label + paramString.at(i);
			}
			if (newValue) {
				value = value + paramString.at(i);
			}
		}
	}

	checkAndSetValues(service, label, value);

	return 0;
}
