/*
 * Copyright (C) 2014-2017 CZ.NIC
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

QString IsdsConversion::boxStateToText(int val)
{
	switch (val) {
	case DBSTATE_ACCESSIBLE:
		/* Datová schránka je přístupná, lze do ní dodávat zprávy, na Portále lze vyhledat. */
		return tr(
		    "The data box is accessible. It is possible to send messages into it. It can be looked up on the Portal.");
		break;
	case DBSTATE_TEMP_UNACCESSIBLE:
		/* Datová schránka je dočasně znepřístupněna (na vlastní žádost), může být později opět zpřístupněna. */
		return tr(
		    "The data box is temporarily inaccessible (at own request). It may be made accessible again at some point in the future.");
		break;
	case DBSTATE_NOT_YET_ACCESSIBLE:
		/* Datová schránka je dosud neaktivní. Vlastník schránky se musí poprvé přihlásit do webového rozhraní, aby došlo k aktivaci schránky. */
		return tr(
		    "The data box is so far inactive. The owner of the box has to log into the web interface at first in order to activate the box.");
		break;
	case DBSTATE_PERM_UNACCESSIBLE:
		/* Datová schránka je trvale znepřístupněna, čeká na smazání (může být opět zpřístupněna). */
		return tr(
		    "The data box is permanently inaccessible. It is waiting to be deleted (but it may be made accessible again).");
		break;
	case DBSTATE_REMOVED:
		/* Datová schránka je smazána (přesto existuje v ISDS). */
		return tr(
		    "The data box has been deleted (none the less it exists in ISDS).");
		break;
	case DBSTATE_TEMP_UNACCESSIBLE_LAW:
		/* Datová schránka je dočasně znepřístupněna (z důvodů vyjmenovaných v zákoně), může být později opět zpřístupněna. */
		return tr(
		    "The data box is temporarily inaccessible (because of reasons listed in law). It may be made accessible again at some point in the future.");
		break;
	default:
		logWarningNL("Unknown data box status value '%d'.", val);
		return tr("An error occurred while checking the status.");
		break;
	}
}

/*
 * DBTYPE_OVM_MAIN is a special value introduced in git version of libisds.
 * It appears not to be included in officially released source packages.
 * TODO -- Check the function of DBTYPE_OVM_MAIN.
 */
#define STR_OVM_MAIN "OVM_MAIN" /* hlavni schranky */
#define STR_SYSTEM "SYSTEM"
#define STR_OVM "OVM"
#define STR_OVM_NOTAR "OVM_NOTAR"
#define STR_OVM_EXEKUT "OVM_EXEKUT"
#define STR_OVM_REQ "OVM_REQ"
#define STR_OVM_FO "OVM_FO"
#define STR_OVM_PFO "OVM_PFO"
#define STR_OVM_PO "OVM_PO"
#define STR_PO "PO"
#define STR_PO_ZAK "PO_ZAK"
#define STR_PO_REQ "PO_REQ"
#define STR_PFO "PFO"
#define STR_PFO_ADVOK "PFO_ADVOK"
#define STR_PFO_DANPOR "PFO_DANPOR"
#define STR_PFO_INSSPR "PFO_INSSPR"
#define STR_PFO_AUDITOR "PFO_AUDITOR"
#define STR_FO "FO"

const QString &IsdsConversion::boxTypeToStr(int val)
{
	static const QString system(STR_SYSTEM),
	    ovm(STR_OVM), ovmNotar(STR_OVM_NOTAR), ovmExekut(STR_OVM_EXEKUT),
	    ovmReq(STR_OVM_REQ), ovmFo(STR_OVM_FO), ovmPfo(STR_OVM_PFO),
	    ovmPo(STR_OVM_PO), po(STR_PO), poZak(STR_PO_ZAK), poReq(STR_PO_REQ),
	    pfo(STR_PFO), pfoAdvok(STR_PFO_ADVOK), pfoDanpor(STR_PFO_DANPOR),
	    pfoInsspr(STR_PFO_INSSPR), pfoAuditor(STR_PFO_AUDITOR), fo(STR_FO);
	static const QString invalid;

	switch (val) {
	case DBTYPE_SYSTEM: return system; break;
	case DBTYPE_OVM: return ovm; break;
	case DBTYPE_OVM_NOTAR: return ovmNotar; break;
	case DBTYPE_OVM_EXEKUT: return ovmExekut; break;
	case DBTYPE_OVM_REQ: return ovmReq; break;
	case DBTYPE_OVM_FO: return ovmFo; break;
	case DBTYPE_OVM_PFO: return ovmPfo; break;
	case DBTYPE_OVM_PO: return ovmPo; break;
	case DBTYPE_PO: return po; break;
	case DBTYPE_PO_ZAK: return poZak; break;
	case DBTYPE_PO_REQ: return poReq; break;
	case DBTYPE_PFO: return pfo; break;
	case DBTYPE_PFO_ADVOK: return pfoAdvok; break;
	case DBTYPE_PFO_DANPOR: return pfoDanpor; break;
	case DBTYPE_PFO_INSSPR: return pfoInsspr; break;
	case DBTYPE_PFO_AUDITOR: return pfoAuditor; break;
	case DBTYPE_FO: return fo; break;
	default:
		logWarningNL("Unknown data box type value '%d'.", val);
		return invalid;
		break;
	}
}

int IsdsConversion::boxTypeStrToInt(const QString &val)
{
	if (val == QLatin1String(STR_SYSTEM)) return DBTYPE_SYSTEM;
	else if (val == QLatin1String(STR_OVM)) return DBTYPE_OVM;
	else if (val == QLatin1String(STR_OVM_NOTAR)) return DBTYPE_OVM_NOTAR;
	else if (val == QLatin1String(STR_OVM_EXEKUT)) return DBTYPE_OVM_EXEKUT;
	else if (val == QLatin1String(STR_OVM_REQ)) return DBTYPE_OVM_REQ;
	else if (val == QLatin1String(STR_OVM_FO)) return DBTYPE_OVM_FO;
	else if (val == QLatin1String(STR_OVM_PFO)) return DBTYPE_OVM_PFO;
	else if (val == QLatin1String(STR_OVM_PO)) return DBTYPE_OVM_PO;
	else if (val == QLatin1String(STR_PO)) return DBTYPE_PO;
	else if (val == QLatin1String(STR_PO_ZAK)) return DBTYPE_PO_ZAK;
	else if (val == QLatin1String(STR_PO_REQ)) return DBTYPE_PO_REQ;
	else if (val == QLatin1String(STR_PFO)) return DBTYPE_PFO;
	else if (val == QLatin1String(STR_PFO_ADVOK)) return DBTYPE_PFO_ADVOK;
	else if (val == QLatin1String(STR_PFO_DANPOR)) return DBTYPE_PFO_DANPOR;
	else if (val == QLatin1String(STR_PFO_INSSPR)) return DBTYPE_PFO_INSSPR;
	else if (val == QLatin1String(STR_PFO_AUDITOR)) return DBTYPE_PFO_AUDITOR;
	else if (val == QLatin1String(STR_FO)) return DBTYPE_FO;
	else {
		logWarningNL("Unknown data box type '%s'.",
		    val.toUtf8().constData());
		return DBTYPE_SYSTEM;
	}
}

QString IsdsConversion::senderBoxTypeToText(int val)
{
	switch (val) {
//	case DBTYPE_OVM_MAIN: return tr("public authority - main box"); break; /* organ verejne moci - hlavni schranka */
	case DBTYPE_SYSTEM: return tr("system box"); break; /* systemova schranka */
	case DBTYPE_OVM: return tr("public authority"); break; /* organ verejne moci */
	case DBTYPE_OVM_NOTAR: return tr("public authority - notary"); break; /* organ verejne moci - notar */
	case DBTYPE_OVM_EXEKUT: return tr("public authority - bailiff"); break; /* organ verejne moci - exekutor */
	case DBTYPE_OVM_REQ: return tr("public authority - at request"); break; /* organ verejne moci - na zadost */
	case DBTYPE_OVM_FO: return tr("public authority - natural person"); break; /* organ verejne moci - fyzicka osoba */
	case DBTYPE_OVM_PFO: return tr("public authority - self-employed person"); break; /* organ verejne moci - podnikajici fyzicka osoba */
	case DBTYPE_OVM_PO: return tr("public authority - legal person"); break; /* organ verejne moci - pravnicka osoba */
	case DBTYPE_PO: return tr("legal person"); break; /* pravnicka osoba */
	case DBTYPE_PO_ZAK: return tr("legal person - founded by an act"); break; /* pravnicka osoba - ze zakona */
	case DBTYPE_PO_REQ: return tr("legal person - at request"); break; /* pravnicka osoba - na zadost */
	case DBTYPE_PFO: return tr("self-employed person"); break; /* podnikajici fyzicka osoba */
	case DBTYPE_PFO_ADVOK: return tr("self-employed person - advocate"); break; /* podnikajici fyzicka osoba - advokat */
	case DBTYPE_PFO_DANPOR: return tr("self-employed person - tax advisor"); break; /* podnikajici fyzicka osoba - danovy poradce */
	case DBTYPE_PFO_INSSPR: return tr("self-employed person - insolvency administrator"); break; /* podnikajici fyzicka osoba - insolvencni spravce */
	case DBTYPE_PFO_AUDITOR: return tr("self-employed person - statutory auditor"); break; /* podnikajici fyzicka osoba - auditor */
	case DBTYPE_FO: return tr("natural person"); break; /* fyzicka osoba */
	default:
		logWarningNL("Unknown sender data box type value '%d'.", val);
		return QString();
		break;
	}
}

QString IsdsConversion::dmTypeToText(const QString &val)
{
	if (val.size() != 1) {
		logWarningNL("Unknown message type value '%s'.",
		    val.toUtf8().constData());
		return QString();
	}

	/*
	 * See Provozni rad ISDS:
	 * Webove sluyby rozhrani ISDS pro manipulaci s datovymi zpravami
	 * section 2.8.2, GetListOfSentMessages
	 */

	switch (val[0].toLatin1()) {
	case 'A':
		return tr(
		    "Subsidised postal data message, initiating reply postal data message");
		break;
	case 'B':
		return tr(
		    "Subsidised postal data message, initiating reply postal data message - used for sending reply");
		break;
	case 'C':
		return tr(
		    "Subsidised postal data message, initiating reply postal data message - unused for sending reply, expired");
		break;
	case 'E':
		return tr("Postal data message sent using a subscription (prepaid credit)");
		break;
	case 'G':
		/* ??? */
		return tr(
		    "Postal data message sent in endowment mode by another data box to the benefactor account");
		break;
	case 'K':
		return tr("Postal data message");
		break;
	case 'I':
		return tr("Initiating postal data message");
		break;
	case 'O':
		return tr(
		    "Reply postal data message; sent at the expense of the sender of the initiating postal data message");
		break;
	case 'V':
		return tr("Public message (recipient or sender is a public authority)");
		break;
	case 'X':
		return tr("Initiating postal data message - unused for sending reply, expired");
		break;
	case 'Y':
		return tr("Initiating postal data message - used for sending reply");
		break;
	default:
		logWarningNL("Unknown message type value '%c'.",
		    val[0].toLatin1());
		return QString();
		break;
	}
}

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
