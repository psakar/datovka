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

/*!
 * @brief Converts user types.
 */
Isds::Type::HashAlg libisdsHashAlg2HashAlg(isds_hash_algorithm a)
{
	switch (a) {
	case HASH_ALGORITHM_MD5: return Isds::Type::HA_MD5; break;
	case HASH_ALGORITHM_SHA_1: return Isds::Type::HA_SHA_1; break;
	case HASH_ALGORITHM_SHA_224: return Isds::Type::HA_SHA_224; break;
	case HASH_ALGORITHM_SHA_256: return Isds::Type::HA_SHA_256; break;
	case HASH_ALGORITHM_SHA_384: return Isds::Type::HA_SHA_384; break;
	case HASH_ALGORITHM_SHA_512: return Isds::Type::HA_SHA_512; break;
	default:
		Q_ASSERT(0);
		return Isds::Type::HA_UNKNOWN;
		break;
	}
}

/*!
 * @brief Set hash according to the libisds hash structure.
 */
static
void setHashContent(Isds::Hash &tgt, const struct isds_hash *src)
{
	if (Q_UNLIKELY(src == NULL)) {
		Q_ASSERT(0);
		return;
	}

	tgt.setAlgorithm(libisdsHashAlg2HashAlg(src->algorithm));
	if (Q_UNLIKELY(tgt.algorithm() == Isds::Type::HA_UNKNOWN)) {
		tgt.setValue(QByteArray());
		return;
	}
	QByteArray data((const char *)src->value, src->length);
	if (!data.isEmpty()) {
		tgt.setValue(data);
	} else {
		tgt.setAlgorithm(Isds::Type::HA_UNKNOWN);
		tgt.setValue(QByteArray());
	}
}

Isds::Hash Isds::Envelope::dmHash(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY((e == NULL) || (e->hash == NULL))) {
		return Hash();
	}

	Hash hash;
	setHashContent(hash, e->hash);
	return hash;
}

/*!
 * @brief Converts user types.
 */
isds_hash_algorithm hashAlg2libisdsHashAlg(Isds::Type::HashAlg a)
{
	switch (a) {
	case Isds::Type::HA_MD5: return HASH_ALGORITHM_MD5; break;
	case Isds::Type::HA_SHA_1: return HASH_ALGORITHM_SHA_1; break;
	case Isds::Type::HA_SHA_224: return HASH_ALGORITHM_SHA_224; break;
	case Isds::Type::HA_SHA_256: return HASH_ALGORITHM_SHA_256; break;
	case Isds::Type::HA_SHA_384: return HASH_ALGORITHM_SHA_384; break;
	case Isds::Type::HA_SHA_512: return HASH_ALGORITHM_SHA_512; break;
	default:
		Q_ASSERT(0);
		return HASH_ALGORITHM_MD5; /* TODO -- This value is clearly incorrect. */
		break;
	}
}

/*!
 * @brief Set libisds hash structure according to the hash.
 */
static
void setLibisdsHashContent(struct isds_hash *tgt, const Isds::Hash &src)
{
	if (Q_UNLIKELY(tgt == NULL)) {
		Q_ASSERT(0);
		return;
	}

	tgt->algorithm = hashAlg2libisdsHashAlg(src.algorithm());
	if (tgt->value != NULL) {
		std::free(tgt->value); tgt->value = NULL;
	}
	const QByteArray &data(src.value());
	tgt->length = data.size();
	if (tgt->length == 0) {
		return;
	}
	tgt->value = std::malloc(tgt->length);
	if (Q_UNLIKELY(tgt->value == NULL)) {
		Q_ASSERT(0);
		tgt->length = 0;
		return;
	}
	std::memcpy(tgt->value, data.constData(), tgt->length);
}

void Isds::Envelope::setDmHash(const Hash &h)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	if (e->hash == NULL) {
		e->hash = (struct isds_hash *)std::malloc(sizeof(*e->hash));
		if (Q_UNLIKELY(e->hash == NULL)) {
			Q_ASSERT(0);
			return;
		}
		std::memset(e->hash, 0, sizeof(*e->hash));
	}

	setLibisdsHashContent(e->hash, h);
}

QByteArray Isds::Envelope::dmQTimestamp(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY((e == NULL) ||
	        (e->timestamp == NULL) || (e->timestamp_length == 0))) {
		return QByteArray();
	}

	return QByteArray((const char *)e->timestamp, e->timestamp_length);
}

void Isds::Envelope::setDmQTimestamp(const QByteArray &ts)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	if (e->timestamp != NULL) {
		std::free(e->timestamp); e->timestamp = NULL;
	}
	e->timestamp_length = ts.size();
	if (e->timestamp_length == 0) {
		return;
	}
	e->timestamp = std::malloc(e->timestamp_length);
	if (Q_UNLIKELY(e->timestamp == NULL)) {
		Q_ASSERT(0);
		e->timestamp_length = 0;
		return;
	}
	std::memcpy(e->timestamp, ts.constData(), e->timestamp_length);
}

QString Isds::Envelope::dmSenderOrgUnit(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return QString();
	}

	return fromCStr(e->dmSenderOrgUnit);
}

void Isds::Envelope::setDmSenderOrgUnit(const QString &sou)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&e->dmSenderOrgUnit, sou);
}

qint64 Isds::Envelope::dmSenderOrgUnitNum(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return -1;
	}

	return fromLongInt(e->dmSenderOrgUnitNum);
}

void Isds::Envelope::setDmSenderOrgUnitNum(qint64 soun)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toLongInt(&e->dmSenderOrgUnitNum, soun);
}

QString Isds::Envelope::dbIDRecipient(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return QString();
	}

	return fromCStr(e->dbIDRecipient);
}

void Isds::Envelope::setDbIDRecipient(const QString &rbi)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&e->dbIDRecipient, rbi);
}

QString Isds::Envelope::dmRecipientOrgUnit(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return QString();
	}

	return fromCStr(e->dmRecipientOrgUnit);
}

void Isds::Envelope::setDmRecipientOrgUnit(const QString &rou)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&e->dmRecipientOrgUnit, rou);
}

qint64 Isds::Envelope::dmRecipientOrgUnitNum(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return -1;
	}

	return fromLongInt(e->dmRecipientOrgUnitNum);
}

void Isds::Envelope::setDmRecipientOrgUnitNum(qint64 &roun)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toLongInt(&e->dmRecipientOrgUnitNum, roun);
}

QString Isds::Envelope::dmToHands(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return QString();
	}

	return fromCStr(e->dmToHands);
}

void Isds::Envelope::setDmToHands(const QString &th)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&e->dmToHands, th);
}

QString Isds::Envelope::dmAnnotation(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return QString();
	}

	return fromCStr(e->dmAnnotation);
}

void Isds::Envelope::setDmAnnotation(const QString &a)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&e->dmAnnotation, a);
}

QString Isds::Envelope::dmRecipientRefNumber(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return QString();
	}

	return fromCStr(e->dmRecipientRefNumber);
}

void Isds::Envelope::setDmRecipientRefNumber(const QString &rrn)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&e->dmRecipientRefNumber, rrn);
}

QString Isds::Envelope::dmSenderRefNumber(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return QString();
	}

	return fromCStr(e->dmSenderRefNumber);
}

void Isds::Envelope::setDmSenderRefNumber(const QString &srn)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&e->dmSenderRefNumber, srn);
}

QString Isds::Envelope::dmRecipientIdent(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return QString();
	}

	return fromCStr(e->dmRecipientIdent);
}

void Isds::Envelope::setDmRecipientIdent(const QString &ri)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&e->dmRecipientIdent, ri);
}

QString Isds::Envelope::dmSenderIdent(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return QString();
	}

	return fromCStr(e->dmSenderIdent);
}

void Isds::Envelope::setDmSenderIdent(const QString &si)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&e->dmSenderIdent, si);
}

qint64 Isds::Envelope::dmLegalTitleLaw(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return -1;
	}

	return fromLongInt(e->dmLegalTitleLaw);
}

void Isds::Envelope::setDmLegalTitleLaw(qint64 l)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toLongInt(&e->dmLegalTitleLaw, l);
}

qint64 Isds::Envelope::dmLegalTitleYear(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return -1;
	}

	return fromLongInt(e->dmLegalTitleYear);
}

void Isds::Envelope::setDmLegalTitleYear(qint64 y)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toLongInt(&e->dmLegalTitleYear, y);
}

QString Isds::Envelope::dmLegalTitleSect(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return QString();
	}

	return fromCStr(e->dmLegalTitleSect);
}

void Isds::Envelope::setDmLegalTitleSect(const QString &s)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&e->dmLegalTitleSect, s);
}

QString Isds::Envelope::dmLegalTitlePar(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return QString();
	}

	return fromCStr(e->dmLegalTitlePar);
}

void Isds::Envelope::setDmLegalTitlePar(const QString &p)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&e->dmLegalTitlePar, p);
}

QString Isds::Envelope::dmLegalTitlePoint(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return QString();
	}

	return fromCStr(e->dmLegalTitlePoint);
}

void Isds::Envelope::setDmLegalTitlePoint(const QString &p)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toCStrCopy(&e->dmLegalTitlePoint, p);
}

enum Isds::Type::NilBool Isds::Envelope::dmPersonalDelivery(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return Type::BOOL_NULL;
	}

	return fromBool(e->dmPersonalDelivery);
}

void Isds::Envelope::setDmPersonalDelivery(enum Type::NilBool pd)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toBool(&e->dmPersonalDelivery, pd);
}

enum Isds::Type::NilBool Isds::Envelope::dmAllowSubstDelivery(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return Type::BOOL_NULL;
	}

	return fromBool(e->dmAllowSubstDelivery);
}

void Isds::Envelope::setDmAllowSubstDelivery(enum Type::NilBool sd)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toBool(&e->dmAllowSubstDelivery, sd);
}

enum Isds::Type::NilBool Isds::Envelope::dmOVM(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return Type::BOOL_NULL;
	}

	return fromBool(e->dmOVM);
}

void Isds::Envelope::dmOVM(enum Type::NilBool ovm)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toBool(&e->dmOVM, ovm);
}

enum Isds::Type::NilBool Isds::Envelope::dmPublishOwnID(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		return Type::BOOL_NULL;
	}

	return fromBool(e->dmPublishOwnID);
}

void Isds::Envelope::setDmPublishOwnID(enum Type::NilBool poi)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	toBool(&e->dmPublishOwnID, poi);
}
