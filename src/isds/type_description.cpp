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
		return QString();
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
