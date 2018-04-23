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

/* Null objects - for convenience. */
static const Isds::Hash nullHash;
static const QByteArray nullByteArray;
static const QDateTime nullDateTime;
static const QList<Isds::Event> nullEventList;
static const QString nullString;

/*!
 * @brief PIMPL Hash class.
 */
class Isds::HashPrivate {
	//Q_DISABLE_COPY(HashPrivate)
public:
	HashPrivate(void)
	    : m_alg(Type::HA_UNKNOWN), m_hash()
	{ }

	HashPrivate &operator=(const HashPrivate &other) Q_DECL_NOTHROW
	{
		m_alg = other.m_alg;
		m_hash = other.m_hash;

		return *this;
	}

	enum Type::HashAlg m_alg;
	QByteArray m_hash;
};

Isds::Hash::Hash(void)
    : d_ptr(Q_NULLPTR)
{
}

Isds::Hash::Hash(const Hash &other)
    : d_ptr((other.d_func() != Q_NULLPTR) ? (new (std::nothrow) HashPrivate) : Q_NULLPTR)
{
	Q_D(Hash);
	if (d == Q_NULLPTR) {
		return;
	}

	*d = *other.d_func();
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::Hash::Hash(Hash &&other) Q_DECL_NOEXCEPT
    : d_ptr(other.d_ptr.take()) //d_ptr(std::move(other.d_ptr))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::Hash::~Hash(void)
{
}

/*!
 * @brief Ensures private hash presence.
 *
 * @note Returns if private hash could not be allocated.
 */
#define ensureHashPrivate(_x_) \
	do { \
		if (Q_UNLIKELY(d_ptr == Q_NULLPTR)) { \
			HashPrivate *p = new (std::nothrow) HashPrivate; \
			if (Q_UNLIKELY(p == Q_NULLPTR)) { \
				Q_ASSERT(0); \
				return _x_; \
			} \
			d_ptr.reset(p); \
		} \
	} while (0)

Isds::Hash &Isds::Hash::operator=(const Hash &other) Q_DECL_NOTHROW
{
	if (other.d_func() == Q_NULLPTR) {
		d_ptr.reset(Q_NULLPTR);
		return *this;
	}
	ensureHashPrivate(*this);
	Q_D(Hash);

	*d = *other.d_func();

	return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::Hash &Isds::Hash::operator=(Hash &&other) Q_DECL_NOTHROW
{
	swap(*this, other);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */

bool Isds::Hash::isNull(void) const
{
	Q_D(const Hash);
	return d == Q_NULLPTR;
}

enum Isds::Type::HashAlg Isds::Hash::algorithm(void) const
{
	Q_D(const Hash);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return Type::HA_UNKNOWN;
	}

	return d->m_alg;
}

void Isds::Hash::setAlgorithm(enum Type::HashAlg a)
{
	ensureHashPrivate();
	Q_D(Hash);
	d->m_alg = a;
}

const QByteArray &Isds::Hash::value(void) const
{
	Q_D(const Hash);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullByteArray;
	}

	return d->m_hash;
}

void Isds::Hash::setValue(const QByteArray &v)
{
	ensureHashPrivate();
	Q_D(Hash);
	d->m_hash = v;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Hash::setValue(QByteArray &&v)
{
	ensureHashPrivate();
	Q_D(Hash);
	d->m_hash = v;
}
#endif /* Q_COMPILER_RVALUE_REFS */

void Isds::swap(Hash &first, Hash &second) Q_DECL_NOTHROW
{
	using std::swap;
	swap(first.d_ptr, second.d_ptr);
}

/*!
 * @brief Converts user types.
 */
static
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
bool setHashContent(Isds::Hash &tgt, const struct isds_hash *src)
{
	if (Q_UNLIKELY(src == NULL)) {
		return true;
	}

	tgt.setAlgorithm(libisdsHashAlg2HashAlg(src->algorithm));
	if (Q_UNLIKELY(tgt.algorithm() == Isds::Type::HA_UNKNOWN)) {
		tgt.setValue(QByteArray());
		return false;
	}
	QByteArray data((const char *)src->value, src->length);
	if (!data.isEmpty()) {
		tgt.setValue(data);
	} else {
		tgt.setAlgorithm(Isds::Type::HA_UNKNOWN);
		tgt.setValue(QByteArray());
	}
	return true;
}

Isds::Hash Isds::libisds2hash(const struct isds_hash *ih, bool *ok)
{
	Hash hash;
	bool ret = setHashContent(hash, ih);
	if (ok != Q_NULLPTR) {
		*ok = ret;
	}
	return hash;
}

/*!
 * @brief Converts user types.
 */
static
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
bool setLibisdsHashContent(struct isds_hash *tgt, const Isds::Hash &src)
{
	if (Q_UNLIKELY(tgt == NULL)) {
		Q_ASSERT(0);
		return false;
	}

	tgt->algorithm = hashAlg2libisdsHashAlg(src.algorithm());
	if (tgt->value != NULL) {
		std::free(tgt->value); tgt->value = NULL;
	}
	const QByteArray &data(src.value());
	tgt->length = data.size();
	if (tgt->length == 0) {
		return true;
	}
	tgt->value = std::malloc(tgt->length);
	if (Q_UNLIKELY(tgt->value == NULL)) {
		Q_ASSERT(0);
		tgt->length = 0;
		return false;
	}
	std::memcpy(tgt->value, data.constData(), tgt->length);
	return true;
}

struct isds_hash *Isds::hash2libisds(const Hash &h)
{
	if (Q_UNLIKELY(h.isNull())) {
		return NULL;
	}

	struct isds_hash *ih = (struct isds_hash *)std::malloc(sizeof(*ih));
	if (Q_UNLIKELY(ih == NULL)) {
		Q_ASSERT(0);
		return NULL;
	}
	if (Q_UNLIKELY(!setLibisdsHashContent(ih, h))) {
		isds_hash_free(&ih);
		return NULL;
	}
	return ih;
}

/*!
 * @brief PIMPL Event class.
 */
class Isds::EventPrivate {
	//Q_DISABLE_COPY(EventPrivate)
public:
	EventPrivate(void)
	    : m_time(), m_type(Type::EV_UNKNOWN), m_descr()
	{ }

	EventPrivate &operator=(const EventPrivate &other) Q_DECL_NOTHROW
	{
		m_time = other.m_time;
		m_type = other.m_type;
		m_descr = other.m_descr;

		return *this;
	}

	QDateTime m_time; /* dmEventTime */
	enum Type::Event m_type; /* Inspired by libisds. */
	QString m_descr; /* dmEventDescr */
};

Isds::Event::Event(void)
    : d_ptr(Q_NULLPTR)
{
}

Isds::Event::Event(const Event &other)
    : d_ptr((other.d_func() != Q_NULLPTR) ? (new (std::nothrow) EventPrivate) : Q_NULLPTR)
{
	Q_D(Event);
	if (d == Q_NULLPTR) {
		return;
	}

	*d = *other.d_func();
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::Event::Event(Event &&other) Q_DECL_NOEXCEPT
    : d_ptr(other.d_ptr.take()) //d_ptr(std::move(other.d_ptr))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::Event::~Event(void)
{
}

/*!
 * @brief Ensures private event presence.
 *
 * @note Returns if private event could not be allocated.
 */
#define ensureEventPrivate(_x_) \
	do { \
		if (Q_UNLIKELY(d_ptr == Q_NULLPTR)) { \
			EventPrivate *p = new (std::nothrow) EventPrivate; \
			if (Q_UNLIKELY(p == Q_NULLPTR)) { \
				Q_ASSERT(0); \
				return _x_; \
			} \
			d_ptr.reset(p); \
		} \
	} while (0)

Isds::Event &Isds::Event::operator=(const Event &other) Q_DECL_NOTHROW
{
	if (other.d_func() == Q_NULLPTR) {
		d_ptr.reset(Q_NULLPTR);
		return *this;
	}
	ensureEventPrivate(*this);
	Q_D(Event);

	*d = *other.d_func();

	return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::Event &Isds::Event::operator=(Event &&other) Q_DECL_NOTHROW
{
	swap(*this, other);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */

bool Isds::Event::isNull(void) const
{
	Q_D(const Event);
	return d == Q_NULLPTR;
}

const QDateTime &Isds::Event::time(void) const
{
	Q_D(const Event);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullDateTime;
	}

	return d->m_time;
}

void Isds::Event::setTime(const QDateTime &t)
{
	ensureEventPrivate();
	Q_D(Event);
	d->m_time = t;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Event::setTime(QDateTime &&t)
{
	ensureEventPrivate();
	Q_D(Event);
	d->m_time = t;
}
#endif /* Q_COMPILER_RVALUE_REFS */

enum Isds::Type::Event Isds::Event::type(void) const
{
	Q_D(const Event);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return Type::EV_UNKNOWN;
	}

	return d->m_type;
}

const QString &Isds::Event::descr(void) const
{
	Q_D(const Event);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_descr;
}

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

void Isds::Event::setDescr(const QString &descr)
{
	ensureEventPrivate();
	Q_D(Event);
	d->m_descr = descr;
	d->m_type = descr2event(d->m_descr);
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Event::setDescr(QString &&descr)
{
	ensureEventPrivate();
	Q_D(Event);
	d->m_descr = descr;
	d->m_type = descr2event(d->m_descr);
}
#endif /* Q_COMPILER_RVALUE_REFS */

void Isds::swap(Event &first, Event &second) Q_DECL_NOTHROW
{
	using std::swap;
	swap(first.d_ptr, second.d_ptr);
}

/*!
 * @brief Set event according to the libisds event structure.
 */
static
bool setEventContent(Isds::Event &tgt, const struct isds_event *src)
{
	if (Q_UNLIKELY(src == NULL)) {
		return true;
	}

	tgt.setTime(Isds::dateTimeFromStructTimeval(src->time));
	//tgt.setType();
	tgt.setDescr(Isds::fromCStr(src->description));
	return true;
}

Isds::Event Isds::libisds2event(const struct isds_event *ie, bool *ok)
{
	Event event;
	bool ret = setEventContent(event, ie);
	if (ok != Q_NULLPTR) {
		*ok = ret;
	}
	return event;
}

/*!
 * @brief Converts event type.
 */
static
bool event2libisdsEvent(isds_event_type **tgt, enum Isds::Type::Event src)
{
	if (Q_UNLIKELY(tgt == NULL)) {
		Q_ASSERT(0);
		return false;
	}
	if (src == Isds::Type::EV_UNKNOWN) {
		if (*tgt != NULL) {
			std::free(*tgt); *tgt = NULL;
		}
		return true;
	}
	if (*tgt == NULL) {
		*tgt = (isds_event_type *)std::malloc(sizeof(**tgt));
		if (Q_UNLIKELY(*tgt == NULL)) {
			Q_ASSERT(0);
			return false;
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

	return true;
}

/*!
 * @brief Set libisds event structure according to the event.
 */
static
bool setLibisdsEventContent(struct isds_event *tgt, const Isds::Event &src)
{
	if (Q_UNLIKELY(tgt == NULL)) {
		Q_ASSERT(0);
		return false;
	}

	if (Q_UNLIKELY(!Isds::toCDateTimeCopy(&tgt->time, src.time()))) {
		return false;
	}
	if (Q_UNLIKELY(!event2libisdsEvent(&tgt->type, src.type()))) {
		return false;
	}
	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->description, src.descr()))) {
		return false;
	}

	return true;
}

struct isds_event *Isds::event2libisds(const Event &e)
{
	if (Q_UNLIKELY(e.isNull())) {
		return NULL;
	}

	struct isds_event *ie = (struct isds_event *)std::malloc(sizeof(*ie));
	if (Q_UNLIKELY(ie == NULL)) {
		Q_ASSERT(0);
		return NULL;
	}
	if (Q_UNLIKELY(!setLibisdsEventContent(ie, e))) {
		isds_event_free(&ie);
		return NULL;
	}
	return ie;
}

class Isds::EnvelopePrivate {
	//Q_DISABLE_COPY(EnvelopePrivate)
public:
	EnvelopePrivate(void)
	    : m_dmID(), m_dbIDSender(), m_dmSender(), m_dmSenderAddress(),
	    m_dmSenderType(Type::BT_NULL), m_dmRecipient(),
	    m_dmRecipientAddress(), m_dmAmbiguousRecipient(Type::BOOL_NULL),
	    m_dmOrdinal(-1), m_dmMessageStatus(Type::MS_NULL),
	    m_dmAttachmentSize(-1), m_dmDeliveryTime(), m_dmAcceptanceTime(),
	    m_dmHash(), m_dmQTimestamp(), m_dmEvents(), m_dmSenderOrgUnit(),
	    m_dmSenderOrgUnitNum(-1), m_dbIDRecipient(), m_dmRecipientOrgUnit(),
	    m_dmRecipientOrgUnitNum(-1), m_dmToHands(), m_dmAnnotation(),
	    m_dmRecipientRefNumber(), m_dmSenderRefNumber(),
	    m_dmRecipientIdent(), m_dmSenderIdent(), m_dmLegalTitleLaw(-1),
	    m_dmLegalTitleYear(-1), m_dmLegalTitleSect(), m_dmLegalTitlePar(),
	    m_dmLegalTitlePoint(), m_dmPersonalDelivery(Type::BOOL_NULL),
	    m_dmAllowSubstDelivery(Type::BOOL_NULL), m_dmType(),
	    m_dmOVM(Type::BOOL_NULL), m_dmPublishOwnID(Type::BOOL_NULL)
	{ }

	EnvelopePrivate &operator=(const EnvelopePrivate &other) Q_DECL_NOTHROW
	{
		m_dmID = other.m_dmID;
		m_dbIDSender = other.m_dbIDSender;
		m_dmSender = other.m_dmSender;
		m_dmSenderAddress = other.m_dmSenderAddress;
		m_dmSenderType = other.m_dmSenderType;
		m_dmRecipient = other.m_dmRecipient;
		m_dmRecipientAddress = other.m_dmRecipientAddress;
		m_dmAmbiguousRecipient = other.m_dmAmbiguousRecipient;

		m_dmOrdinal = other.m_dmOrdinal;
		m_dmMessageStatus = other.m_dmMessageStatus;
		m_dmAttachmentSize = other.m_dmAttachmentSize;
		m_dmDeliveryTime = other.m_dmDeliveryTime;
		m_dmAcceptanceTime = other.m_dmAcceptanceTime;
		m_dmHash = other.m_dmHash;
		m_dmQTimestamp = other.m_dmQTimestamp;
		m_dmEvents = other.m_dmEvents;

		m_dmSenderOrgUnit = other.m_dmSenderOrgUnit;
		m_dmSenderOrgUnitNum = other.m_dmSenderOrgUnitNum;
		m_dbIDRecipient = other.m_dbIDRecipient;
		m_dmRecipientOrgUnit = other.m_dmRecipientOrgUnit;
		m_dmRecipientOrgUnitNum = other.m_dmRecipientOrgUnitNum;
		m_dmToHands = other.m_dmToHands;
		m_dmAnnotation = other.m_dmAnnotation;
		m_dmRecipientRefNumber = other.m_dmRecipientRefNumber;
		m_dmSenderRefNumber = other.m_dmSenderRefNumber;
		m_dmRecipientIdent = other.m_dmRecipientIdent;
		m_dmSenderIdent = other.m_dmSenderIdent;

		/* Act addressing. */
		m_dmLegalTitleLaw = other.m_dmLegalTitleLaw;
		m_dmLegalTitleYear = other.m_dmLegalTitleYear;
		m_dmLegalTitleSect = other.m_dmLegalTitleSect;
		m_dmLegalTitlePar = other.m_dmLegalTitlePar;
		m_dmLegalTitlePoint = other.m_dmLegalTitlePoint;
		m_dmPersonalDelivery = other.m_dmPersonalDelivery;
		m_dmAllowSubstDelivery = other.m_dmAllowSubstDelivery;
		m_dmType = other.m_dmType;

		/* Outgoing messages only. */
		m_dmOVM = other.m_dmOVM;
		m_dmPublishOwnID = other.m_dmPublishOwnID;

		return *this;
	}

	QString m_dmID;
	QString m_dbIDSender;
	QString m_dmSender;
	QString m_dmSenderAddress;
	enum Type::DbType m_dmSenderType;
	QString m_dmRecipient;
	QString m_dmRecipientAddress;
	enum Type::NilBool m_dmAmbiguousRecipient;

	quint64 m_dmOrdinal;
	enum Type::DmState m_dmMessageStatus;
	qint64 m_dmAttachmentSize;
	QDateTime m_dmDeliveryTime;
	QDateTime m_dmAcceptanceTime;
	Hash m_dmHash;
	QByteArray m_dmQTimestamp;
	QList<Event> m_dmEvents;

	QString m_dmSenderOrgUnit;
	qint64 m_dmSenderOrgUnitNum;
	QString m_dbIDRecipient;
	QString m_dmRecipientOrgUnit;
	qint64 m_dmRecipientOrgUnitNum;
	QString m_dmToHands;
	QString m_dmAnnotation;
	QString m_dmRecipientRefNumber;
	QString m_dmSenderRefNumber;
	QString m_dmRecipientIdent;
	QString m_dmSenderIdent;

	/* Act addressing. */
	qint64 m_dmLegalTitleLaw;
	qint64 m_dmLegalTitleYear;
	QString m_dmLegalTitleSect;
	QString m_dmLegalTitlePar;
	QString m_dmLegalTitlePoint;
	enum Type::NilBool m_dmPersonalDelivery;
	enum Type::NilBool m_dmAllowSubstDelivery;
	QChar m_dmType;

	/* Outgoing messages only. */
	enum Type::NilBool m_dmOVM;
	enum Type::NilBool m_dmPublishOwnID;
};

Isds::Envelope::Envelope(void)
    : d_ptr(Q_NULLPTR)
{
}

Isds::Envelope::Envelope(const Envelope &other)
    : d_ptr((other.d_func() != Q_NULLPTR) ? (new (std::nothrow) EnvelopePrivate) : Q_NULLPTR)
{
	Q_D(Envelope);
	if (d == Q_NULLPTR) {
		return;
	}

	*d = *other.d_func();
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::Envelope::Envelope(Envelope &&other) Q_DECL_NOEXCEPT
    : d_ptr(other.d_ptr.take()) //d_ptr(std::move(other.d_ptr))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::Envelope::~Envelope(void)
{
}

/*!
 * @brief Ensures private envelope presence.
 *
 * @note Returns if private envelope could not be allocated.
 */
#define ensureEnvelopePrivate(_x_) \
	do { \
		if (Q_UNLIKELY(d_ptr == Q_NULLPTR)) { \
			EnvelopePrivate *p = new (std::nothrow) EnvelopePrivate; \
			if (Q_UNLIKELY(p == Q_NULLPTR)) { \
				Q_ASSERT(0); \
				return _x_; \
			} \
			d_ptr.reset(p); \
		} \
	} while (0)

Isds::Envelope &Isds::Envelope::operator=(const Envelope &other) Q_DECL_NOTHROW
{
	if (other.d_func() == Q_NULLPTR) {
		d_ptr.reset(Q_NULLPTR);
		return *this;
	}
	ensureEnvelopePrivate(*this);
	Q_D(Envelope);

	*d = *other.d_func();

	return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::Envelope &Isds::Envelope::operator=(Envelope &&other) Q_DECL_NOTHROW
{
	swap(*this, other);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */

bool Isds::Envelope::isNull(void) const
{
	Q_D(const Envelope);
	return d == Q_NULLPTR;
}

qint64 Isds::Envelope::dmId(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return -1;
	}

	bool ok = false;
	qint64 id = d->m_dmID.toLongLong(&ok);
	return ok ? id : -1;
}

void Isds::Envelope::setDmId(qint64 id)
{
	setDmID((id >= 0) ? QString::number(id) : QString());
}

const QString &Isds::Envelope::dmID(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dmID;
}

void Isds::Envelope::setDmID(const QString &id)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmID = id;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Envelope::setDmID(QString &&id)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmID = id;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::Envelope::dbIDSender(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dbIDSender;
}

void Isds::Envelope::setDbIDSender(const QString &sbi)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dbIDSender= sbi;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Envelope::setDbIDSender(QString &&sbi)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dbIDSender= sbi;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::Envelope::dmSender(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dmSender;
}

void Isds::Envelope::setDmSender(const QString &sn)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmSender = sn;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Envelope::setDmSender(QString &&sn)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmSender = sn;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::Envelope::dmSenderAddress(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dmSenderAddress;
}

void Isds::Envelope::setDmSenderAddress(const QString &sa)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmSenderAddress = sa;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Envelope::setDmSenderAddress(QString &&sa)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmSenderAddress = sa;
}
#endif /* Q_COMPILER_RVALUE_REFS */

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
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return Type::BT_NULL;
	}
	return d->m_dmSenderType;
}

void Isds::Envelope::setDmSenderType(enum Type::DbType st)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmSenderType = st;
}

const QString &Isds::Envelope::dmRecipient(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dmRecipient;
}

void Isds::Envelope::setDmRecipient(const QString &rn)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmRecipient = rn;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Envelope::setDmRecipient(QString &&rn)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmRecipient = rn;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::Envelope::dmRecipientAddress(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dmRecipientAddress;
}

void Isds::Envelope::setDmRecipientAddress(const QString &ra)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmRecipientAddress = ra;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Envelope::setDmRecipientAddress(QString &&ra)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmRecipientAddress = ra;
}
#endif /* Q_COMPILER_RVALUE_REFS */

enum Isds::Type::NilBool Isds::Envelope::dmAmbiguousRecipient(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return Type::BOOL_NULL;
	}
	return d->m_dmAmbiguousRecipient;
}

void Isds::Envelope::setDmAmbiguousRecipient(enum Type::NilBool ar)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmAmbiguousRecipient = ar;
}

quint64 Isds::Envelope::dmOrdinal(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return 0;
	}
	return d->m_dmOrdinal;
}

void Isds::Envelope::setDmOrdinal(quint64 o)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmOrdinal = o;
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
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return Type::MS_NULL;
	}
	return d->m_dmMessageStatus;
}

void Isds::Envelope::setDmMessageStatus(enum Type::DmState s)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmMessageStatus = s;
}

qint64 Isds::Envelope::dmAttachmentSize(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return -1;
	}
	return d->m_dmAttachmentSize;
}

void Isds::Envelope::setDmAttachmentSize(qint64 as)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmAttachmentSize = as;
}

const QDateTime &Isds::Envelope::dmDeliveryTime(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullDateTime;
	}
	return d->m_dmDeliveryTime;
}

void Isds::Envelope::setDmDeliveryTime(const QDateTime &dt)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmDeliveryTime = dt;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Envelope::setDmDeliveryTime(QDateTime &&dt)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmDeliveryTime = dt;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QDateTime &Isds::Envelope::dmAcceptanceTime(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullDateTime;
	}
	return d->m_dmAcceptanceTime;
}

void Isds::Envelope::setDmAcceptanceTime(const QDateTime &at)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmAcceptanceTime = at;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Envelope::setDmAcceptanceTime(QDateTime &&at)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmAcceptanceTime = at;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const Isds::Hash &Isds::Envelope::dmHash(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullHash;
	}
	return d->m_dmHash;
}

void Isds::Envelope::setDmHash(const Hash &h)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmHash = h;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Envelope::setDmHash(Hash &&h)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmHash = h;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QByteArray &Isds::Envelope::dmQTimestamp(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullByteArray;
	}
	return d->m_dmQTimestamp;
}

void Isds::Envelope::setDmQTimestamp(const QByteArray &ts)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmQTimestamp = ts;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Envelope::setDmQTimestamp(QByteArray &&ts)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmQTimestamp = ts;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QList<Isds::Event> &Isds::Envelope::dmEvents(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullEventList;
	}
	return d->m_dmEvents;
}

void Isds::Envelope::setDmEvents(const QList<Event> &el)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmEvents = el;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Envelope::setDmEvents(QList<Event> &&el)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmEvents = el;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::Envelope::dmSenderOrgUnit(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dmSenderOrgUnit;
}

void Isds::Envelope::setDmSenderOrgUnit(const QString &sou)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmSenderOrgUnit = sou;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Envelope::setDmSenderOrgUnit(QString &&sou)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmSenderOrgUnit = sou;
}
#endif /* Q_COMPILER_RVALUE_REFS */

qint64 Isds::Envelope::dmSenderOrgUnitNum(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return -1;
	}
	return d->m_dmSenderOrgUnitNum;
}

void Isds::Envelope::setDmSenderOrgUnitNum(qint64 soun)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmSenderOrgUnitNum = (soun >= 0) ? soun : -1;
}

const QString &Isds::Envelope::dbIDRecipient(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dbIDRecipient;
}

void Isds::Envelope::setDbIDRecipient(const QString &rbi)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dbIDRecipient = rbi;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Envelope::setDbIDRecipient(QString &&rbi)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dbIDRecipient = rbi;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::Envelope::dmRecipientOrgUnit(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dmRecipientOrgUnit;
}

void Isds::Envelope::setDmRecipientOrgUnit(const QString &rou)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmRecipientOrgUnit = rou;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Envelope::setDmRecipientOrgUnit(QString &&rou)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmRecipientOrgUnit = rou;
}
#endif /* Q_COMPILER_RVALUE_REFS */

qint64 Isds::Envelope::dmRecipientOrgUnitNum(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return -1;
	}
	return d->m_dmRecipientOrgUnitNum;
}

void Isds::Envelope::setDmRecipientOrgUnitNum(qint64 roun)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmRecipientOrgUnitNum = (roun >= 0) ? roun : -1;
}

const QString &Isds::Envelope::dmToHands(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dmToHands;
}

void Isds::Envelope::setDmToHands(const QString &th)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmToHands = th;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Envelope::setDmToHands(QString &&th)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmToHands = th;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::Envelope::dmAnnotation(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dmAnnotation;
}

void Isds::Envelope::setDmAnnotation(const QString &a)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmAnnotation = a;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Envelope::setDmAnnotation(QString &&a)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmAnnotation = a;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::Envelope::dmRecipientRefNumber(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dmRecipientRefNumber;
}

void Isds::Envelope::setDmRecipientRefNumber(const QString &rrn)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmRecipientRefNumber = rrn;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Envelope::setDmRecipientRefNumber(QString &&rrn)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmRecipientRefNumber = rrn;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::Envelope::dmSenderRefNumber(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dmSenderRefNumber;
}

void Isds::Envelope::setDmSenderRefNumber(const QString &srn)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmSenderRefNumber = srn;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Envelope::setDmSenderRefNumber(QString &&srn)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmSenderRefNumber = srn;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::Envelope::dmRecipientIdent(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dmRecipientIdent;
}

void Isds::Envelope::setDmRecipientIdent(const QString &ri)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmRecipientIdent = ri;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Envelope::setDmRecipientIdent(QString &&ri)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmRecipientIdent = ri;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::Envelope::dmSenderIdent(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dmSenderIdent;
}

void Isds::Envelope::setDmSenderIdent(const QString &si)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmSenderIdent = si;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Envelope::setDmSenderIdent(QString &&si)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmSenderIdent = si;
}
#endif /* Q_COMPILER_RVALUE_REFS */

qint64 Isds::Envelope::dmLegalTitleLaw(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return -1;
	}
	return d->m_dmLegalTitleLaw;
}

void Isds::Envelope::setDmLegalTitleLaw(qint64 l)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmLegalTitleLaw = (l >= 0) ? l : -1;
}

qint64 Isds::Envelope::dmLegalTitleYear(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return -1;
	}
	return d->m_dmLegalTitleYear;
}

void Isds::Envelope::setDmLegalTitleYear(qint64 y)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmLegalTitleYear = (y >= 0) ? y : -1;
}

const QString &Isds::Envelope::dmLegalTitleSect(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dmLegalTitleSect;
}

void Isds::Envelope::setDmLegalTitleSect(const QString &s)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmLegalTitleSect = s;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Envelope::setDmLegalTitleSect(QString &&s)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmLegalTitleSect = s;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::Envelope::dmLegalTitlePar(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dmLegalTitlePar;
}

void Isds::Envelope::setDmLegalTitlePar(const QString &p)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmLegalTitlePar = p;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Envelope::setDmLegalTitlePar(QString &&p)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmLegalTitlePar = p;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Isds::Envelope::dmLegalTitlePoint(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dmLegalTitlePoint;
}

void Isds::Envelope::setDmLegalTitlePoint(const QString &p)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmLegalTitlePoint = p;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Envelope::setDmLegalTitlePoint(QString &&p)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmLegalTitlePoint = p;
}
#endif /* Q_COMPILER_RVALUE_REFS */

enum Isds::Type::NilBool Isds::Envelope::dmPersonalDelivery(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return Type::BOOL_NULL;
	}
	return d->m_dmPersonalDelivery;
}

void Isds::Envelope::setDmPersonalDelivery(enum Type::NilBool pd)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmPersonalDelivery = pd;
}

enum Isds::Type::NilBool Isds::Envelope::dmAllowSubstDelivery(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return Type::BOOL_NULL;
	}
	return d->m_dmAllowSubstDelivery;
}

void Isds::Envelope::setDmAllowSubstDelivery(enum Type::NilBool sd)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmAllowSubstDelivery = sd;
}

QChar Isds::Envelope::dmType(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return QChar();
	}
	return d->m_dmType;
}

void Isds::Envelope::setDmType(QChar t)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmType = t;
}

enum Isds::Type::NilBool Isds::Envelope::dmOVM(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return Type::BOOL_NULL;
	}
	return d->m_dmOVM;
}

void Isds::Envelope::setDmOVM(enum Type::NilBool ovm)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmOVM = ovm;
}

enum Isds::Type::NilBool Isds::Envelope::dmPublishOwnID(void) const
{
	Q_D(const Envelope);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return Type::BOOL_NULL;
	}
	return d->m_dmPublishOwnID;
}

void Isds::Envelope::setDmPublishOwnID(enum Type::NilBool poi)
{
	ensureEnvelopePrivate();
	Q_D(Envelope);
	d->m_dmPublishOwnID = poi;
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

void Isds::swap(Envelope &first, Envelope &second) Q_DECL_NOTHROW
{
	using std::swap;
	swap(first.d_ptr, second.d_ptr);
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
 * @brief Converts event list.
 */
static
QList<Isds::Event> libisds2eventList(const struct isds_list *item,
    bool *ok = Q_NULLPTR)
{
	QList<Isds::Event> eventList;

	while (item != NULL) {
		const struct isds_event *ev = (struct isds_event *)item->data;

		if (ev != NULL) {
			bool iOk = false;
			eventList.append(Isds::libisds2event(ev, &iOk));
			if (!iOk) {
				if (ok != Q_NULLPTR) {
					*ok = false;
				}
				return QList<Isds::Event>();
			}
		}

		item = item->next;
	}

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return eventList;
}

Isds::Envelope Isds::libisds2envelope(const struct isds_envelope *ie, bool *ok)
{
	if (Q_UNLIKELY(ie == NULL)) {
		Q_ASSERT(0);
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return Envelope();
	}

	bool iOk = false;
	Envelope env;

	env.setDmID(fromCStr(ie->dmID));
	env.setDbIDSender(fromCStr(ie->dbIDSender));
	env.setDmSender(fromCStr(ie->dmSender));
	env.setDmSenderAddress(fromCStr(ie->dmSenderAddress));
	env.setDmSenderType(long2DbType(ie->dmSenderType));
	env.setDmRecipient(fromCStr(ie->dmRecipient));
	env.setDmRecipientAddress(fromCStr(ie->dmRecipientAddress));
	env.setDmAmbiguousRecipient(fromBool(ie->dmAmbiguousRecipient));

	env.setDmOrdinal((ie->dmOrdinal != NULL) ? *ie->dmOrdinal : 0); /* FIXME */
	env.setDmMessageStatus(libisdsMessageStatus2DmState(ie->dmMessageStatus));
	env.setDmAttachmentSize(fromLongInt(ie->dmAttachmentSize));
	env.setDmDeliveryTime(dateTimeFromStructTimeval(ie->dmDeliveryTime));
	env.setDmAcceptanceTime(dateTimeFromStructTimeval(ie->dmAcceptanceTime));
	env.setDmHash(libisds2hash(ie->hash, &iOk));
	if (Q_UNLIKELY(!iOk)) {
		goto fail;
	}
	env.setDmQTimestamp(fromCData(ie->timestamp, ie->timestamp_length));
	env.setDmEvents(libisds2eventList(ie->events, &iOk));
	if (Q_UNLIKELY(!iOk)) {
		goto fail;
	}

	env.setDmSenderOrgUnit(fromCStr(ie->dmSenderOrgUnit));
	env.setDmSenderOrgUnitNum(fromLongInt(ie->dmSenderOrgUnitNum));
	env.setDbIDRecipient(fromCStr(ie->dbIDRecipient));
	env.setDmRecipientOrgUnit(fromCStr(ie->dmRecipientOrgUnit));
	env.setDmRecipientOrgUnitNum(fromLongInt(ie->dmRecipientOrgUnitNum));
	env.setDmToHands(fromCStr(ie->dmToHands));
	env.setDmAnnotation(fromCStr(ie->dmAnnotation));
	env.setDmRecipientRefNumber(fromCStr(ie->dmRecipientRefNumber));
	env.setDmSenderRefNumber(fromCStr(ie->dmSenderRefNumber));
	env.setDmRecipientIdent(fromCStr(ie->dmRecipientIdent));
	env.setDmSenderIdent(fromCStr(ie->dmSenderIdent));

	env.setDmLegalTitleLaw(fromLongInt(ie->dmLegalTitleLaw));
	env.setDmLegalTitleYear(fromLongInt(ie->dmLegalTitleYear));
	env.setDmLegalTitleSect(fromCStr(ie->dmLegalTitleSect));
	env.setDmLegalTitlePar(fromCStr(ie->dmLegalTitlePar));
	env.setDmLegalTitlePoint(fromCStr(ie->dmLegalTitlePoint));
	env.setDmPersonalDelivery(fromBool(ie->dmPersonalDelivery));
	env.setDmAllowSubstDelivery(fromBool(ie->dmAllowSubstDelivery));
	env.setDmType((ie->dmType != NULL) ? QChar(*ie->dmType) : QChar());

	env.setDmOVM(fromBool(ie->dmOVM));
	env.setDmPublishOwnID(fromBool(ie->dmPublishOwnID));

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return env;

fail:
	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return Envelope();
}

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

#ifdef Q_COMPILER_RVALUE_REFS
Isds::Document::Document(Document &&other) Q_DECL_NOEXCEPT
    : d_ptr(other.d_ptr.take()) //d_ptr(std::move(other.d_ptr))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

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

const QByteArray &Isds::Document::binaryContent(void) const
{
	Q_D(const Document);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullByteArray;
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

const QString &Isds::Document::mimeType(void) const
{
	Q_D(const Document);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
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

const QString &Isds::Document::fileGuid(void) const
{
	Q_D(const Document);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
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

const QString &Isds::Document::upFileGuid(void) const
{
	Q_D(const Document);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
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

const  QString &Isds::Document::fileDescr(void) const
{
	Q_D(const Document);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
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

const QString &Isds::Document::format(void) const
{
	Q_D(const Document);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
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

void Isds::swap(Document &first, Document &second) Q_DECL_NOTHROW
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

struct isds_document *Isds::document2libisds(const Document &doc)
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
	if (Q_UNLIKELY(!toCDataCopy(&idoc->data, &idoc->data_length,
	                   doc.binaryContent()))) {
		goto fail;
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
