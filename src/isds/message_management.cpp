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
#include <QPair>
#include <utility> /* std::move */

#include "src/isds/internal_conversion.h"
#include "src/isds/message_management.h"

Isds::Hash::Hash(const Hash &other)
    : m_alg(other.m_alg),
    m_hash(other.m_hash)
{
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::Hash::Hash(Hash &&other) Q_DECL_NOEXCEPT
    : m_alg(std::move(other.m_alg)),
    m_hash(std::move(other.m_hash))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::Hash &Isds::Hash::operator=(const Hash &other) Q_DECL_NOTHROW
{
	m_alg = other.m_alg;
	m_hash = other.m_hash;
	return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::Hash &Isds::Hash::operator=(Hash &&other) Q_DECL_NOTHROW
{
	std::swap(m_alg, other.m_alg);
	std::swap(m_hash, other.m_hash);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::Event::Event(const Event &other)
    : m_time(other.m_time),
    m_type(other.m_type),
    m_descr(other.m_descr)
{
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::Event::Event(Event &&other) Q_DECL_NOEXCEPT
    : m_time(std::move(other.m_time)),
    m_type(std::move(other.m_type)),
    m_descr(std::move(other.m_descr))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

/*!
 * @brief Converts description to event.
 *
 * #note Libisds does not know all events listed in the documentation.
 */
static
enum Isds::Type::Event descr2event(const QString &d)
{
	typedef QPair<QString, Isds::Type::Event> EventPair;

	static const QList<EventPair> pairs({
	    {"EV0:", Isds::Type::EV_ENTERED},
	    {"EV5:", Isds::Type::EV_DELIVERED},
	    {"EV1:", Isds::Type::EV_ACCEPTED_LOGIN},
	    {"EV11:", Isds::Type::EV_PRIMARY_LOGIN},
	    {"EV12:", Isds::Type::EV_ENTRUSTED_LOGIN},
	    {"EV13:", Isds::Type::EV_SYSCERT_LOGIN},
	    {"EV2:", Isds::Type::EV_ACCEPTED_FICTION},
	    {"EV3:", Isds::Type::EV_UNDELIVERABLE},
	    {"EV4:", Isds::Type::EV_ACCEPTED_BY_RECIPIENT},
	    {"EV8:", Isds::Type::EV_UNDELIVERED_AV_CHECK}
	});

	foreach (const EventPair &pair, pairs) {
		if (d.startsWith(pair.first)) {
			return pair.second;
		}
	}

	return Isds::Type::EV_UNKNOWN;
}

void Isds::Event::setDescr(const QString &d)
{
	m_descr = d;
	m_type = descr2event(m_descr);
}

Isds::Event &Isds::Event::operator=(const Event &other) Q_DECL_NOTHROW
{
	m_time = other.m_time;
	m_type = other.m_type;
	m_descr = other.m_descr;
	return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::Event &Isds::Event::operator=(Event &&other) Q_DECL_NOTHROW
{
	std::swap(m_time, other.m_time);
	std::swap(m_type, other.m_type);
	std::swap(m_descr, other.m_descr);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */

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

//Isds::Envelope::Envelope(const Envelope &other);

#ifdef Q_COMPILER_RVALUE_REFS
Isds::Envelope::Envelope(Envelope &&other) Q_DECL_NOEXCEPT
    : m_dataPtr(std::move(other.m_dataPtr))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

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
enum Isds::Type::DbType long2DbType(const long int *bt)
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
void dbType2long(long int **btPtr, enum Isds::Type::DbType bt)
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

/*!
 * @brief Set event according to the libisds event structure.
 */
static
void setEventContent(Isds::Event &tgt, const struct isds_event *src)
{
	if (Q_UNLIKELY(src == NULL)) {
		Q_ASSERT(0);
		return;
	}

	tgt.setTime(Isds::dateTimeFromStructTimeval(src->time));
	//tgt.setType();
	tgt.setDescr(Isds::fromCStr(src->description));
}

QList<Isds::Event> Isds::Envelope::dmEvents(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY((e == NULL) || (e->events == NULL))) {
		return QList<Event>();
	}

	QList<Event> eventList;

	const struct isds_list *item = e->events;
	while (item != NULL) {
		const struct isds_event *ev = (struct isds_event *)item->data;

		if (ev != NULL) {
			Event event;
			setEventContent(event, ev);
			eventList.append(event);
		}

		item = item->next;
	}

	return eventList;
}

/*!
 * @brief Converts event type.
 */
static
void event2libisdsEvent(isds_event_type **tgt, enum Isds::Type::Event src)
{
	if (Q_UNLIKELY(tgt == NULL)) {
		Q_ASSERT(0);
		return;
	}
	if (src == Isds::Type::EV_UNKNOWN) {
		if (*tgt != NULL) {
			std::free(*tgt); *tgt = NULL;
		}
		return;
	}
	if (*tgt == NULL) {
		*tgt = (isds_event_type *)std::malloc(sizeof(**tgt));
		if (Q_UNLIKELY(*tgt == NULL)) {
			Q_ASSERT(0);
			return;
		}
	}
	switch (src) {
	/* case Isds::Type::EV_UNKNOWN: Same as deafult. */
	case Isds::Type::EV_ENTERED: **tgt = EVENT_ENTERED_SYSTEM; break;
	case Isds::Type::EV_DELIVERED: **tgt = EVENT_DELIVERED; break;
	case Isds::Type::EV_ACCEPTED_LOGIN: **tgt = EVENT_ACCEPTED_BY_RECIPIENT; break;
	case Isds::Type::EV_PRIMARY_LOGIN: **tgt = EVENT_PRIMARY_LOGIN; break;
	case Isds::Type::EV_ENTRUSTED_LOGIN: **tgt = EVENT_ENTRUSTED_LOGIN; break;
	case Isds::Type::EV_SYSCERT_LOGIN: **tgt = EVENT_SYSCERT_LOGIN; break;
	case Isds::Type::EV_ACCEPTED_FICTION: **tgt = EVENT_ACCEPTED_BY_FICTION; break;
	case Isds::Type::EV_UNDELIVERABLE: **tgt = EVENT_UNDELIVERABLE; break;
	case Isds::Type::EV_ACCEPTED_BY_RECIPIENT: **tgt = EVENT_COMMERCIAL_ACCEPTED; break;
	/* case Isds::Type::EV_UNDELIVERED_AV_CHECK: Unknown to libisds. */
	default:
		**tgt = EVENT_UKNOWN;
		break;
	}
}

/*!
 * @brief Set libisds event structure according to the event.
 */
static
void setLibisdsEventContent(struct isds_event *tgt, const Isds::Event &src)
{
	if (Q_UNLIKELY(tgt == NULL)) {
		Q_ASSERT(0);
		return;
	}

	Isds::toCDateTimeCopy(&tgt->time, src.time());
	event2libisdsEvent(&tgt->type, src.type());
	Isds::toCStrCopy(&tgt->description, src.descr());
}

void Isds::Envelope::setDmEvents(const QList<Event> &el)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	if (el.isEmpty()) {
		if (e->events != NULL) {
			isds_list_free(&e->events);
		}
		return;
	}

	struct isds_list *lastItem = NULL;

	foreach (const Event &ev, el) {
		struct isds_list *item =
		    (struct isds_list *)std::malloc(sizeof(*item));
		if (Q_UNLIKELY(item == NULL)) {
			Q_ASSERT(0);
			goto fail;
		}
		std::memset(item, 0, sizeof(*item));

		struct isds_event *iev =
		    (struct isds_event *)std::malloc(sizeof(*iev));
		if (Q_UNLIKELY(iev == NULL)) {
			Q_ASSERT(0);
			std::free(item); item = NULL;
			goto fail;
		}

		setLibisdsEventContent(iev, ev);

		/* Set list item. */
		item->next = NULL;
		item->data = iev;
		item->destructor = (void (*)(void **))isds_event_free;

		/* Append item. */
		if (lastItem == NULL) {
			e->events = item;
		} else {
			lastItem->next = item;
		}
		lastItem = item;
	}

	return;

fail:
	Q_ASSERT(0);
	isds_list_free(&e->events);
	return;
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

QChar Isds::Envelope::dmType(void) const
{
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY((e == NULL) || (e->dmType == NULL))) {
		return QChar();
	}

	return QChar(*e->dmType);
}

void Isds::Envelope::setDmType(QChar t)
{
	intAllocMissingEnvelope(&m_dataPtr);
	struct isds_envelope *e = (struct isds_envelope *)m_dataPtr;
	if (Q_UNLIKELY(e == NULL)) {
		Q_ASSERT(0);
		return;
	}

	/* Libisds stores the value as a string. */
	Isds::toCStrCopy(&e->dmType, t.isNull() ? QString() : QString(t));
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

enum Isds::Type::DmType Isds::Envelope::char2DmType(QChar c)
{
	if (c.isNull()) {
		return Type::MT_UNKNOWN;
	}

	switch (c.unicode()) {
	case Isds::Type::MT_I:
	case Isds::Type::MT_K:
	case Isds::Type::MT_O:
	case Isds::Type::MT_V:
	case Isds::Type::MT_A:
	case Isds::Type::MT_B:
	case Isds::Type::MT_C:
	case Isds::Type::MT_D:
	case Isds::Type::MT_E:
	case Isds::Type::MT_G:
	case Isds::Type::MT_X:
	case Isds::Type::MT_Y:
	case Isds::Type::MT_Z:
		return (Isds::Type::DmType)c.unicode();
		break;
	default:
		return Type::MT_UNKNOWN;
		break;
	}
}

QChar Isds::Envelope::dmType2Char(enum Type::DmType t)
{
	if (t != Type::MT_UNKNOWN) {
		return t;
	} else {
		return QChar();
	}
}

//Envelope &operator=(const Envelope &other) Q_DECL_NOTHROW;

#ifdef Q_COMPILER_RVALUE_REFS
Isds::Envelope &Isds::Envelope::operator=(Envelope &&other) Q_DECL_NOTHROW
{
	std::swap(m_dataPtr, other.m_dataPtr);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */

/*!
 * @brief PIMPL Document class.
 */
class Isds::DocumentPrivate {
	//Q_DISABLE_COPY(DocumentPrivate)
public:
	DocumentPrivate(void)
	    : m_xml(false), m_binaryContent(), m_mimeType(),
	    m_metaType(Type::FMT_UNKNOWN), m_fileGuid(), m_upFileGuid(),
	    m_fileDescr(), m_format()
	{ }

	DocumentPrivate &operator=(const DocumentPrivate &other) Q_DECL_NOTHROW
	{
		m_xml = other.m_xml;
		m_binaryContent = other.m_binaryContent;
		m_mimeType = other.m_mimeType;
		m_metaType = other.m_metaType;
		m_fileGuid = other.m_fileGuid;
		m_upFileGuid = other.m_upFileGuid;
		m_fileDescr = other.m_fileDescr;
		m_format = other.m_format;

		return *this;
	}

	bool m_xml; /*!< Inspired by libisds. Direct XML handling is not supported yet! */

	QByteArray m_binaryContent;
	// m_xmlContent;

	QString m_mimeType; /* See pril_2/WS_ISDS_Manipulace_s_datovymi_zpravami.pdf appendix 3. */
	enum Type::FileMetaType m_metaType;
	QString m_fileGuid; /* Optional message-local document identifier. */
	QString m_upFileGuid; /* Optional reference to upper document. */
	QString m_fileDescr; /* Mandatory document name. */
	QString m_format; /* Optional. Can hold a form name for loading XML data from the dmXMLContent element. */
};

Isds::Document::Document(void)
    : d_ptr(Q_NULLPTR)
{
}

Isds::Document::Document(const Document &other)
    : d_ptr((other.d_func() != Q_NULLPTR) ? (new (std::nothrow) DocumentPrivate) : Q_NULLPTR)
{
	Q_D(Document);
	if (d == Q_NULLPTR) {
		return;
	}

	*d = *other.d_func();
}

Isds::Document::Document(Document &&other) Q_DECL_NOEXCEPT
    : d_ptr(other.d_ptr.take()) //d_ptr(std::move(other.d_ptr))
{
}

Isds::Document::~Document(void)
{
}

/*!
 * @brief Ensures private document presence.
 *
 * @note Returns if private document could not be allocated.
 */
#define ensureDocumentPrivate(_x_) \
	do { \
		if (Q_UNLIKELY(d_ptr == Q_NULLPTR)) { \
			DocumentPrivate *p = new (std::nothrow) DocumentPrivate; \
			if (Q_UNLIKELY(p == Q_NULLPTR)) { \
				Q_ASSERT(0); \
				return _x_; \
			} \
			d_ptr.reset(p); \
		} \
	} while (0)

Isds::Document &Isds::Document::operator=(const Document &other) Q_DECL_NOTHROW
{
	if (other.d_func() == Q_NULLPTR) {
		d_ptr.reset(Q_NULLPTR);
		return *this;
	}
	ensureDocumentPrivate(*this);
	Q_D(Document);

	*d = *other.d_func();

	return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::Document &Isds::Document::operator=(Document &&other) Q_DECL_NOTHROW
{
	swap(*this, other);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */

bool Isds::Document::isNull(void) const
{
	Q_D(const Document);
	return d == Q_NULLPTR;
}

bool Isds::Document::isXml(void) const
{
	Q_D(const Document);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return false;
	}
	return d->m_xml;
}

QByteArray Isds::Document::binaryContent(void) const
{
	Q_D(const Document);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return QByteArray();
	}
	return d->m_binaryContent;
}

void Isds::Document::setBinaryContent(const QByteArray &bc)
{
	ensureDocumentPrivate();
	Q_D(Document);
	/* Should also delete XML content if it is present. */
	d->m_binaryContent = bc;
	d->m_xml = false;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Document::setBinaryContent(QByteArray &&bc)
{
	ensureDocumentPrivate();
	Q_D(Document);
	/* Should also delete XML content if it is present. */
	d->m_binaryContent = bc;
	d->m_xml = false;
}
#endif /* Q_COMPILER_RVALUE_REFS */

QString Isds::Document::mimeType(void) const
{
	Q_D(const Document);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return QString();
	}
	return d->m_mimeType;
}

void Isds::Document::setMimeType(const QString &mt)
{
	ensureDocumentPrivate();
	Q_D(Document);
	d->m_mimeType = mt;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Document::setMimeType(QString &&mt)
{
	ensureDocumentPrivate();
	Q_D(Document);
	d->m_mimeType = mt;
}
#endif /* Q_COMPILER_RVALUE_REFS */

enum Isds::Type::FileMetaType Isds::Document::fileMetaType(void) const
{
	Q_D(const Document);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return Isds::Type::FMT_UNKNOWN;
	}
	return d->m_metaType;
}

void Isds::Document::setFileMetaType(enum Type::FileMetaType mt)
{
	ensureDocumentPrivate();
	Q_D(Document);
	d->m_metaType = mt;
}

QString Isds::Document::fileGuid(void) const
{
	Q_D(const Document);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return QString();
	}
	return d->m_fileGuid;
}

void Isds::Document::setFileGuid(const QString &g)
{
	ensureDocumentPrivate();
	Q_D(Document);
	d->m_fileGuid = g;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Document::setFileGuid(QString &&g)
{
	ensureDocumentPrivate();
	Q_D(Document);
	d->m_fileGuid = g;
}
#endif /* Q_COMPILER_RVALUE_REFS */

QString Isds::Document::upFileGuid(void) const
{
	Q_D(const Document);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return QString();
	}
	return d->m_upFileGuid;
}

void Isds::Document::setUpFileGuid(const QString &ug)
{
	ensureDocumentPrivate();
	Q_D(Document);
	d->m_upFileGuid = ug;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Document::setUpFileGuid(QString &&ug)
{
	ensureDocumentPrivate();
	Q_D(Document);
	d->m_upFileGuid = ug;
}
#endif /* Q_COMPILER_RVALUE_REFS */

QString Isds::Document::fileDescr(void) const
{
	Q_D(const Document);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return QString();
	}
	return d->m_fileDescr;
}

void Isds::Document::setFileDescr(const QString &fd)
{
	ensureDocumentPrivate();
	Q_D(Document);
	d->m_fileDescr = fd;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Document::setFileDescr(QString &&fd)
{
	ensureDocumentPrivate();
	Q_D(Document);
	d->m_fileDescr = fd;
}
#endif /* Q_COMPILER_RVALUE_REFS */

QString Isds::Document::format(void) const
{
	Q_D(const Document);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return QString();
	}
	return d->m_format;
}

void Isds::Document::setFormat(const QString &f)
{
	ensureDocumentPrivate();
	Q_D(Document);
	d->m_format = f;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Document::setFormat(QString &&f)
{
	ensureDocumentPrivate();
	Q_D(Document);
	d->m_format = f;
}
#endif /* Q_COMPILER_RVALUE_REFS */

void Isds::swap(Isds::Document &first, Isds::Document &second) Q_DECL_NOTHROW
{
	using std::swap;
	swap(first.d_ptr, second.d_ptr);
}

/*!
 * @brief Converts file meta type.
 */
static
enum Isds::Type::FileMetaType libisdsFileMetaType2FileMetaType(
    isds_FileMetaType ifmt)
{
	switch (ifmt) {
	case FILEMETATYPE_MAIN: return Isds::Type::FMT_MAIN; break;
	case FILEMETATYPE_ENCLOSURE: return Isds::Type::FMT_ENCLOSURE; break;
	case FILEMETATYPE_SIGNATURE: return Isds::Type::FMT_SIGNATURE; break;
	case FILEMETATYPE_META: return Isds::Type::FMT_META; break;
	default:
		return Isds::Type::FMT_UNKNOWN;
		break;
	}
}

Isds::Document Isds::libisds2document(const struct isds_document *id)
{
	if (Q_UNLIKELY(id == NULL)) {
		Q_ASSERT(0);
		return Document();
	}

	Document doc;

	/* Does not support XML documents. */
	if (id->is_xml) {
		return doc;
	}

	doc.setBinaryContent(QByteArray((const char *)id->data, id->data_length));
	doc.setMimeType(fromCStr(id->dmMimeType));
	doc.setFileMetaType(libisdsFileMetaType2FileMetaType(id->dmFileMetaType));
	doc.setFileGuid(fromCStr(id->dmFileGuid));
	doc.setUpFileGuid(fromCStr(id->dmUpFileGuid));
	doc.setFileDescr(fromCStr(id->dmFileDescr));
	doc.setFormat(fromCStr(id->dmFormat));

	return doc;
}

/*!
 * @brief Converts file meta type.
 */
static
isds_FileMetaType fileMetaType2libisdsFileMetaType(
    enum Isds::Type::FileMetaType fmt)
{
	switch (fmt) {
	case Isds::Type::FMT_MAIN: return FILEMETATYPE_MAIN; break;
	case Isds::Type::FMT_ENCLOSURE: return FILEMETATYPE_ENCLOSURE; break;
	case Isds::Type::FMT_SIGNATURE: return FILEMETATYPE_SIGNATURE; break;
	case Isds::Type::FMT_META: return FILEMETATYPE_META; break;
	default:
		Q_ASSERT(0);
		return FILEMETATYPE_MAIN; /* FIXME ? */
		break;
	}
}

struct isds_document *Isds::document2libisds(const Isds::Document &doc)
{
	if (Q_UNLIKELY(doc.isNull())) {
		return NULL;
	}

	struct isds_document *idoc =
	    (struct isds_document *)std::malloc(sizeof(*idoc));
	if (Q_UNLIKELY(idoc == NULL)) {
		Q_ASSERT(0);
		return NULL;
	}
	std::memset(idoc, 0, sizeof(*idoc));

	idoc->is_xml = doc.isXml();
	if (Q_UNLIKELY(idoc->is_xml)) {
		/* Does not support XML documents. */
		goto fail;
	}
	//idoc->xml_node_list = NULL;
	{
		const QByteArray &data(doc.binaryContent());
		if (data.size() > 0) {
			idoc->data_length = data.size();
			idoc->data = std::malloc(idoc->data_length);
			if (Q_UNLIKELY(idoc->data == NULL)) {
				Q_ASSERT(0);
				goto fail;
			}
			std::memcpy(idoc->data, data.constData(), idoc->data_length);
		}
	}
	if (Q_UNLIKELY(!toCStrCopy(&idoc->dmMimeType, doc.mimeType()))) {
		goto fail;
	}
	idoc->dmFileMetaType = fileMetaType2libisdsFileMetaType(doc.fileMetaType());
	if (Q_UNLIKELY(!toCStrCopy(&idoc->dmFileGuid, doc.fileGuid()))) {
		goto fail;
	}
	if (Q_UNLIKELY(!toCStrCopy(&idoc->dmUpFileGuid, doc.upFileGuid()))) {
		goto fail;
	}
	if (Q_UNLIKELY(!toCStrCopy(&idoc->dmFileDescr, doc.fileDescr()))) {
		goto fail;
	}
	if (Q_UNLIKELY(!toCStrCopy(&idoc->dmFormat, doc.format()))) {
		goto fail;
	}

	return idoc;

fail:
	isds_document_free(&idoc);
	return NULL;
}

Isds::Message::Message(void)
    : m_dataPtr(NULL)
{
}

Isds::Message::~Message(void)
{
	struct isds_message *m = (struct isds_message *)m_dataPtr;
	if (Q_UNLIKELY(m == NULL)) {
		return;
	}
	isds_message_free(&m);
}
