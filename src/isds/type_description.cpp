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

#include "src/isds/type_description.h"
#include "src/log/log.h"

QString Isds::Description::descrDbType(enum Type::DbType type)
{
	switch (type) {
	case Type::BT_NULL:
		return QString();
		break;
//	case Type::BT_OVM_MAIN: return tr("public authority - main box"); break; /* organ verejne moci - hlavni schranka */
	case Type::BT_SYSTEM: return tr("system box"); break; /* systemova schranka */
	case Type::BT_OVM: return tr("public authority"); break; /* organ verejne moci */
	case Type::BT_OVM_NOTAR: return tr("public authority - notary"); break; /* organ verejne moci - notar */
	case Type::BT_OVM_EXEKUT: return tr("public authority - bailiff"); break; /* organ verejne moci - exekutor */
	case Type::BT_OVM_REQ: return tr("public authority - at request"); break; /* organ verejne moci - na zadost */
	case Type::BT_OVM_FO: return tr("public authority - natural person"); break; /* organ verejne moci - fyzicka osoba */
	case Type::BT_OVM_PFO: return tr("public authority - self-employed person"); break; /* organ verejne moci - podnikajici fyzicka osoba */
	case Type::BT_OVM_PO: return tr("public authority - legal person"); break; /* organ verejne moci - pravnicka osoba */
	case Type::BT_PO: return tr("legal person"); break; /* pravnicka osoba */
	case Type::BT_PO_ZAK: return tr("legal person - founded by an act"); break; /* pravnicka osoba - ze zakona */
	case Type::BT_PO_REQ: return tr("legal person - at request"); break; /* pravnicka osoba - na zadost */
	case Type::BT_PFO: return tr("self-employed person"); break; /* podnikajici fyzicka osoba */
	case Type::BT_PFO_ADVOK: return tr("self-employed person - advocate"); break; /* podnikajici fyzicka osoba - advokat */
	case Type::BT_PFO_DANPOR: return tr("self-employed person - tax advisor"); break; /* podnikajici fyzicka osoba - danovy poradce */
	case Type::BT_PFO_INSSPR: return tr("self-employed person - insolvency administrator"); break; /* podnikajici fyzicka osoba - insolvencni spravce */
	case Type::BT_PFO_AUDITOR: return tr("self-employed person - statutory auditor"); break; /* podnikajici fyzicka osoba - auditor */
	case Type::BT_FO: return tr("natural person"); break; /* fyzicka osoba */
	default:
		Q_ASSERT(0);
		logWarningNL("Unknown sender data box type value '%d'.", (int)type);
		return tr("An error occurred while checking the type.");
		break;
	}
}

QString Isds::Description::descrDbState(enum Type::DbState state)
{
	switch (state) {
	case Type::BS_ERROR:
		return QString();
		break;
	case Type::BS_ACCESSIBLE:
		/* Datová schránka je přístupná, lze do ní dodávat zprávy, na Portále lze vyhledat. */
		return tr(
		    "The data box is accessible. It is possible to send messages into it. It can be looked up on the Portal.");
		break;
	case Type::BS_TEMP_INACCESSIBLE:
		/* Datová schránka je dočasně znepřístupněna (na vlastní žádost), může být později opět zpřístupněna. */
		return tr(
		    "The data box is temporarily inaccessible (at own request). It may be made accessible again at some point in the future.");
		break;
	case Type::BS_NOT_YET_ACCESSIBLE:
		/* Datová schránka je dosud neaktivní. Vlastník schránky se musí poprvé přihlásit do webového rozhraní, aby došlo k aktivaci schránky. */
		return tr(
		    "The data box is so far inactive. The owner of the box has to log into the web interface at first in order to activate the box.");
		break;
	case Type::BS_PERM_INACCESSIBLE:
		/* Datová schránka je trvale znepřístupněna, čeká na smazání (může být opět zpřístupněna). */
		return tr(
		    "The data box is permanently inaccessible. It is waiting to be deleted (but it may be made accessible again).");
		break;
	case Type::BS_REMOVED:
		/* Datová schránka je smazána (přesto existuje v ISDS). */
		return tr(
		    "The data box has been deleted (none the less it exists in ISDS).");
		break;
	case Type::BS_TEMP_UNACCESSIBLE_LAW:
		/* Datová schránka je dočasně znepřístupněna (z důvodů vyjmenovaných v zákoně), může být později opět zpřístupněna. */
		return tr(
		    "The data box is temporarily inaccessible (because of reasons listed in law). It may be made accessible again at some point in the future.");
		break;
	default:
		Q_ASSERT(0);
		logWarningNL("Unknown data box status value '%d'.", (int)state);
		return tr("An error occurred while checking the status.");
		break;
	}
}

QString Isds::Description::descrDmState(enum Type::DmState state)
{
	switch (state) {
	case Type::MS_NULL:
		return QString();
		break;
	case Type::MS_POSTED:
		/* Zprava byla podana (vznikla v ISDS). */
		return tr("Message has been submitted (has been created in ISDS)");
		break;
	case Type::MS_STAMPED:
		/*
		 * Datová zprava vcetne pisemnosti podepsana casovym razitkem.
		 */
		return tr("Data message including its attachments signed with time-stamp.");
		break;
	case Type::MS_INFECTED:
		/*
		 * Zprava neprosla AV kontrolou; nakazena pisemnost je smazana;
		 * konecny stav zpravy pred smazanim.
		 */
		return tr("Message did not pass through AV check; "
		    "infected paper deleted; final status before deletion.");
		break;
	case Type::MS_DELIVERED:
		/* Zprava dodana do ISDS (zapsan cas dodani). */
		return tr("Message handed into ISDS (delivery time recorded).");
		break;
	case Type::MS_ACCEPTED_FICT:
		/*
		 * Uplynulo 10 dnu od dodani verejne zpravy, ktera dosud nebyla
		 * dorucena prihlasenim (predpoklad dorucení fikci u neOVM DS);
		 * u komercni zpravy nemuze tento stav nastat.
		 */
		return tr("10 days have passed since the delivery of "
		    "the public message which has not been accepted by "
		    "logging-in (assumption of acceptance through fiction in non-OVM "
		    "DS); this state cannot occur for commercial messages.");
		break;
	case Type::MS_ACCEPTED:
		/*
		 * Osoba opravnena cist tuto zpravu se prihlasila - dodana
		 * zprava byla dorucena.",
		 */
		return tr("A person authorised to read this message "
		    "has logged in -- delivered message has been accepted.");
		break;
	case Type::MS_READ:
		/* Zprava byla prectena (na portale nebo akci ESS). */
		return tr("Message has been read (on the portal or by ESS action).");
		break;
	case Type::MS_UNDELIVERABLE:
		/*
		 * Zprava byla oznacena jako nedorucitelna, protoze DS adresata
		 * byla zpetne znepristupnena.
		 */
		return tr("Message marked as undeliverable because "
		    "the target DS has been made inaccessible.");
		break;
	case Type::MS_REMOVED:
		/*
		 * Obsah zpravy byl smazan, obalka zpravy vcetne hashu
		 * presunuta do archivu.
		 */
		return tr("Message content deleted, envelope "
		    "including hashes has been moved into archive.");
		break;
	case Type::MS_IN_VAULT:
		/* Zprava je v Datovem trezoru. */
		return tr("Message resides in data vault.");
		break;
	default:
		Q_ASSERT(0);
		logWarningNL("Unknown message state value '%d'.", (int)state);
		return tr("An error occurred while checking the status.");
		break;
	}
}

QString Isds::Description::descrError(enum Type::Error err)
{
	switch (err) {
	case Type::ERR_SUCCESS: return tr("Success"); break;
	case Type::ERR_ERROR: return tr("Unspecified error"); break;
	case Type::ERR_NOTSUP: return tr("Not supported"); break;
	case Type::ERR_INVAL: return tr("Invalid value"); break;
	case Type::ERR_INVALID_CONTEXT: return tr("Invalid context"); break;
	case Type::ERR_NOT_LOGGED_IN: return tr("Not logged in"); break;
	case Type::ERR_CONNECTION_CLOSED: return tr("Connection closed"); break;
	case Type::ERR_TIMED_OUT:return ("Timed out"); break;
	case Type::ERR_NOEXIST: return ("Non existent"); break;
	case Type::ERR_NOMEM: return tr("Out of memory"); break;
	case Type::ERR_NETWORK: return tr("Network problem"); break;
	case Type::ERR_HTTP: return tr("HTTP problem"); break;
	case Type::ERR_SOAP: return tr("SOAP problem"); break;
	case Type::ERR_XML: return tr("XML problem"); break;
	case Type::ERR_ISDS: return tr("ISDS server problem"); break;
	case Type::ERR_ENUM: return tr("Invalid enum value"); break;
	case Type::ERR_DATE: return tr("Invalid date value"); break;
	case Type::ERR_2BIG: return tr("Too big"); break;
	case Type::ERR_2SMALL: return tr("Too small"); break;
	case Type::ERR_NOTUNIQ: return tr("Value not unique"); break;
	case Type::ERR_NOTEQUAL: return tr("Values not equal"); break;
	case Type::ERR_PARTIAL_SUCCESS: return tr("Some suboperations failed"); break;
	case Type::ERR_ABORTED: return tr("Operation aborted"); break;
	case Type::ERR_SECURITY: return tr("Security problem"); break;
	default:
		Q_ASSERT(0);
		return tr("Unknown error");
		break;
	}
}
