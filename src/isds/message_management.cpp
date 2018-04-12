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

#if defined(__APPLE__) || defined(__clang__)
#  define __USE_C99_MATH
#  define _Bool bool
#else /* !__APPLE__ */
#  include <cstdbool>
#endif /* __APPLE__ */

#include <cstdlib>
#include <cstring>
#include <isds.h>
#include <utility> /* std::move */

#include "src/isds/internal_conversion.h"
#include "src/isds/message_management.h"

Isds::Envelope::Envelope(void)
    : m_dataPtr(NULL)
{
}

Isds::Envelope::~Envelope(void)
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return;
	}
	isds_envelope_free(&e);
}

qint64 Isds::Envelope::dmId(void) const
{
	bool ok = false;
	qint64 id = dmID().toLongLong(&ok);
	return ok ? id : -1;
}

void Isds::Envelope::setDmId(qint64 id)
{
	setDmID((id >= 0) ? QString::number(id) : QString());
}

QString Isds::Envelope::dmID(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return QString();
	}

	return fromCStr(e->dmID);
}

/*!
 * @brief Allocates libisds envelope structure.
 *
 * @param[in,out] dataPtr Pointer to pointer holding the structure.
 */
static
void intAllocMissingEnvelope(void **dataPtr)
{
	if (Q_UNLIKELY(dataPtr == Q_NULLPTR)) {
		Q_ASSERT(0);
		return;
	}
	if (*dataPtr != NULL) {
		/* Already allocated. */
		return;
	}
	struct isds_envelope *e =
	    (struct isds_envelope *)std::malloc(sizeof(*e));
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}
	std::memset(e, 0, sizeof(*e));
	*dataPtr = e;
}

void Isds::Envelope::setDmID(const QString &id)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&e->dmID, id);
}

QString Isds::Envelope::dbIDSender(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return QString();
	}

	return fromCStr(e->dbIDSender);
}

void Isds::Envelope::setDbIDSender(const QString &sbi)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&e->dbIDSender, sbi);
}
