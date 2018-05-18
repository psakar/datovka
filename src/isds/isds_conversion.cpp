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

#include "src/io/isds_sessions.h"
#include "src/isds/isds_conversion.h"
#include "src/log/log.h"

int IsdsConversion::msgStatusIsdsToDbRepr(int val)
{
	switch (val) {
	case MESSAGESTATE_SENT: return 1; break;
	case MESSAGESTATE_STAMPED: return 2; break;
	case MESSAGESTATE_INFECTED: return 3; break;
	case MESSAGESTATE_DELIVERED: return 4; break;
	case MESSAGESTATE_SUBSTITUTED: return 5; break;
	case MESSAGESTATE_RECEIVED: return 6; break;
	case MESSAGESTATE_READ: return 7; break;
	case MESSAGESTATE_UNDELIVERABLE: return 8; break;
	case MESSAGESTATE_REMOVED: return 9; break;
	case MESSAGESTATE_IN_SAFE: return 10; break;
	default:
		logWarningNL("Unknown message state value '%d'.", val);
		return 0;
		break;
	}
}

#define STR_PRIMARY_USER "PRIMARY_USER"
#define STR_ENTRUSTED_USER "ENTRUSTED_USER"
#define STR_ADMINISTRATOR "ADMINISTRATOR"
#define STR_OFFICIAL "OFFICIAL"
#define STR_VIRTUAL "VIRTUAL"
#define STR_OFFICIAL_CERT "OFFICIAL_CERT"
#define STR_LIQUIDATOR "LIQUIDATOR"
#define STR_RECEIVER "RECEIVER"
#define STR_GUARDIAN "GUARDIAN"

const QString &IsdsConversion::senderTypeToStr(int val)
{
	static const QString pu(STR_PRIMARY_USER), eu(STR_ENTRUSTED_USER),
	    a(STR_ADMINISTRATOR), o(STR_OFFICIAL), v(STR_VIRTUAL),
	    oc(STR_OFFICIAL_CERT), l(STR_LIQUIDATOR), r(STR_RECEIVER),
	    g(STR_GUARDIAN);
	static const QString invalid;

	switch (val) {
	case SENDERTYPE_PRIMARY: return pu; break;
	case SENDERTYPE_ENTRUSTED: return eu; break;
	case SENDERTYPE_ADMINISTRATOR: return a; break;
	case SENDERTYPE_OFFICIAL: return o; break;
	case SENDERTYPE_VIRTUAL: return v; break;
	case SENDERTYPE_OFFICIAL_CERT: return oc; break;
	case SENDERTYPE_LIQUIDATOR: return l; break;
	case SENDERTYPE_RECEIVER: return r; break;
	case SENDERTYPE_GUARDIAN: return g; break;
	default:
		logWarningNL("Unknown sender type value '%d'.", val);
		return invalid;
		break;
	}
}

QString IsdsConversion::senderTypeStrToText(const QString &val)
{
	if (val == QLatin1String(STR_PRIMARY_USER)) {
		/* Opravnena osoba nebo likvidator. */
		return tr("Primary user");
	} else if (val == QLatin1String(STR_ENTRUSTED_USER)) {
		/* Poverena osoba. */
		return tr("Entrusted user");
	} else if (val == QLatin1String(STR_ADMINISTRATOR)) {
		/* Systemove DZ. */
		return tr("Administrator");
	} else if (val == QLatin1String(STR_OFFICIAL)) {
		/* Systemove DZ. */
		return tr("Official");
	} else if (val == QLatin1String(STR_VIRTUAL)) {
		/* Spisovka. */
		return tr("Virtual");
	} else if (val == QLatin1String(STR_OFFICIAL_CERT)) {
		return tr("???");
	} else if (val == QLatin1String(STR_LIQUIDATOR)) {
		return tr("Liquidator");
	} else if (val == QLatin1String(STR_RECEIVER)) {
		return tr("Receiver");
	} else if (val == QLatin1String(STR_GUARDIAN)) {
		return tr("Guardian");
	} else {
		logWarningNL("Unknown author type '%s'.",
		    val.toUtf8().constData());
		return QString();
	}
}

QString IsdsConversion::userPrivilsToText(int val)
{
	QString privStr;
	const QString sepPref("<li>- "), sepSuff("</li>");

	if (val == 255) {
		return tr("Full control");
	} else {
		privStr = tr("Restricted control");
	}

	if (val & PRIVIL_READ_NON_PERSONAL) {
		// "stahovat a číst došlé DZ"
		privStr += sepPref + tr("download and read incoming DM") +
		    sepSuff;
	}
	if (val & PRIVIL_READ_ALL) {
		// "stahovat a číst DZ určené do vlastních rukou"
		privStr += sepPref +
		    tr("download and read DM sent into own hands") + sepSuff;
	}
	if (val & PRIVIL_CREATE_DM) {
		// "vytvářet a odesílat DZ, stahovat odeslané DZ"
		privStr += sepPref +
		    tr("create and send DM, download sent DM") + sepSuff;
	}
	if (val & PRIVIL_VIEW_INFO) {
		// "načítat seznamy DZ, Dodejky a Doručenky"
		privStr += sepPref +
		    tr("retrieve DM lists, delivery and acceptance reports") +
		    sepSuff;
	}
	if (val & PRIVIL_SEARCH_DB) {
		// "vyhledávat DS"
		privStr += sepPref + tr("search for data boxes") + sepSuff;
	}
	if (val & PRIVIL_OWNER_ADM) {
		// "spravovat DS"
		privStr += sepPref + tr("manage the data box") + sepSuff;
	}
	if (val & PRIVIL_READ_VAULT) {
		// "číst zprávy v DT"
		privStr += sepPref + tr("read message in data vault") + sepSuff;
	}
	if (val & PRIVIL_ERASE_VAULT) {
		// "mazat zprávy v DT"
		privStr += sepPref + tr("erase messages from data vault") +
		    sepSuff;
	}
	return privStr;
}
