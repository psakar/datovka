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


#include "common.h"
#include "src/io/isds_sessions.h"
#include "src/log/log.h"


const QString dateTimeDisplayFormat("dd.MM.yyyy HH:mm:ss");
const QString dateDisplayFormat("dd.MM.yyyy");

QLocale programLocale;


/* ========================================================================= */
/*
 * Translates message type to text.
 */
const QString dmTypeToText(const QString &dmType)
/* ========================================================================= */
{
	if (dmType.size() != 1) {
		return QString();
	}

	switch (dmType[0].toLatin1()) {
	case 'K':
		return QObject::tr("Postal data message");
		break;
	case 'I':
		return QObject::tr("Initializing postal data message");
		break;
	case 'O':
		return QObject::tr("Reply postal data message");
		break;
	case 'X':
		return QObject::tr(
		    "Initializing postal data message - expired");
		break;
	case 'Y':
		return QObject::tr("Initializing postal data message - used");
		break;
	default:
		return QString();
		break;
	}
}


/* ========================================================================= */
/*
 * Translates author type to text.
 */
const QString authorTypeToText(const QString &authorType)
/* ========================================================================= */
{
	if ("PRIMARY_USER" == authorType) {
		/* Opravnena osoba nebo likvidator. */
		return QObject::tr("Primary user");
	} else if ("ENTRUSTED_USER" == authorType) {
		/* Poverena osoba. */
		return QObject::tr("Entrusted user");
	} else if ("ADMINISTRATOR" == authorType) {
		/* Systemove DZ. */
		return QObject::tr("Administrator");
	} else if ( "OFFICIAL" == authorType) {
		/* Systemove DZ. */
		return QObject::tr("Official");
	} else if ("VIRTUAL" == authorType) {
		/* Spisovka. */
		return QObject::tr("Virtual");
	} else {
		return QString();
	}
}


/* ========================================================================= */
/*
 * Returns message status description.
 */
const QString msgStatusToText(int status)
/* ========================================================================= */
{
	switch (status) {
	case 1:
		/* Zprava byla podana (vznikla v ISDS). */
		return QObject::tr("Message has been submitted (has been created in ISDS)");
		break;
	case 2:
		/*
		 * Datová zprava vcetne pisemnosti podepsana casovym razitkem.
		 */
		return QObject::tr("Data message and including its attachments signed with time-stamp.");
		break;
	case 3:
		/*
		 * Zprava neprosla AV kontrolou; nakazena pisemnost je smazana;
		 * konecny stav zpravy pred smazanim.
		 */
		return QObject::tr("Message did not pass through AV check; "
		    "infected paper deleted; final status before deletion.");
		break;
	case 4:
		/* Zprava dodana do ISDS (zapsan cas dodani). */
		return QObject::tr("Message handed into ISDS "
		    "(delivery time recorded).");
		break;
	case 5:
		/*
		 * Uplynulo 10 dnu od dodani verejne zpravy, ktera dosud nebyla
		 * dorucena prihlasenim (predpoklad dorucení fikci u neOVM DS);
		 * u komercni zpravy nemuze tento stav nastat.
		 */
		return QObject::tr("10 days have passed since the delivery of "
		    "the public message which has not been accepted by "
		    "logging-in (assumption of delivery by fiction in nonOVM "
		    "DS); this state cannot occur for commercial messages.");
		break;
	case 6:
		/*
		 * Osoba opravnena cist tuto zpravu se prihlasila - dodana
		 * zprava byla dorucena.",
		 */
		return QObject::tr("A person authorised to read this message "
		    "has logged in -- delivered message has been accepted.");
		break;
	case 7:
		/* Zprava byla prectena (na portale nebo akci ESS). */
		return QObject::tr("Message has been read (on the portal or "
		    "by ESS action).");
		break;
	case 8:
		/*
		 * Zprava byla oznacena jako nedorucitelna, protoze DS adresata
		 * byla zpetne znepristupnena.
		 */
		return QObject::tr("Message marked as undeliverable because "
		    "the target DS has been made inaccessible.");
		break;
	case 9:
		/*
		 * Obsah zpravy byl smazan, obalka zpravy vcetne hashu
		 * presunuta do archivu.
		 */
		return QObject::tr("Message content deleted, envelope "
		    "including hashes has been moved into archive.");
		break;
	case 10:
		/* Zprava je v Datovem trezoru. */
		return QObject::tr("Message resides in data vault.");
		break;
	default:
		return QString();
		break;
	}
}


/* ========================================================================= */
/*
 * Return privilegs as html string from number representation.
 */
QString convertUserPrivilsToString(int userPrivils)
/* ========================================================================= */
{
	QString privStr;
	const QString sepPref("<li>- "), sepSuff("</li>");

	if (userPrivils == 255) {
		return QObject::tr("Full control");
	} else {
		privStr = QObject::tr("Restricted control");
	}

	if (userPrivils & PRIVIL_READ_NON_PERSONAL) {
		// "stahovat a číst došlé DZ"
		privStr += sepPref +
		    QObject::tr("download and read incoming DM") + sepSuff;
	}
	if (userPrivils & PRIVIL_READ_ALL) {
		// "stahovat a číst DZ určené do vlastních rukou"
		privStr += sepPref +
		    QObject::tr("download and read DM sent into own hands") +
		    sepSuff;
	}
	if (userPrivils & PRIVIL_CREATE_DM) {
		// "vytvářet a odesílat DZ, stahovat odeslané DZ"
		privStr += sepPref +
		    QObject::tr("create and send DM, download sent DM") +
		    sepSuff;
	}
	if (userPrivils & PRIVIL_VIEW_INFO) {
		// "načítat seznamy DZ, Dodejky a Doručenky"
		privStr += sepPref +
		    QObject::tr(
		        "retrieve DM lists, delivery and acceptance reports") +
		    sepSuff;
	}
	if (userPrivils & PRIVIL_SEARCH_DB) {
		// "vyhledávat DS"
		privStr += sepPref +
		    QObject::tr("search for data boxes") + sepSuff;
	}
	if (userPrivils & PRIVIL_OWNER_ADM) {
		// "spravovat DS"
		privStr += sepPref + QObject::tr("manage the data box") +
		    sepSuff;
	}
	if (userPrivils & PRIVIL_READ_VAULT) {
		// "číst zprávy v DT"
		privStr += sepPref +
		    QObject::tr("read message in data vault") + sepSuff;
	}
	if (userPrivils & PRIVIL_ERASE_VAULT) {
		// "mazat zprávy v DT"
		privStr += sepPref +
		    QObject::tr("erase messages from data vault") + sepSuff;
	}
	return privStr;
}


/* ========================================================================= */
/*
 * Convert hex to dec number
 */
int convertHexToDecIndex(int value)
/* ========================================================================= */
{
	if (value == MESSAGESTATE_SENT) return 1;
	else if (value == MESSAGESTATE_STAMPED) return 2;
	else if (value == MESSAGESTATE_INFECTED) return 3;
	else if (value == MESSAGESTATE_DELIVERED) return 4;
	else if (value == MESSAGESTATE_SUBSTITUTED) return 5;
	else if (value == MESSAGESTATE_RECEIVED) return 6;
	else if (value == MESSAGESTATE_READ) return 7;
	else if (value == MESSAGESTATE_UNDELIVERABLE) return 8;
	else if (value == MESSAGESTATE_REMOVED) return 9;
	else if (value == MESSAGESTATE_IN_SAFE) return 10;
	else return 0;
}


/* ========================================================================= */
/*
 * Convert hash algorithm to string
 */
QString convertHashAlg(int value)
/* ========================================================================= */
{
	if (value == HASH_ALGORITHM_MD5) return "MD5";
	else if (value == HASH_ALGORITHM_SHA_1) return "SHA-1";
	else if (value == HASH_ALGORITHM_SHA_224) return "SHA-224";
	else if (value == HASH_ALGORITHM_SHA_256) return "SHA-256";
	else if (value == HASH_ALGORITHM_SHA_384) return "SHA-384";
	else if (value == HASH_ALGORITHM_SHA_512) return "SHA-512";
	else return "";
}


/* ========================================================================= */
/*
 * Convert hash algorithm to int
 */
int convertHashAlg2(QString value)
/* ========================================================================= */
{
	if (value == "MD5") return HASH_ALGORITHM_MD5;
	else if (value == "SHA-1") return HASH_ALGORITHM_SHA_1;
	else if (value == "SHA-224") return HASH_ALGORITHM_SHA_224;
	else if (value == "SHA-256") return HASH_ALGORITHM_SHA_256;
	else if (value == "SHA-384") return HASH_ALGORITHM_SHA_384;
	else if (value == "SHA-512") return HASH_ALGORITHM_SHA_512;
	else return 0;
}



/* ========================================================================= */
/*
 * Convert type of attachment files
 */
QString convertAttachmentType(int value)
/* ========================================================================= */
{
	if (value == FILEMETATYPE_MAIN) return "main";
	else if (value == FILEMETATYPE_ENCLOSURE) return "encl";
	else if (value == FILEMETATYPE_SIGNATURE) return "sign";
	else if (value == FILEMETATYPE_META) return "meta";
	else return "";
}

/* ========================================================================= */
/*
 * Convert type of author to string
 */
QString convertSenderTypeToString(int value)
/* ========================================================================= */
{
	if (value == SENDERTYPE_PRIMARY) return "PRIMARY_USER";
	else if (value == SENDERTYPE_ENTRUSTED) return "ENTRUSTED_USER";
	else if (value == SENDERTYPE_ADMINISTRATOR) return "ADMINISTRATOR";
	else if (value == SENDERTYPE_OFFICIAL) return "OFFICIAL";
	else if (value == SENDERTYPE_VIRTUAL) return "VIRTUAL";
	else if (value == SENDERTYPE_OFFICIAL_CERT) return "OFFICIAL_CERT";
	else if (value == SENDERTYPE_LIQUIDATOR) return "LIQUIDATOR";
	else return "";
}


/* ========================================================================= */
/*
 * Convert type of databox to string
 */
QString convertDbTypeToString(int value)
/* ========================================================================= */
{
	if (value == DBTYPE_SYSTEM) return "SYSTEM";
	else if (value == DBTYPE_OVM) return "OVM";
	else if (value == DBTYPE_OVM_NOTAR) return "OVM_NOTAR";
	else if (value == DBTYPE_OVM_EXEKUT) return "OVM_EXEKUT";
	else if (value == DBTYPE_OVM_REQ) return "OVM_REQ";
	else if (value == DBTYPE_PO) return "PO";
	else if (value == DBTYPE_PO_ZAK) return "PO_ZAK";
	else if (value == DBTYPE_PO_REQ) return "PO_REQ";
	else if (value == DBTYPE_PFO) return "PFO";
	else if (value == DBTYPE_PFO_ADVOK) return "PFO_ADVOK";
	else if (value == DBTYPE_PFO_DANPOR) return "PFO_DANPOR";
	else if (value == DBTYPE_PFO_INSSPR) return "PFO_INSSPR";
	else if (value == DBTYPE_FO) return "FO";
	else return "";
}


/* ========================================================================= */
/*
 * Convert type of user to string
 */
const QString & convertUserTypeToString(int value)
/* ========================================================================= */
{
	static const QString pu("PRIMARY_USER"), eu("ENTRUSTED_USER"),
	    a("ADMINISTRATOR"), l("LIQUIDATOR"), ou("OFFICIAL_USER"),
	    ocu("OFFICIAL_CERT_USER");
	static const QString empty;

	switch (value) {
	case USERTYPE_PRIMARY: return pu;
	case USERTYPE_ENTRUSTED: return eu;
	case USERTYPE_ADMINISTRATOR: return a;
	case USERTYPE_LIQUIDATOR: return l;
	case USERTYPE_OFFICIAL: return ou;
	case USERTYPE_OFFICIAL_CERT: return ocu;
	default: return empty;
	}
}


/* ========================================================================= */
/*
 * Return sender databox type as string from number representation.
 */
QString convertSenderDbTypesToString(int value)
/* ========================================================================= */
{
	/* System ISDS */
	if (value == DBTYPE_SYSTEM) return QObject::tr("System ISDS");
	/* OVM */
	else if (value == DBTYPE_OVM) return QObject::tr("Public authority");
	/* PO */
	else if (value == DBTYPE_PO) return QObject::tr("Legal person");
	/* PFO (OSVC) */
	else if (value == DBTYPE_PFO) return QObject::tr("Self-employed person");
	/* FO */
	else if (value == DBTYPE_FO) return QObject::tr("Natural person");
	/* unknown */
	else return "";
}


/* ========================================================================= */
/*
 * Convert type of databox to int
 */
int convertDbTypeToInt(QString value)
/* ========================================================================= */
{
	if (value == "OVM") return DBTYPE_OVM;
	else if (value == "OVM_NOTAR") return DBTYPE_OVM_NOTAR;
	else if (value == "OVM_EXEKUT") return DBTYPE_OVM_EXEKUT;
	else if (value == "OVM_REQ") return DBTYPE_OVM_REQ;
	else if (value == "PO") return DBTYPE_PO;
	else if (value == "PO_ZAK") return DBTYPE_PO_ZAK;
	else if (value == "PO_REQ") return DBTYPE_PO_REQ;
	else if (value == "PFO") return DBTYPE_PFO;
	else if (value == "PFO_ADVOK") return DBTYPE_PFO_ADVOK;
	else if (value == "PFO_DANPOR") return DBTYPE_PFO_DANPOR;
	else if (value == "PFO_INSSPR") return DBTYPE_PFO_INSSPR;
	else if (value == "FO") return DBTYPE_FO;
	else return DBTYPE_SYSTEM;
}

/* ========================================================================= */
/*
 * Convert event type to string
 */
QString convertEventTypeToString(int value)
/* ========================================================================= */
{
	if (value == EVENT_ACCEPTED_BY_RECIPIENT) return "EV4: ";
	else if (value == EVENT_ACCEPTED_BY_FICTION) return "EV2: ";
	else if (value == EVENT_UNDELIVERABLE) return "EV3: ";
	else if (value == EVENT_COMMERCIAL_ACCEPTED) return "EV1: ";
	else if (value == EVENT_ENTERED_SYSTEM) return "EV0: ";
	else if (value == EVENT_DELIVERED) return "EV5: ";
	else if (value == EVENT_PRIMARY_LOGIN) return "EV11: ";
	else if (value == EVENT_ENTRUSTED_LOGIN) return "EV12: ";
	else if (value == EVENT_SYSCERT_LOGIN) return "EV13: ";
	else if (value == EVENT_UKNOWN) return "";
	else return "";
}


/* ========================================================================= */
/*
 * Convert event type to string
 */
QString getdbStateText(int value)
/* ========================================================================= */
{
	switch (value) {
	case DBSTATE_ACCESSIBLE:
		/* Datová schránka je přístupná, lze do ní dodávat zprávy, na Portále lze vyhledat. */
		return QObject::tr(
		    "The data box is accessible. It is possible to send messages into it. It can be looked up on the Portal.");
		break;
	case DBSTATE_TEMP_UNACCESSIBLE:
		/* Datová schránka je dočasně znepřístupněna (na vlastní žádost), může být později opět zpřístupněna. */
		return QObject::tr(
		    "The data box is temporarily inaccessible (at own request). It may be made accessible again at some point in the future.");
		break;
	case DBSTATE_NOT_YET_ACCESSIBLE:
		/* Datová schránka je dosud neaktivní. Vlastník schránky se musí poprvé přihlásit do webového rozhraní, aby došlo k aktivaci schránky. */
		return QObject::tr(
		    "The data box is so far inactive. The owner of the box has to log into the web interface at first in order to activate the box.");
		break;
	case DBSTATE_PERM_UNACCESSIBLE:
		/* Datová schránka je trvale znepřístupněna, čeká na smazání (může být opět zpřístupněna). */
		return QObject::tr(
		    "The data box is permanently inaccessible. It is waiting to be deleted (but it may be made accessible again).");
		break;
	case DBSTATE_REMOVED:
		/* Datová schránka je smazána (přesto existuje v ISDS). */
		return QObject::tr(
		    "The data box has been deleted (none the less it exists in ISDS).");
		break;
	default:
		return QObject::tr("An error occurred while checking the status.");
		break;
	}
}


/* ========================================================================= */
/*
 * Converts base64 encoded string into plain text.
 */
QString fromBase64(const QString &base64)
/* ========================================================================= */
{
	return QString::fromUtf8(QByteArray::fromBase64(base64.toUtf8()));
}


/* ========================================================================= */
/*!
 * @brief Converts string into base64.
 */
QString toBase64(const QString &plain)
/* ========================================================================= */
{
	return QString::fromUtf8(plain.toUtf8().toBase64());
}

/* ========================================================================= */
int base64RealSize(const QByteArray &b64)
/* ========================================================================= */
{
	int b64size = b64.size();
	int cnt = 0;
	if (b64size >= 3) {
		for (int i = 1; i <= 3; ++i) {
			if ('=' == b64[b64size - i]) {
				++cnt;
			}
		}
	}
	return b64size * 3 / 4 - cnt;
}
