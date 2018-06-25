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

#include <cstdlib> // malloc
#include <cstring> // memcpy
#include <isds.h>

#include "src/datovka_shared/isds/internal_conversion.h"
#include "src/datovka_shared/isds/type_conversion.h"
#include "src/isds/conversion_internal.h"
#include "src/isds/message_conversion.h"

/*!
 * @brief Converts user types.
 */
static
Isds::Type::HashAlg libisdsHashAlg2HashAlg(isds_hash_algorithm a,
    bool *ok = Q_NULLPTR)
{
	bool iOk = true;
	enum Isds::Type::HashAlg alg = Isds::Type::HA_UNKNOWN;

	switch (a) {
	case HASH_ALGORITHM_MD5: alg = Isds::Type::HA_MD5; break;
	case HASH_ALGORITHM_SHA_1: alg = Isds::Type::HA_SHA_1; break;
	case HASH_ALGORITHM_SHA_224: alg = Isds::Type::HA_SHA_224; break;
	case HASH_ALGORITHM_SHA_256: alg = Isds::Type::HA_SHA_256; break;
	case HASH_ALGORITHM_SHA_384: alg = Isds::Type::HA_SHA_384; break;
	case HASH_ALGORITHM_SHA_512: alg = Isds::Type::HA_SHA_512; break;
	default:
		Q_ASSERT(0);
		iOk = false;
		break;
	}

	if (ok != Q_NULLPTR) {
		*ok = iOk;
	}
	return alg;
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

	bool iOk = false;

	tgt.setAlgorithm(libisdsHashAlg2HashAlg(src->algorithm, &iOk));
	if (Q_UNLIKELY(!iOk)) {
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
isds_hash_algorithm hashAlg2libisdsHashAlg(Isds::Type::HashAlg a,
    bool *ok = Q_NULLPTR)
{
	bool iOk = true;
	isds_hash_algorithm alg = HASH_ALGORITHM_MD5; /* TODO -- This value is clearly incorrect. */

	switch (a) {
	case Isds::Type::HA_MD5: alg = HASH_ALGORITHM_MD5; break;
	case Isds::Type::HA_SHA_1: alg = HASH_ALGORITHM_SHA_1; break;
	case Isds::Type::HA_SHA_224: alg = HASH_ALGORITHM_SHA_224; break;
	case Isds::Type::HA_SHA_256: alg = HASH_ALGORITHM_SHA_256; break;
	case Isds::Type::HA_SHA_384: alg = HASH_ALGORITHM_SHA_384; break;
	case Isds::Type::HA_SHA_512: alg = HASH_ALGORITHM_SHA_512; break;
	default:
		Q_ASSERT(0);
		iOk = false;
		break;
	}

	if (ok != Q_NULLPTR) {
		*ok = iOk;
	}
	return alg;
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

	bool iOk = false;

	tgt->algorithm = hashAlg2libisdsHashAlg(src.algorithm(), &iOk);
	if (Q_UNLIKELY(!iOk)) {
		return false;
	}
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

struct isds_hash *Isds::hash2libisds(const Hash &h, bool *ok)
{
	if (Q_UNLIKELY(h.isNull())) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return NULL;
	}

	struct isds_hash *ih = (struct isds_hash *)std::malloc(sizeof(*ih));
	if (Q_UNLIKELY(ih == NULL)) {
		Q_ASSERT(0);
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return NULL;
	}
	std::memset(ih, 0, sizeof(*ih));

	if (Q_UNLIKELY(!setLibisdsHashContent(ih, h))) {
		isds_hash_free(&ih);
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return NULL;
	}
	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return ih;
}

/*!
 * @brief Converts event type.
 */
static
enum Isds::Type::Event libisdsEventType2EventType(isds_event_type *src,
    bool *ok = Q_NULLPTR)
{
	if (Q_UNLIKELY(src == NULL)) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return Isds::Type::EV_UNKNOWN;
	}

	bool iOk = true;
	enum Isds::Type::Event event = Isds::Type::EV_UNKNOWN;

	switch (*src) {
	case EVENT_UKNOWN: event = Isds::Type::EV_UNKNOWN; break;
	case EVENT_ENTERED_SYSTEM: event = Isds::Type::EV_ENTERED; break;
	case EVENT_DELIVERED: event = Isds::Type::EV_DELIVERED; break;
	case EVENT_ACCEPTED_BY_RECIPIENT: event = Isds::Type::EV_ACCEPTED_LOGIN; break;
	case EVENT_PRIMARY_LOGIN: event = Isds::Type::EV_PRIMARY_LOGIN; break;
	case EVENT_ENTRUSTED_LOGIN: event = Isds::Type::EV_ENTRUSTED_LOGIN; break;
	case EVENT_SYSCERT_LOGIN: event = Isds::Type::EV_SYSCERT_LOGIN; break;
	case EVENT_ACCEPTED_BY_FICTION: event = Isds::Type::EV_ACCEPTED_FICTION; break;
	case EVENT_UNDELIVERABLE: event = Isds::Type::EV_UNDELIVERABLE; break;
	case EVENT_COMMERCIAL_ACCEPTED: event = Isds::Type::EV_ACCEPTED_BY_RECIPIENT; break;
	case EVENT_UNDELIVERED_AV_CHECK: event = Isds::Type::EV_UNDELIVERED_AV_CHECK; break;
	default:
		iOk = false;
		break;
	}

	if (ok != Q_NULLPTR) {
		*ok = iOk;
	}
	return event;
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

	bool iOk = false;

	tgt.setTime(Isds::dateTimeFromStructTimeval(src->time));
	tgt.setType(libisdsEventType2EventType(src->type, &iOk));
	if (Q_UNLIKELY(!iOk)) {
		return false;
	}
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
bool eventType2libisdsEventType(isds_event_type **tgt,
    enum Isds::Type::Event src)
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
	case Isds::Type::EV_UNDELIVERED_AV_CHECK: **tgt = EVENT_UNDELIVERED_AV_CHECK; break;
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
	if (Q_UNLIKELY(!eventType2libisdsEventType(&tgt->type, src.type()))) {
		return false;
	}
	if (Q_UNLIKELY(!Isds::toCStrCopy(&tgt->description, src.descr()))) {
		return false;
	}

	return true;
}

struct isds_event *Isds::event2libisds(const Event &e, bool *ok)
{
	if (Q_UNLIKELY(e.isNull())) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return NULL;
	}

	struct isds_event *ie = (struct isds_event *)std::malloc(sizeof(*ie));
	if (Q_UNLIKELY(ie == NULL)) {
		Q_ASSERT(0);
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return NULL;
	}
	std::memset(ie, 0, sizeof(*ie));

	if (Q_UNLIKELY(!setLibisdsEventContent(ie, e))) {
		isds_event_free(&ie);
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return NULL;
	}
	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return ie;
}

/*!
 * @brief Converts data box types.
 */
static
enum Isds::Type::DbType longPtr2DbType(const long int *bt, bool *ok = Q_NULLPTR)
{
	if (bt == NULL) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return Isds::Type::BT_NULL;
	}

	return Isds::long2DbType(*bt, ok);
}

/*!
 * @brief Converts message status.
 */
static
enum Isds::Type::DmState libisdsMessageStatus2DmState(
    const isds_message_status *ms, bool *ok = Q_NULLPTR)
{
	if (ms == NULL) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return Isds::Type::MS_NULL;
	}

	bool iOk = true;
	enum Isds::Type::DmState state = Isds::Type::MS_NULL;

	switch (*ms) {
	case MESSAGESTATE_SENT: state = Isds::Type::MS_POSTED; break;
	case MESSAGESTATE_STAMPED: state = Isds::Type::MS_STAMPED; break;
	case MESSAGESTATE_INFECTED: state = Isds::Type::MS_INFECTED; break;
	case MESSAGESTATE_DELIVERED: state = Isds::Type::MS_DELIVERED; break;
	case MESSAGESTATE_SUBSTITUTED: state = Isds::Type::MS_ACCEPTED_FICT; break;
	case MESSAGESTATE_RECEIVED: state = Isds::Type::MS_ACCEPTED; break;
	case MESSAGESTATE_READ: state = Isds::Type::MS_READ; break;
	case MESSAGESTATE_UNDELIVERABLE: state = Isds::Type::MS_UNDELIVERABLE; break;
	case MESSAGESTATE_REMOVED: state = Isds::Type::MS_REMOVED; break;
	case MESSAGESTATE_IN_SAFE: state = Isds::Type::MS_IN_VAULT; break;
	default:
		iOk = false;
		break;
	}

	if (ok != Q_NULLPTR) {
		*ok = iOk;
	}
	return state;
}

/*!
 * @brief Converts event list.
 */
static
QList<Isds::Event> libisds2eventList(const struct isds_list *item,
    bool *ok = Q_NULLPTR)
{
	/* Event destructor function type. */
	typedef void (*evnt_destr_func_t)(struct isds_event **);

	QList<Isds::Event> eventList;

	while (item != NULL) {
		const struct isds_event *ie = (struct isds_event *)item->data;
		evnt_destr_func_t idestr = (evnt_destr_func_t)item->destructor;
		/* Destructor function must be set. */
		if (!IsdsInternal::crudeEqual(idestr, isds_event_free)) {
			Q_ASSERT(0);
			if (ok != Q_NULLPTR) {
				*ok = false;
			}
			return QList<Isds::Event>();
		}

		if (ie != NULL) {
			bool iOk = false;
			eventList.append(Isds::libisds2event(ie, &iOk));
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
	env.setDmSenderType(longPtr2DbType(ie->dmSenderType, &iOk));
	if (Q_UNLIKELY(!iOk)) {
		goto fail;
	}
	env.setDmRecipient(fromCStr(ie->dmRecipient));
	env.setDmRecipientAddress(fromCStr(ie->dmRecipientAddress));
	env.setDmAmbiguousRecipient(fromBoolPtr(ie->dmAmbiguousRecipient));

	env.setDmOrdinal((ie->dmOrdinal != NULL) ? *ie->dmOrdinal : 0); /* FIXME ? */
	env.setDmMessageStatus(libisdsMessageStatus2DmState(ie->dmMessageStatus, &iOk));
	if (Q_UNLIKELY(!iOk)) {
		goto fail;
	}
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
	env.setDmPersonalDelivery(fromBoolPtr(ie->dmPersonalDelivery));
	env.setDmAllowSubstDelivery(fromBoolPtr(ie->dmAllowSubstDelivery));
	env.setDmType((ie->dmType != NULL) ? QChar(*ie->dmType) : QChar());

	env.setDmOVM(fromBoolPtr(ie->dmOVM));
	env.setDmPublishOwnID(fromBoolPtr(ie->dmPublishOwnID));

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
 * @brief Converts data box types.
 */
static
bool dbType2longPtr(long int **btPtr, enum Isds::Type::DbType bt)
{
	if (Q_UNLIKELY(btPtr == Q_NULLPTR)) {
		Q_ASSERT(0);
		return false;
	}
	if (*btPtr == NULL) {
		*btPtr = (long int *)std::malloc(sizeof(**btPtr));
		if (Q_UNLIKELY(btPtr == NULL)) {
			Q_ASSERT(0);
			return false;
		}
	}
	switch (bt) {
	case Isds::Type::BT_NULL:
		std::free(*btPtr); *btPtr = NULL;
		break;
	default:
		**btPtr = bt;
		break;
	}

	return true;
}

/*!
 * @brief Creates a unsigned long int from supplied number.
 *
 * @note Zero causes the pointer to be set to NULL.
 *
 * @param[in,out] cULongPtr Pointer to unsigned long int.
 * @param[in] u Unsigned integerer.
 * @return True on success, false in failure.
 */
static
bool toLongUInt(unsigned long int **cULongPtr, quint64 u)
{
	if (Q_UNLIKELY(cULongPtr == Q_NULLPTR)) {
		Q_ASSERT(0);
		return false;
	}
	if (u == 0) {
		if (*cULongPtr != NULL) {
			std::free(*cULongPtr); *cULongPtr = NULL;
		}
		return true;
	}
	if (*cULongPtr == NULL) {
		*cULongPtr =
		    (unsigned long int*)std::malloc(sizeof(**cULongPtr));
		if (Q_UNLIKELY(*cULongPtr == NULL)) {
			Q_ASSERT(0);
			return false;
		}
	}

	**cULongPtr = u;
	return true;
}

/*!
 * @brief Converts message status.
 */
static
bool dmState2libisdsMessageStatus(isds_message_status **tgt,
    enum Isds::Type::DmState src)
{
	if (Q_UNLIKELY(tgt == NULL)) {
		Q_ASSERT(0);
		return false;
	}
	if (src == Isds::Type::MS_NULL) {
		if (*tgt != NULL) {
			std::free(*tgt); *tgt = NULL;
		}
		return true;
	}
	if (*tgt == NULL) {
		*tgt = (isds_message_status *)std::malloc(sizeof(**tgt));
		if (Q_UNLIKELY(*tgt == NULL)) {
			Q_ASSERT(0);
			return false;
		}
	}
	switch (src) {
	/*
	 * Isds::Type::MS_NULL cannot be reached here.
	 *
	 * case Isds::Type::MS_NULL:
	 * 	std::free(*tgt); *tgt = NULL;
	 * 	break;
	 */
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
		return false;
		break;
	}

	return true;
}

/*!
 * @brief Converts event list.
 */
static
struct isds_list *eventList2libisds(const QList<Isds::Event> &el,
    bool *ok = Q_NULLPTR)
{
	if (Q_UNLIKELY(el.isEmpty())) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return NULL;
	}

	struct isds_list *iel = NULL;
	struct isds_list *lastItem = NULL;
	foreach (const Isds::Event &ev, el) {
		struct isds_list *item =
		    (struct isds_list *)std::malloc(sizeof(*item));
		if (Q_UNLIKELY(item == NULL)) {
			Q_ASSERT(0);
			goto fail;
		}
		std::memset(item, 0, sizeof(*item));

		bool iOk = false;
		struct isds_event *iev = Isds::event2libisds(ev, &iOk);
		if (Q_UNLIKELY(!iOk)) {
			std::free(item); item = NULL;
			goto fail;
		}

		/* Set list item. */
		item->next = NULL;
		item->data = iev;
		item->destructor = (void (*)(void **))isds_event_free;

		/* Append item. */
		if (lastItem == NULL) {
			iel = item;
		} else {
			lastItem->next = item;
		}
		lastItem = item;
	}

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return iel;

fail:
	isds_list_free(&iel);
	if (ok != Q_NULLPTR) {
		*ok = false;
	}
	return NULL;
}

struct isds_envelope *Isds::envelope2libisds(const Envelope &env, bool *ok)
{
	if (Q_UNLIKELY(env.isNull())) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return NULL;
	}

	struct isds_envelope *ienv =
	    (struct isds_envelope *)std::malloc(sizeof(*ienv));
	if (Q_UNLIKELY(ienv == NULL)) {
		Q_ASSERT(0);
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return NULL;
	}
	std::memset(ienv, 0, sizeof(*ienv));

	bool iOk = false;

	if (Q_UNLIKELY(!toCStrCopy(&ienv->dmID, env.dmID()))) { goto fail; }
	if (Q_UNLIKELY(!toCStrCopy(&ienv->dbIDSender, env.dbIDSender()))) { goto fail; }
	if (Q_UNLIKELY(!toCStrCopy(&ienv->dmSender, env.dmSender()))) { goto fail; }
	if (Q_UNLIKELY(!toCStrCopy(&ienv->dmSenderAddress, env.dmSenderAddress()))) { goto fail; }
	if (Q_UNLIKELY(!dbType2longPtr(&ienv->dmSenderType, env.dmSenderType()))) { goto fail; }
	if (Q_UNLIKELY(!toCStrCopy(&ienv->dmRecipient, env.dmRecipient()))) { goto fail; }
	if (Q_UNLIKELY(!toCStrCopy(&ienv->dmRecipientAddress, env.dmRecipientAddress()))) { goto fail; }
	if (Q_UNLIKELY(!toBoolPtr(&ienv->dmAmbiguousRecipient, env.dmAmbiguousRecipient()))) { goto fail; }

	if (Q_UNLIKELY(!toLongUInt(&ienv->dmOrdinal, env.dmOrdinal()))) { goto fail; }
	if (Q_UNLIKELY(!dmState2libisdsMessageStatus(&ienv->dmMessageStatus, env.dmMessageStatus()))) { goto fail; }
	if (Q_UNLIKELY(!toLongInt(&ienv->dmAttachmentSize, env.dmAttachmentSize()))) { goto fail; }
	if (Q_UNLIKELY(!toCDateTimeCopy(&ienv->dmDeliveryTime, env.dmDeliveryTime()))) { goto fail; }
	if (Q_UNLIKELY(!toCDateTimeCopy(&ienv->dmAcceptanceTime, env.dmAcceptanceTime()))) { goto fail; }
	ienv->hash = hash2libisds(env.dmHash(), &iOk); if (Q_UNLIKELY(!iOk)) { goto fail; }
	if (Q_UNLIKELY(!toCDataCopy(&ienv->timestamp, &ienv->timestamp_length, env.dmQTimestamp()))) { goto fail; }
	ienv->events = eventList2libisds(env.dmEvents(), &iOk); if (Q_UNLIKELY(!iOk)) { goto fail; }

	if (Q_UNLIKELY(!toCStrCopy(&ienv->dmSenderOrgUnit, env.dmSenderOrgUnit()))) { goto fail; }
	if (Q_UNLIKELY(!toLongInt(&ienv->dmSenderOrgUnitNum, env.dmSenderOrgUnitNum()))) { goto fail; }
	if (Q_UNLIKELY(!toCStrCopy(&ienv->dbIDRecipient, env.dbIDRecipient()))) { goto fail; }
	if (Q_UNLIKELY(!toCStrCopy(&ienv->dmRecipientOrgUnit, env.dmRecipientOrgUnit()))) { goto fail; }
	if (Q_UNLIKELY(!toLongInt(&ienv->dmRecipientOrgUnitNum, env.dmRecipientOrgUnitNum()))) { goto fail; }
	if (Q_UNLIKELY(!toCStrCopy(&ienv->dmToHands, env.dmToHands()))) { goto fail; }
	if (Q_UNLIKELY(!toCStrCopy(&ienv->dmAnnotation, env.dmAnnotation()))) { goto fail; }
	if (Q_UNLIKELY(!toCStrCopy(&ienv->dmRecipientRefNumber, env.dmRecipientRefNumber()))) { goto fail; }
	if (Q_UNLIKELY(!toCStrCopy(&ienv->dmSenderRefNumber, env.dmSenderRefNumber()))) { goto fail; }
	if (Q_UNLIKELY(!toCStrCopy(&ienv->dmRecipientIdent, env.dmRecipientIdent()))) { goto fail; }
	if (Q_UNLIKELY(!toCStrCopy(&ienv->dmSenderIdent, env.dmSenderIdent()))) { goto fail; }

	if (Q_UNLIKELY(!toLongInt(&ienv->dmLegalTitleLaw, env.dmLegalTitleLaw()))) { goto fail; }
	if (Q_UNLIKELY(!toLongInt(&ienv->dmLegalTitleYear, env.dmLegalTitleYear()))) { goto fail; }
	if (Q_UNLIKELY(!toCStrCopy(&ienv->dmLegalTitleSect, env.dmLegalTitleSect()))) { goto fail; }
	if (Q_UNLIKELY(!toCStrCopy(&ienv->dmLegalTitlePar, env.dmLegalTitlePar()))) { goto fail; }
	if (Q_UNLIKELY(!toCStrCopy(&ienv->dmLegalTitlePoint, env.dmLegalTitlePoint()))) { goto fail; }
	if (Q_UNLIKELY(!toBoolPtr(&ienv->dmPersonalDelivery, env.dmPersonalDelivery()))) { goto fail; }
	if (Q_UNLIKELY(!toBoolPtr(&ienv->dmAllowSubstDelivery, env.dmAllowSubstDelivery()))) { goto fail; }
	if (Q_UNLIKELY(!toCStrCopy(&ienv->dmType,
	                   (!env.dmType().isNull()) ? QString(env.dmType()) : QString()))) { goto fail; }

	if (Q_UNLIKELY(!toBoolPtr(&ienv->dmOVM, env.dmOVM()))) { goto fail; }
	if (Q_UNLIKELY(!toBoolPtr(&ienv->dmPublishOwnID, env.dmPublishOwnID()))) { goto fail; }

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return ienv;

fail:
	isds_envelope_free(&ienv);
	if (ok != Q_NULLPTR) {
		*ok = false;
	}
	return NULL;
}

/*!
 * @brief Converts file meta type.
 */
static
enum Isds::Type::FileMetaType libisdsFileMetaType2FileMetaType(
    isds_FileMetaType ifmt, bool *ok = Q_NULLPTR)
{
	bool iOk = true;
	enum Isds::Type::FileMetaType type = Isds::Type::FMT_UNKNOWN;

	switch (ifmt) {
	case FILEMETATYPE_MAIN: type = Isds::Type::FMT_MAIN; break;
	case FILEMETATYPE_ENCLOSURE: type = Isds::Type::FMT_ENCLOSURE; break;
	case FILEMETATYPE_SIGNATURE: type = Isds::Type::FMT_SIGNATURE; break;
	case FILEMETATYPE_META: type = Isds::Type::FMT_META; break;
	default:
		iOk = false;
		break;
	}

	if (ok != Q_NULLPTR) {
		*ok = iOk;
	}
	return type;
}

Isds::Document Isds::libisds2document(const struct isds_document *id, bool *ok)
{
	if (Q_UNLIKELY(id == NULL)) {
		Q_ASSERT(0);
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return Document();
	}

	bool iOk = false;
	Document doc;

	/* Does not support XML documents. */
	if (id->is_xml) {
		goto fail;
	}

	doc.setBinaryContent(QByteArray((const char *)id->data, id->data_length));
	doc.setMimeType(fromCStr(id->dmMimeType));
	doc.setFileMetaType(libisdsFileMetaType2FileMetaType(id->dmFileMetaType, &iOk));
	if (Q_UNLIKELY(!iOk)) {
		goto fail;
	}
	doc.setFileGuid(fromCStr(id->dmFileGuid));
	doc.setUpFileGuid(fromCStr(id->dmUpFileGuid));
	doc.setFileDescr(fromCStr(id->dmFileDescr));
	doc.setFormat(fromCStr(id->dmFormat));

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return doc;

fail:
	if (ok != Q_NULLPTR) {
		*ok = false;
	}
	return Document();
}

/*!
 * @brief Converts file meta type.
 */
static
isds_FileMetaType fileMetaType2libisdsFileMetaType(
    enum Isds::Type::FileMetaType fmt, bool *ok = Q_NULLPTR)
{
	bool iOk = true;
	isds_FileMetaType type = FILEMETATYPE_MAIN; /* FIXME ? */

	switch (fmt) {
	case Isds::Type::FMT_MAIN: type = FILEMETATYPE_MAIN; break;
	case Isds::Type::FMT_ENCLOSURE: type = FILEMETATYPE_ENCLOSURE; break;
	case Isds::Type::FMT_SIGNATURE: type = FILEMETATYPE_SIGNATURE; break;
	case Isds::Type::FMT_META: type = FILEMETATYPE_META; break;
	default:
		Q_ASSERT(0);
		iOk = false;
		break;
	}

	if (ok != Q_NULLPTR) {
		*ok = iOk;
	}
	return type;
}

struct isds_document *Isds::document2libisds(const Document &doc, bool *ok)
{
	if (Q_UNLIKELY(doc.isNull())) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return NULL;
	}

	struct isds_document *idoc =
	    (struct isds_document *)std::malloc(sizeof(*idoc));
	if (Q_UNLIKELY(idoc == NULL)) {
		Q_ASSERT(0);
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return NULL;
	}
	std::memset(idoc, 0, sizeof(*idoc));

	bool iOk = false;

	idoc->is_xml = doc.isXml();
	if (Q_UNLIKELY(idoc->is_xml)) {
		/* Does not support XML documents. */
		goto fail;
	}
	//idoc->xml_node_list = NULL;
	if (Q_UNLIKELY(!toCDataCopy(&idoc->data, &idoc->data_length,
	                   doc.binaryContent()))) { goto fail; }
	if (Q_UNLIKELY(!toCStrCopy(&idoc->dmMimeType, doc.mimeType()))) { goto fail; }
	idoc->dmFileMetaType = fileMetaType2libisdsFileMetaType(doc.fileMetaType(), &iOk);
	if (Q_UNLIKELY(!iOk)) { goto fail; }
	if (Q_UNLIKELY(!toCStrCopy(&idoc->dmFileGuid, doc.fileGuid()))) { goto fail; }
	if (Q_UNLIKELY(!toCStrCopy(&idoc->dmUpFileGuid, doc.upFileGuid()))) { goto fail; }
	if (Q_UNLIKELY(!toCStrCopy(&idoc->dmFileDescr, doc.fileDescr()))) { goto fail; }
	if (Q_UNLIKELY(!toCStrCopy(&idoc->dmFormat, doc.format()))) { goto fail; }

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return idoc;

fail:
	isds_document_free(&idoc);
	if (ok != Q_NULLPTR) {
		*ok = false;
	}
	return NULL;
}

/*!
 * @brief Converts raw type.
 */
static
enum Isds::Type::RawType libisdsRawType2RawType(isds_raw_type irt,
    bool *ok = Q_NULLPTR)
{
	bool iOk = true;
	enum Isds::Type::RawType type = Isds::Type::RT_UNKNOWN;

	switch (irt) {
	case RAWTYPE_INCOMING_MESSAGE: type = Isds::Type::RT_INCOMING_MESSAGE; break;
	case RAWTYPE_PLAIN_SIGNED_INCOMING_MESSAGE: type = Isds::Type::RT_PLAIN_SIGNED_INCOMING_MESSAGE; break;
	case RAWTYPE_CMS_SIGNED_INCOMING_MESSAGE: type = Isds::Type::RT_CMS_SIGNED_INCOMING_MESSAGE; break;
	case RAWTYPE_PLAIN_SIGNED_OUTGOING_MESSAGE: type = Isds::Type::RT_PLAIN_SIGNED_OUTGOING_MESSAGE; break;
	case RAWTYPE_CMS_SIGNED_OUTGOING_MESSAGE: type = Isds::Type::RT_CMS_SIGNED_OUTGOING_MESSAGE; break;
	case RAWTYPE_DELIVERYINFO: type = Isds::Type::RT_DELIVERYINFO; break;
	case RAWTYPE_PLAIN_SIGNED_DELIVERYINFO: type = Isds::Type::RT_PLAIN_SIGNED_DELIVERYINFO; break;
	case RAWTYPE_CMS_SIGNED_DELIVERYINFO: type = Isds::Type::RT_CMS_SIGNED_DELIVERYINFO; break;
	default:
		Q_ASSERT(0);
		iOk = false;
		break;
	}

	if (ok != Q_NULLPTR) {
		*ok = iOk;
	}
	return type;
}

/*!
 * @brief Converts document list.
 */
static
QList<Isds::Document> libisds2documentList(const struct isds_list *item,
    bool *ok = Q_NULLPTR)
{
	/* Document destructor function type. */
	typedef void (*doc_destr_func_t)(struct isds_document **);

	QList<Isds::Document> documentList;

	while (item != NULL) {
		const struct isds_document *id = (struct isds_document *)item->data;
		doc_destr_func_t idestr = (doc_destr_func_t)item->destructor;
		/* Destructor function must be set. */
		if (!IsdsInternal::crudeEqual(idestr, isds_document_free)) {
			Q_ASSERT(0);
			if (ok != Q_NULLPTR) {
				*ok = false;
			}
			return QList<Isds::Document>();
		}

		if (id != NULL) {
			bool iOk = false;
			documentList.append(Isds::libisds2document(id, &iOk));
			if (!iOk) {
				if (ok != Q_NULLPTR) {
					*ok = false;
				}
				return QList<Isds::Document>();
			}
		}

		item = item->next;
	}

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return documentList;
}

Isds::Message Isds::libisds2message(const struct isds_message *im, bool *ok)
{
	if (Q_UNLIKELY(im == NULL)) {
		Q_ASSERT(0);
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return Message();
	}

	bool iOk = false;
	Message m;

	m.setRaw(fromCData(im->raw, im->raw_length));
	m.setRawType(libisdsRawType2RawType(im->raw_type, &iOk));
	if (Q_UNLIKELY(!iOk)) {
		goto fail;
	}
	//m.setXml();
	m.setEnvelope(libisds2envelope(im->envelope, &iOk));
	if (Q_UNLIKELY(!iOk)) {
		goto fail;
	}
	m.setDocuments(libisds2documentList(im->documents, &iOk));
	if (Q_UNLIKELY(!iOk)) {
		goto fail;
	}

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return m;

fail:
	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return Message();
}

/*!
 * @brief Converts raw type.
 */
static
isds_raw_type rawType2libisdsRawType(enum Isds::Type::RawType rc,
    bool *ok = Q_NULLPTR)
{
	bool iOk = true;
	isds_raw_type type = RAWTYPE_INCOMING_MESSAGE; /* FIXME ? */

	switch (rc) {
	case Isds::Type::RT_INCOMING_MESSAGE: type = RAWTYPE_INCOMING_MESSAGE; break;
	case Isds::Type::RT_PLAIN_SIGNED_INCOMING_MESSAGE: type = RAWTYPE_PLAIN_SIGNED_INCOMING_MESSAGE; break;
	case Isds::Type::RT_CMS_SIGNED_INCOMING_MESSAGE: type = RAWTYPE_CMS_SIGNED_INCOMING_MESSAGE; break;
	case Isds::Type::RT_PLAIN_SIGNED_OUTGOING_MESSAGE: type = RAWTYPE_PLAIN_SIGNED_OUTGOING_MESSAGE; break;
	case Isds::Type::RT_CMS_SIGNED_OUTGOING_MESSAGE: type = RAWTYPE_CMS_SIGNED_OUTGOING_MESSAGE; break;
	case Isds::Type::RT_DELIVERYINFO: type = RAWTYPE_DELIVERYINFO; break;
	case Isds::Type::RT_PLAIN_SIGNED_DELIVERYINFO: type = RAWTYPE_PLAIN_SIGNED_DELIVERYINFO; break;
	case Isds::Type::RT_CMS_SIGNED_DELIVERYINFO: type = RAWTYPE_CMS_SIGNED_DELIVERYINFO; break;
	default:
		/*
		 * This code does not generate any error here, as the used
		 * values are ignored when taken as input of libisds.
		 */
		break;
	}

	if (ok != Q_NULLPTR) {
		*ok = iOk;
	}
	return type;
}

/*!
 * @brief Converts document list.
 */
static
struct isds_list *documentList2libisds(const QList<Isds::Document> &dl,
    bool *ok = Q_NULLPTR)
{
	if (Q_UNLIKELY(dl.isEmpty())) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return NULL;
	}

	struct isds_list *idl = NULL;
	struct isds_list *lastItem = NULL;
	foreach (const Isds::Document &doc, dl) {
		struct isds_list *item =
		    (struct isds_list *)std::malloc(sizeof(*item));
		if (Q_UNLIKELY(item == NULL)) {
			Q_ASSERT(0);
			goto fail;
		}
		std::memset(item, 0, sizeof(*item));

		bool iOk = false;
		struct isds_document *idoc = Isds::document2libisds(doc, &iOk);
		if (Q_UNLIKELY(!iOk)) {
			std::free(item); item = NULL;
			goto fail;
		}

		/* Set list item. */
		item->next = NULL;
		item->data = idoc;
		item->destructor = (void (*)(void **))isds_document_free;

		/* Append item. */
		if (lastItem == NULL) {
			idl = item;
		} else {
			lastItem->next = item;
		}
		lastItem = item;
	}

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return idl;

fail:
	isds_list_free(&idl);
	if (ok != Q_NULLPTR) {
		*ok = false;
	}
	return NULL;
}

struct isds_message *Isds::message2libisds(const Message &m, bool *ok)
{
	if (Q_UNLIKELY(m.isNull())) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return NULL;
	}

	struct isds_message *im =
	    (struct isds_message *)std::malloc(sizeof(*im));
	if (Q_UNLIKELY(im == NULL)) {
		Q_ASSERT(0);
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return NULL;
	}
	std::memset(im, 0, sizeof(*im));

	bool iOk = false;

	if (Q_UNLIKELY(!toCDataCopy(&im->raw, &im->raw_length, m.raw()))) { goto fail; }
	im->raw_type = rawType2libisdsRawType(m.rawType(), &iOk);
	if (Q_UNLIKELY(!iOk)) { goto fail; }
	//im->xml = NULL;
	im->envelope = envelope2libisds(m.envelope(), &iOk);
	if (Q_UNLIKELY(!iOk)) { goto fail; }
	im->documents = documentList2libisds(m.documents(), &iOk);
	if (Q_UNLIKELY(!iOk)) { goto fail; }

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return im;

fail:
	isds_message_free(&im);
	if (ok != Q_NULLPTR) {
		*ok = false;
	}
	return NULL;
}

QList<Isds::Message> Isds::libisds2messageList(const struct isds_list *item,
    bool *ok)
{
	/* Message destructor function type. */
	typedef void (*msg_destr_func_t)(struct isds_message **);

	QList<Message> messageList;

	while (item != NULL) {
		const struct isds_message *im = (struct isds_message *)item->data;
		msg_destr_func_t idestr = (msg_destr_func_t)item->destructor;
		/* Destructor function must be set. */
		if (!IsdsInternal::crudeEqual(idestr, isds_message_free)) {
			Q_ASSERT(0);
			if (ok != Q_NULLPTR) {
				*ok = false;
			}
			return QList<Message>();
		}

		if (im != NULL) {
			bool iOk = false;
			messageList.append(libisds2message(im, &iOk));
			if (!iOk) {
				if (ok != Q_NULLPTR) {
					*ok = false;
				}
				return QList<Message>();
			}
		}

		item = item->next;
	}

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return messageList;
}

struct isds_list *Isds::messageList2libisds(const QList<Message> &ml,
    bool *ok)
{
	if (Q_UNLIKELY(ml.isEmpty())) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return NULL;
	}

	struct isds_list *iml = NULL;
	struct isds_list *lastItem = NULL;
	foreach (const Message &msg, ml) {
		struct isds_list *item =
		    (struct isds_list *)std::malloc(sizeof(*item));
		if (Q_UNLIKELY(item == NULL)) {
			Q_ASSERT(0);
			goto fail;
		}
		std::memset(item, 0, sizeof(*item));

		bool iOk = false;
		struct isds_message *imsg = message2libisds(msg, &iOk);
		if (Q_UNLIKELY(!iOk)) {
			std::free(item); item = NULL;
			goto fail;
		}

		/* Set list item. */
		item->next = NULL;
		item->data = imsg;
		item->destructor = (void (*)(void **))isds_message_free;

		/* Append item. */
		if (lastItem == NULL) {
			iml = item;
		} else {
			lastItem->next = item;
		}
		lastItem = item;
	}

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return iml;

fail:
	isds_list_free(&iml);
	if (ok != Q_NULLPTR) {
		*ok = false;
	}
	return NULL;
}
