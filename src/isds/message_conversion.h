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

#include <QList>

#include "src/datovka_shared/isds/message_interface.h"

extern "C" {
	struct isds_hash;
	struct isds_event;
	struct isds_envelope;
	struct isds_document;
	struct isds_message;
	struct isds_list;
}

namespace Isds {

	Hash libisds2hash(const struct ::isds_hash *ih, bool *ok = Q_NULLPTR);
	struct ::isds_hash *hash2libisds(const Hash &h, bool *ok = Q_NULLPTR);

	Event libisds2event(const struct ::isds_event *ie, bool *ok = Q_NULLPTR);
	struct ::isds_event *event2libisds(const Event &e, bool *ok = Q_NULLPTR);

	Envelope libisds2envelope(const struct ::isds_envelope *ie,
	    bool *ok = Q_NULLPTR);
	struct ::isds_envelope *envelope2libisds(const Envelope &env,
	    bool *ok = Q_NULLPTR);

	Document libisds2document(const struct ::isds_document *id,
	    bool *ok = Q_NULLPTR);
	struct ::isds_document *document2libisds(const Document &doc,
	    bool *ok = Q_NULLPTR);

	Message libisds2message(const struct ::isds_message *im,
	    bool *ok = Q_NULLPTR);
	struct ::isds_message *message2libisds(const Message &m,
	    bool *ok = Q_NULLPTR);

	QList<Message> libisds2messageList(const struct ::isds_list *iml,
	    bool *ok = Q_NULLPTR);
	struct ::isds_list *messageList2libisds(const QList<Message> &ml,
	    bool *ok = Q_NULLPTR);

}
