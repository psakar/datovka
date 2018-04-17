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

QString Isds::Envelope::dmSender(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return QString();
	}

	return fromCStr(e->dmSender);
}

void Isds::Envelope::setDmSender(const QString &sn)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&e->dmSender, sn);
}

QString Isds::Envelope::dmSenderAddress(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return QString();
	}

	return fromCStr(e->dmSenderAddress);
}

void Isds::Envelope::setDmSenderAddress(const QString &sa)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&e->dmSenderAddress, sa);
}

/*!
 * @brief Converts data box types.
 */
static
Isds::Type::DbType long2DbType(const long int *bt)
{
	if (bt == NULL) {
		return Isds::Type::BT_NULL;
	}

	switch (*bt) {
	case Isds::Type::BT_SYSTEM: return Isds::Type::BT_SYSTEM; break;
	case Isds::Type::BT_OVM: return Isds::Type::BT_OVM; break;
	case Isds::Type::BT_OVM_NOTAR: return Isds::Type::BT_OVM_NOTAR; break;
	case Isds::Type::BT_OVM_EXEKUT: return Isds::Type::BT_OVM_EXEKUT; break;
	case Isds::Type::BT_OVM_REQ: return Isds::Type::BT_OVM_REQ; break;
	case Isds::Type::BT_OVM_FO: return Isds::Type::BT_OVM_FO; break;
	case Isds::Type::BT_OVM_PFO: return Isds::Type::BT_OVM_PFO; break;
	case Isds::Type::BT_OVM_PO: return Isds::Type::BT_OVM_PO; break;
	case Isds::Type::BT_PO: return Isds::Type::BT_PO; break;
	case Isds::Type::BT_PO_ZAK: return Isds::Type::BT_PO_ZAK; break;
	case Isds::Type::BT_PO_REQ: return Isds::Type::BT_PO_REQ; break;
	case Isds::Type::BT_PFO: return Isds::Type::BT_PFO; break;
	case Isds::Type::BT_PFO_ADVOK: return Isds::Type::BT_PFO_ADVOK; break;
	case Isds::Type::BT_PFO_DANPOR: return Isds::Type::BT_PFO_DANPOR; break;
	case Isds::Type::BT_PFO_INSSPR: return Isds::Type::BT_PFO_INSSPR; break;
	case Isds::Type::BT_PFO_AUDITOR: return Isds::Type::BT_PFO_AUDITOR; break;
	case Isds::Type::BT_FO: return Isds::Type::BT_FO; break;
	default:
		Q_ASSERT(0);
		return Isds::Type::BT_SYSTEM; /* FIXME */
		break;
	}
}

/*!
 * @brief Converts data box types.
 */
static
void dbType2long(long int **btPtr, Isds::Type::DbType bt)
{
	if (Q_UNLIKELY(btPtr == Q_NULLPTR)) {
		Q_ASSERT(0);
		return;
	}
	if (*btPtr == NULL) {
		*btPtr = (long int *)std::malloc(sizeof(**btPtr));
		if (Q_UNLIKELY(btPtr == NULL)) {
			Q_ASSERT(0);
			return;
		}
	}
	switch (bt) {
	case Isds::Type::BT_NULL:
		Q_ASSERT(0);
		std::free(*btPtr); *btPtr = NULL;
		break;
	default:
		**btPtr = bt;
		break;
	}
}

enum Isds::Type::DbType Isds::Envelope::dmSenderType(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return Isds::Type::BT_NULL;
	}

	return long2DbType(e->dmSenderType);
}

void Isds::Envelope::setDmSenderType(enum Type::DbType st)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	dbType2long(&e->dmSenderType, st);
}

QString Isds::Envelope::dmRecipient(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return QString();
	}

	return fromCStr(e->dmRecipient);
}

void Isds::Envelope::setDmRecipient(const QString &rn)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&e->dmRecipient, rn);
}

QString Isds::Envelope::dmRecipientAddress(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return QString();
	}

	return fromCStr(e->dmRecipientAddress);
}

void Isds::Envelope::setDmRecipientAddress(const QString &ra)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&e->dmRecipientAddress, ra);
}

enum Isds::Type::NilBool Isds::Envelope::dmAmbiguousRecipient(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return Type::BOOL_NULL;
	}

	return fromBool(e->dmAmbiguousRecipient);
}

void Isds::Envelope::setDmAmbiguousRecipient(enum Type::NilBool ar)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toBool(&e->dmAmbiguousRecipient, ar);
}

quint64 Isds::Envelope::dmOrdinal(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY((e == NULL) || (e->dmOrdinal))) {
		return 0;
	}

	return *e->dmOrdinal;
}

void Isds::Envelope::setDmOrdinal(quint64 o)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}
	if (e->dmOrdinal == NULL) {
		e->dmOrdinal = (unsigned long int *)std::malloc(sizeof(*e->dmOrdinal));
		if (e->dmOrdinal == NULL) {
			Q_ASSERT(0);
			return;
		}
	}

	*e->dmOrdinal = o;
}

/*!
 * @brief Converts message status.
 */
static
enum Isds::Type::DmState libisdsMessageStatus2DmState(
    const isds_message_status *ms)
{
	if (ms == NULL) {
		return Isds::Type::MS_NULL;
	}

	switch (*ms) {
	case MESSAGESTATE_SENT: return Isds::Type::MS_POSTED; break;
	case MESSAGESTATE_STAMPED: return Isds::Type::MS_STAMPED; break;
	case MESSAGESTATE_INFECTED: return Isds::Type::MS_INFECTED; break;
	case MESSAGESTATE_DELIVERED: return Isds::Type::MS_DELIVERED; break;
	case MESSAGESTATE_SUBSTITUTED: return Isds::Type::MS_ACCEPTED_FICT; break;
	case MESSAGESTATE_RECEIVED: return Isds::Type::MS_ACCEPTED; break;
	case MESSAGESTATE_READ: return Isds::Type::MS_READ; break;
	case MESSAGESTATE_UNDELIVERABLE: return Isds::Type::MS_UNDELIVERABLE; break;
	case MESSAGESTATE_REMOVED: return Isds::Type::MS_REMOVED; break;
	case MESSAGESTATE_IN_SAFE: return Isds::Type::MS_IN_VAULT; break;
	default:
		return Isds::Type::MS_NULL;
		break;
	}
}

/*!
 * @brief Converts message status.
 */
static
void dmState2libisdsMessageStatus(isds_message_status **tgt,
    enum Isds::Type::DmState src)
{
	if (Q_UNLIKELY(tgt == NULL)) {
		Q_ASSERT(0);
		return;
	}
	if (src == Isds::Type::MS_NULL) {
		if (*tgt != NULL) {
			std::free(*tgt); *tgt = NULL;
		}
		return;
	}
	if (*tgt == NULL) {
		*tgt = (isds_message_status *)std::malloc(sizeof(**tgt));
		if (Q_UNLIKELY(*tgt == NULL)) {
			Q_ASSERT(0);
			return;
		}
	}
	switch (src) {
	/* case Isds::Type::MS_NULL: Same as default. */
	case Isds::Type::MS_POSTED: **tgt = MESSAGESTATE_SENT; break;
	case Isds::Type::MS_STAMPED: **tgt = MESSAGESTATE_STAMPED; break;
	case Isds::Type::MS_INFECTED: **tgt = MESSAGESTATE_INFECTED; break;
	case Isds::Type::MS_DELIVERED: **tgt = MESSAGESTATE_DELIVERED; break;
	case Isds::Type::MS_ACCEPTED_FICT: **tgt = MESSAGESTATE_SUBSTITUTED; break;
	case Isds::Type::MS_ACCEPTED: **tgt = MESSAGESTATE_RECEIVED; break;
	case Isds::Type::MS_READ: **tgt = MESSAGESTATE_READ; break;
	case Isds::Type::MS_UNDELIVERABLE: **tgt = MESSAGESTATE_UNDELIVERABLE; break;
	case Isds::Type::MS_REMOVED: **tgt = MESSAGESTATE_REMOVED; break;
	case Isds::Type::MS_IN_VAULT: **tgt = MESSAGESTATE_IN_SAFE; break;
	default:
		Q_ASSERT(0);
		std::free(*tgt); *tgt = NULL;
		break;
	}
}

enum Isds::Type::DmState Isds::Envelope::dmMessageStatus(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return Type::MS_NULL;
	}

	return libisdsMessageStatus2DmState(e->dmMessageStatus);
}

void Isds::Envelope::setDmMessageStatus(enum Type::DmState s)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	dmState2libisdsMessageStatus(&e->dmMessageStatus, s);
}

qint64 Isds::Envelope::dmAttachmentSize(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return -1;
	}

	return fromLongInt(e->dmAttachmentSize);
}

void Isds::Envelope::setDmAttachmentSize(qint64 as)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toLongInt(&e->dmAttachmentSize, as);
}

QDateTime Isds::Envelope::dmDeliveryTime(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return QDateTime();
	}

	return dateTimeFromStructTimeval(e->dmDeliveryTime);
}

void Isds::Envelope::setDmDeliveryTime(const QDateTime &dt)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCDateTimeCopy(&e->dmDeliveryTime, dt);
}


QDateTime Isds::Envelope::dmAcceptanceTime(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return QDateTime();
	}

	return dateTimeFromStructTimeval(e->dmAcceptanceTime);
}

void Isds::Envelope::setDmAcceptanceTime(const QDateTime &at)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCDateTimeCopy(&e->dmAcceptanceTime, at);
}
