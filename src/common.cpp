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

#include <QMimeDatabase>
#include <QMimeType>

#include "common.h"
#include "src/io/isds_sessions.h"
#include "src/log/log.h"

const QString dateTimeDisplayFormat("dd.MM.yyyy HH:mm:ss");
const QString dateDisplayFormat("dd.MM.yyyy");

void addAttachmentToEmailMessage(QString &message, const QString &attachName,
    const QByteArray &base64, const QString &boundary)
{
	const QString newLine("\n"); /* "\r\n" ? */

	QMimeDatabase mimeDb;

	QMimeType mimeType(
	    mimeDb.mimeTypeForData(QByteArray::fromBase64(base64)));

	message += newLine;
	message += "--" + boundary + newLine;
	message += "Content-Type: " + mimeType.name() + "; charset=UTF-8;" + newLine +
	    " name=\"" + attachName +  "\"" + newLine;
	message += "Content-Transfer-Encoding: base64" + newLine;
	message += "Content-Disposition: attachment;" + newLine +
	    " filename=\"" + attachName + "\"" + newLine;

	for (int i = 0; i < base64.size(); ++i) {
		if ((i % 60) == 0) {
			message += newLine;
		}
		message += base64.at(i);
	}
	message += newLine;
}

const QString authorTypeToText(const QString &authorType)
{
	if (QLatin1String("PRIMARY_USER") == authorType) {
		/* Opravnena osoba nebo likvidator. */
		return QObject::tr("Primary user");
	} else if (QLatin1String("ENTRUSTED_USER") == authorType) {
		/* Poverena osoba. */
		return QObject::tr("Entrusted user");
	} else if (QLatin1String("ADMINISTRATOR") == authorType) {
		/* Systemove DZ. */
		return QObject::tr("Administrator");
	} else if (QLatin1String("OFFICIAL") == authorType) {
		/* Systemove DZ. */
		return QObject::tr("Official");
	} else if (QLatin1String("VIRTUAL") == authorType) {
		/* Spisovka. */
		return QObject::tr("Virtual");
	} else if (QLatin1String("OFFICIAL_CERT") == authorType) {
		return QObject::tr("???");
	} else if (QLatin1String("LIQUIDATOR") == authorType) {
		return QObject::tr("Liquidator");
	} else if (QLatin1String("RECEIVER") == authorType) {
		return QObject::tr("Receiver");
	} else if (QLatin1String("GUARDIAN") == authorType) {
		return QObject::tr("Guardian");
	} else {
		logWarningNL("Unknown author type '%s'.",
		    authorType.toUtf8().constData());
		return QString();
	}
}

int base64RealSize(const QByteArray &b64)
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

const QString &convertAttachmentType(int value)
{
	static const QString main("main"), encl("encl"), sign("sign"),
	    meta("meta");
	static const QString invalid;

	switch (value) {
	case FILEMETATYPE_MAIN: return main; break;
	case FILEMETATYPE_ENCLOSURE: return encl; break;
	case FILEMETATYPE_SIGNATURE: return sign; break;
	case FILEMETATYPE_META: return sign; break;
	default:
		logWarningNL("Unknown attachment type value '%d'.", value);
		return invalid;
		break;
	}
}

#define STR_OVM_MAIN "OVM_MAIN"
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

int convertBoxTypeToInt(const QString &value)
{
	if (value == QLatin1String(STR_OVM_MAIN)) return DBTYPE_OVM_MAIN;
	else if (value == QLatin1String(STR_SYSTEM)) return DBTYPE_SYSTEM;
	else if (value == QLatin1String(STR_OVM)) return DBTYPE_OVM;
	else if (value == QLatin1String(STR_OVM_NOTAR)) return DBTYPE_OVM_NOTAR;
	else if (value == QLatin1String(STR_OVM_EXEKUT)) return DBTYPE_OVM_EXEKUT;
	else if (value == QLatin1String(STR_OVM_REQ)) return DBTYPE_OVM_REQ;
	else if (value == QLatin1String(STR_OVM_FO)) return DBTYPE_OVM_FO;
	else if (value == QLatin1String(STR_OVM_PFO)) return DBTYPE_OVM_PFO;
	else if (value == QLatin1String(STR_OVM_PO)) return DBTYPE_OVM_PO;
	else if (value == QLatin1String(STR_PO)) return DBTYPE_PO;
	else if (value == QLatin1String(STR_PO_ZAK)) return DBTYPE_PO_ZAK;
	else if (value == QLatin1String(STR_PO_REQ)) return DBTYPE_PO_REQ;
	else if (value == QLatin1String(STR_PFO)) return DBTYPE_PFO;
	else if (value == QLatin1String(STR_PFO_ADVOK)) return DBTYPE_PFO_ADVOK;
	else if (value == QLatin1String(STR_PFO_DANPOR)) return DBTYPE_PFO_DANPOR;
	else if (value == QLatin1String(STR_PFO_INSSPR)) return DBTYPE_PFO_INSSPR;
	else if (value == QLatin1String(STR_PFO_AUDITOR)) return DBTYPE_PFO_AUDITOR;
	else if (value == QLatin1String(STR_FO)) return DBTYPE_FO;
	else {
		logWarningNL("Unknown data box type '%s'.",
		    value.toUtf8().constData());
		return DBTYPE_SYSTEM;
	}
}

const QString &convertBoxTypeToString(int value)
{
	static const QString ovmMain(STR_OVM_MAIN), system(STR_SYSTEM),
	    ovm(STR_OVM), ovmNotar(STR_OVM_NOTAR), ovmExekut(STR_OVM_EXEKUT),
	    ovmReq(STR_OVM_REQ), ovmFo(STR_OVM_FO), ovmPfo(STR_OVM_PFO),
	    ovmPo(STR_OVM_PO), po(STR_PO), poZak(STR_PO_ZAK), poReq(STR_PO_REQ),
	    pfo(STR_PFO), pfoAdvok(STR_PFO_ADVOK), pfoDanpor(STR_PFO_DANPOR),
	    pfoInsspr(STR_PFO_INSSPR), pfoAuditor(STR_PFO_AUDITOR), fo(STR_FO);
	static const QString invalid;

	switch (value) {
	case DBTYPE_OVM_MAIN: return ovmMain; break;
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
		logWarningNL("Unknown data box type value '%d'.", value);
		return invalid;
		break;
	}
}

const QString &convertEventTypeToString(int value)
{
	static const QString ev0("EV0"), ev1("EV1"), ev2("EV2"), ev3("EV3"),
	    ev4("EV4"), ev5("EV5"), ev11("EV11"), ev12("EV12"), ev13("EV13");
	static const QString invalid;

	switch (value) {
	case EVENT_ACCEPTED_BY_RECIPIENT: return ev4; break;
	case EVENT_ACCEPTED_BY_FICTION: return ev2; break;
	case EVENT_UNDELIVERABLE: return ev3; break;
	case EVENT_COMMERCIAL_ACCEPTED: return ev1; break;
	case EVENT_ENTERED_SYSTEM: return ev0; break;
	case EVENT_DELIVERED: return ev5; break;
	case EVENT_PRIMARY_LOGIN: return ev11; break;
	case EVENT_ENTRUSTED_LOGIN: return ev12; break;
	case EVENT_SYSCERT_LOGIN: return ev13; break;
	case EVENT_UKNOWN:
	default:
		logWarningNL("Unknown event type value '%d'.", value);
		return invalid;
		break;
	}
}

#define STR_MD5 "MD5"
#define STR_SHA_1 "SHA-1"
#define STR_SHA_224 "SHA-224"
#define STR_SHA_256 "SHA-256"
#define STR_SHA_384 "SHA-384"
#define STR_SHA_512 "SHA-512"

const QString &convertHashAlgToString(int value)
{
	static const QString md5(STR_MD5), sha1(STR_SHA_1), sha224(STR_SHA_224),
	    sha256(STR_SHA_256), sha384(STR_SHA_384), sha512(STR_SHA_512);
	static const QString invalid;

	switch (value) {
	case HASH_ALGORITHM_MD5: return md5; break;
	case HASH_ALGORITHM_SHA_1: return sha1; break;
	case HASH_ALGORITHM_SHA_224: return sha224; break;
	case HASH_ALGORITHM_SHA_256: return sha256; break;
	case HASH_ALGORITHM_SHA_384: return sha384; break;
	case HASH_ALGORITHM_SHA_512: return sha512; break;
	default:
		logWarningNL("Unknown hash algorithm value '%d'.", value);
		return invalid;
		break;
	}
}

int convertHashAlgToInt(const QString &value)
{
	if (value == QLatin1String(STR_MD5)) return HASH_ALGORITHM_MD5;
	else if (value == QLatin1String(STR_SHA_1)) return HASH_ALGORITHM_SHA_1;
	else if (value == QLatin1String(STR_SHA_224)) return HASH_ALGORITHM_SHA_224;
	else if (value == QLatin1String(STR_SHA_256)) return HASH_ALGORITHM_SHA_256;
	else if (value == QLatin1String(STR_SHA_384)) return HASH_ALGORITHM_SHA_384;
	else if (value == QLatin1String(STR_SHA_512)) return HASH_ALGORITHM_SHA_512;
	else {
		logWarningNL("Unknown hash algorithm '%s'.",
		    value.toUtf8().constData());
		return HASH_ALGORITHM_MD5;
	}
}

int convertIsdsMsgStatusToDbRepr(int value)
{
	switch (value) {
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
		logWarningNL("Unknown message state value '%d'.", value);
		return 0;
		break;
	}
}

QString dbMsgStatusToText(int status)
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
		return QObject::tr("Data message including its attachments signed with time-stamp.");
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
		    "logging-in (assumption of acceptance through fiction in non-OVM "
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

QString convertSenderBoxTypeToString(int value)
{
	switch (value) {
	case DBTYPE_SYSTEM: return QObject::tr("System ISDS"); break;
	case DBTYPE_OVM: return QObject::tr("Public authority"); break;
	case DBTYPE_PO: return QObject::tr("Legal person"); break;
	case DBTYPE_PFO: return QObject::tr("Self-employed person"); break; /* (OSVC) */
	case DBTYPE_FO: return QObject::tr("Natural person"); break;
	default:
		logWarningNL("Unknown sender data box type value '%d'.", value);
		return QString();
		break;
	}
}

const QString &convertSenderTypeToString(int value)
{
	static const QString pu("PRIMARY_USER"), eu("ENTRUSTED_USER"),
	    a("ADMINISTRATOR"), o("OFFICIAL"), v("VIRTUAL"),
	    oc("OFFICIAL_CERT"), l("LIQUIDATOR"), r("RECEIVER"), g("GUARDIAN");
	static const QString invalid;

	switch (value) {
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
		logWarningNL("Unknown sender type value '%d'.", value);
		return invalid;
		break;
	}
}

QString convertUserPrivilsToString(int userPrivils)
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

const QString &convertUserTypeToString(int value)
{
	static const QString pu("PRIMARY_USER"), eu("ENTRUSTED_USER"),
	    a("ADMINISTRATOR"), l("LIQUIDATOR"), ou("OFFICIAL_USER"),
	    ocu("OFFICIAL_CERT_USER"), r("RECEIVER"), g("GUARDIAN");
	static const QString invalid;

	switch (value) {
	case USERTYPE_PRIMARY: return pu; break;
	case USERTYPE_ENTRUSTED: return eu; break;
	case USERTYPE_ADMINISTRATOR: return a; break;
	case USERTYPE_LIQUIDATOR: return l; break;
	case USERTYPE_OFFICIAL: return ou; break;
	case USERTYPE_OFFICIAL_CERT: return ocu; break;
	case USERTYPE_RECEIVER: return r; break;
	case USERTYPE_GUARDIAN: return g; break;
	default:
		logWarningNL("Unknown user type value '%d'.", value);
		return invalid;
		break;
	}
}

void createEmailMessage(QString &message, const QString &subj,
    const QString &boundary)
{
	message.clear();

	const QString newLine("\n"); /* "\r\n" ? */

	/* Rudimentary header. */
	message += "Subject: " + subj + newLine;
	message += "MIME-Version: 1.0" + newLine;
	message += "Content-Type: multipart/mixed;" + newLine +
	    " boundary=\"" + boundary + "\"" + newLine;

	/* Body. */
	message += newLine;
	message += "--" + boundary + newLine;
	message += "Content-Type: text/plain; charset=UTF-8" + newLine;
	message += "Content-Transfer-Encoding: 8bit" + newLine;

	message += newLine;
	message += "-- " + newLine; /* Must contain the space. */
	message += " " + QObject::tr("Created using Datovka") + " " VERSION "." + newLine;
	message += " <URL: " DATOVKA_HOMEPAGE_URL ">" + newLine;
}

QString dmTypeToText(const QString &dmType)
{
	if (dmType.size() != 1) {
		logWarningNL("Unknown multi-character message type value '%s'.",
		    dmType.toUtf8().constData());
		return QString();
	}

	switch (dmType[0].toLatin1()) {
	case 'K':
		return QObject::tr("Postal data message");
		break;
	case 'I':
		return QObject::tr("Initialising postal data message");
		break;
	case 'O':
		return QObject::tr("Reply postal data message");
		break;
	case 'X':
		return QObject::tr(
		    "Initialising postal data message - expired");
		break;
	case 'Y':
		return QObject::tr("Initialising postal data message - used");
		break;
	default:
		logWarningNL("Unknown message type value '%c'.",
		    dmType[0].toLatin1());
		return QString();
		break;
	}
}

void finishEmailMessage(QString &message, const QString &boundary)
{
	const QString newLine("\n"); /* "\r\n" ? */
	message += newLine + "--" + boundary + "--" + newLine;
}

QString fromBase64(const QString &base64)
{
	return QString::fromUtf8(QByteArray::fromBase64(base64.toUtf8()));
}

QString getBoxStateText(int value)
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
	case DBSTATE_TEMP_UNACCESSIBLE_LAW:
		/* Datová schránka je dočasně znepřístupněna (z důvodů vyjmenovaných v zákoně), může být později opět zpřístupněna. */
		return QObject::tr(
		    "The data box is temporarily inaccessible (because of reasons listed in law). It may be made accessible again at some point in the future.");
		break;
	default:
		logWarningNL("Unknown data box status value '%d'.", value);
		return QObject::tr("An error occurred while checking the status.");
		break;
	}
}

QString getWebDatovkaUsername(const QString &userId, const QString &accountId)
{
	return DB_MOJEID_NAME_PREFIX + userId + "-" + accountId;
}

int getWebDatovkaAccountId(const QString &userName)
{
	if (!userName.contains(DB_MOJEID_NAME_PREFIX)) {
		return -1;
	}

	const QString aID = userName.split("-").at(2);
	return aID.toInt();
}

int getWebDatovkaUserId(const QString &userName)
{
	if (!userName.contains(DB_MOJEID_NAME_PREFIX)) {
		return -1;
	}
	const QString uID = userName.split("-").at(1);
	return uID.toInt();
}

QString getWebDatovkaTagDbPrefix(const QString &userName)
{
	if (!userName.contains(DB_MOJEID_NAME_PREFIX)) {
		return QString();
	}

	return DB_MOJEID_NAME_PREFIX + userName.split("-").at(1);
}

bool isWebDatovkaAccount(const QString &userName)
{
	return userName.contains(DB_MOJEID_NAME_PREFIX);
}

bool isValidDatabaseFileName(QString inDbFileName,
    QString &dbUserName, QString &dbYear, bool &dbTestingFlag, QString &errMsg)
{
	QStringList fileNameParts;
	bool ret = false;
	errMsg = "";
	dbUserName = "";
	dbYear = "";

	if (inDbFileName.contains("___")) {
		// get username from filename
		fileNameParts = inDbFileName.split("_");
		if (fileNameParts.isEmpty() || fileNameParts.count() <= 1) {
			errMsg = QObject::tr("File '%1' does not contain a valid "
			    "database filename.").arg(inDbFileName);
			return ret;
		}
		if (fileNameParts[0].isEmpty() ||
		    fileNameParts[0].length() != 6) {
			errMsg = QObject::tr("File '%1' does not contain a valid "
			    "username in the database filename.").arg(inDbFileName);
			return ret;
		}
		dbUserName = fileNameParts[0];

		// get year from filename
		if (fileNameParts[1].isEmpty()) {
			dbYear = "";
		} else if (fileNameParts[1] == "inv") {
			dbYear = fileNameParts[1];
		} else if (fileNameParts[1].length() == 4) {
			dbYear = fileNameParts[1];
		} else {
			errMsg = QObject::tr("File '%1' does not contain valid "
			    "year in the database filename.").arg(inDbFileName);
			dbYear = "";
			return ret;
		}

		// get testing flag from filename
		fileNameParts = inDbFileName.split(".");
		if (fileNameParts.isEmpty()) {
			errMsg = QObject::tr("File '%1' does not contain valid "
			    "database filename.").arg(inDbFileName);
			return ret;
		}
		fileNameParts = fileNameParts[0].split("___");
		if (fileNameParts.isEmpty()) {
			errMsg = QObject::tr("File '%1' does not contain "
			    "valid database filename.").arg(inDbFileName);
			return ret;
		}

		if (fileNameParts[1] == "1") {
			dbTestingFlag = true;
		} else if (fileNameParts[1] == "0") {
			dbTestingFlag = false;
		} else {
			errMsg = QObject::tr("File '%1' does not contain a valid "
			    "account type flag or filename has wrong format.").arg(inDbFileName);
			dbTestingFlag = false;
			return ret;
		}
	} else {
		errMsg = QObject::tr("File '%1' does not contain a valid message "
		    "database or filename has wrong format.").arg(inDbFileName);
		return ret;
	}

	return true;
}

QString toBase64(const QString &plain)
{
	return QString::fromUtf8(plain.toUtf8().toBase64());
}
