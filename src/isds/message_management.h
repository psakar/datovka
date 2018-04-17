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

#include <QChar>
#include <QDateTime>
#include <QByteArray>
#include <QList>
#include <QString>

/*
 * Structures originating from pril_2/WS_ISDS_Manipulace_s_datovymi_zpravami.pdf.
 */

namespace Isds {

	/*!
	 * @brief Described in dmBaseTypes.xsd as type tHash
	 */
	class Hash {
	public:
		Hash(void)
		    : m_alg(Type::HA_UNKNOWN), m_hash()
		{ }

		/* algorithm */
		enum Type::HashAlg algorithm(void) const;
		void setAlgorithm(enum Type::HashAlg a);
		/* __item */
		QByteArray value(void) const;
		void setValue(const QByteArray &v);

	private:
		enum Type::HashAlg m_alg;
		QByteArray m_hash;
	};

	/*!
	 * @brief Described in dmBaseTypes.xsd as type tEvent.
	 */
	class Event {
	public:
		Event(void)
		    : m_time(), m_type(Type::EV_UNKNOWN), m_descr()
		{ }

		QDateTime time(void) const;
		void setTime(const QDateTime &t);
		enum Type::Event type(void) const;
		//void setType(enum Type::Event e);
		QString descr(void) const;
		void setDescr(const QString &d);

	private:
		QDateTime m_time; /* dmEventTime */
		enum Type::Event m_type; /* Inspired by libisds. */
		QString m_descr; /* dmEventDescr */
	};

	/*!
	 * @brief Described in dmBaseTypes.xsd as group gMessageEnvelope
	 *     pril_2/WS_ISDS_Manipulace_s_datovymi_zpravami.pdf
	 *     section 2.1 (CreateMessage), section 2.4 (MessageDownload, SignedMessageDownload)
	 */
	class Envelope {
	public:
		Envelope(void);
		~Envelope(void);

		/*
		 * For convenience purposes. Message identifier consists only
		 * of digits, but documentation explicitly states that it is
		 * a max. 20 chars old string.
		 *
		 * Returns -1 if conversion to number fails.
		 */
		qint64 dmId(void) const;
		void setDmId(qint64 id);

		/* dmID */
		QString dmID(void) const;
		void setDmID(const QString &id);
		/* dbIDSender -- sender box identifier, max. 7 characters */
		QString dbIDSender(void) const;
		void setDbIDSender(const QString &sbi);
		/* dmSender -- sender name, max. 100 characters */
		QString dmSender(void) const;
		void setDmSender(const QString &sn);
		/* dmSenderAddress -- sender address, max. 100 characters */
		QString dmSenderAddress(void) const;
		void setDmSenderAddress(const QString &sa);
		/* dmSenderType -- roughly specified sender box type */
		enum Type::DbType dmSenderType(void) const;
		void setDmSenderType(enum Type::DbType st);
		/* dmRecipient -- recipient name, max. 100 characters */
		QString dmRecipient(void) const;
		void setDmRecipient(const QString &rn);
		/* dmRecipientAddress -- recipient address, max. 100 characters */
		QString dmRecipientAddress(void) const;
		void setDmRecipientAddress(const QString &ra);
		/* dmAmbiguousRecipient -- optional, recipient has elevated OVM role */
		enum Type::NilBool dmAmbiguousRecipient(void) const;
		void setDmAmbiguousRecipient(enum Type::NilBool ar);

		/* dmOrdinal -- defined in ns1__tRecord (dmBaseTypes.xsd), ordinal number in list of messages */
		quint64 dmOrdinal(void) const;
		void setDmOrdinal(quint64 o);
		/* dmMessageStatus -- message state */
		enum Type::DmState dmMessageStatus(void) const;
		void setDmMessageStatus(enum Type::DmState s);
		/* dmAttachmentSize -- rounded size in kB */
		qint64 dmAttachmentSize(void) const;
		void setDmAttachmentSize(qint64 as);
		/* dmDeliveryTime */
		QDateTime dmDeliveryTime(void) const;
		void setDmDeliveryTime(const QDateTime &dt);
		/* dmAcceptanceTime */
		QDateTime dmAcceptanceTime(void) const;
		void setDmAcceptanceTime(const QDateTime &at);
		/* dmHash */
		Hash dmHash(void) const;
		void setDmHash(const Hash &h);
		/* dmQTimestamp -- qualified time stamp, optional */
		QByteArray dmQTimestamp(void) const;
		void setDmQTimestamp(const QByteArray &ts);
		/* dmEvents -- list of events the message has passed through. */
		QList<Event> dmEvents(void) const;
		void setDmEvents(const QList<Event> &e);

		/* dmSenderOrgUnit -- sender organisation unit, optional */
		QString dmSenderOrgUnit(void) const;
		void setDmSenderOrgUnit(const QString &sou);
		/* dmSenderOrgUnitNum */
		qint64 dmSenderOrgUnitNum(void) const;
		void setDmSenderOrgUnitNum(qint64 soun);
		/* dbIDRecipient -- recipient box identifier, max. 7 characters, mandatory */
		QString dbIDRecipient(void) const;
		void setDbIDRecipient(const QString &rbi);
		/* dmRecipientOrgUnit -- recipient organisation unit, optional */
		QString dmRecipientOrgUnit(void) const;
		void setDmRecipientOrgUnit(const QString &rou);
		/* dmRecipientOrgUnitNum */
		qint64 dmRecipientOrgUnitNum(void) const;
		void setDmRecipientOrgUnitNum(qint64 &roun);
		/* dmToHands -- optional */
		QString dmToHands(void) const;
		void setDmToHands(const QString &th);
		/* dmAnnotation -- subject/title, mac 255 characters */
		QString dmAnnotation(void) const;
		void setDmAnnotation(const QString &a);
		/* dmRecipientRefNumber -- recipient reference identifier, max. 50 characters, optional */
		QString dmRecipientRefNumber(void) const;
		void setDmRecipientRefNumber(const QString &rrn);
		/* dmSenderRefNumber -- sender reference identifier, max. 50 characters, optional */
		QString dmSenderRefNumber(void) const;
		void setDmSenderRefNumber(const QString &srn);
		/* dmRecipientIdent -- CZ: spisova znacka, DE: Aktenzeichen, max. 50 characters, optional */
		QString dmRecipientIdent(void) const;
		void setDmRecipientIdent(const QString &ri);
		/* dmSenderIdent */
		QString dmSenderIdent(void) const;
		void setDmSenderIdent(const QString &si);

		/* Act addressing. */
		/* dmLegalTitleLaw */
		qint64 dmLegalTitleLaw(void) const;
		void setDmLegalTitleLaw(qint64 l);
		/* dmLegalTitleYear */
		qint64 dmLegalTitleYear(void) const;
		void setDmLegalTitleYear(qint64 y);
		/* dmLegalTitleSect */
		QString dmLegalTitleSect(void) const;
		void setDmLegalTitleSect(const QString &s);
		/* dmLegalTitlePar */
		QString dmLegalTitlePar(void) const;
		void setDmLegalTitlePar(const QString &p);
		/* dmLegalTitlePoint -- CZ: pismeno, DE: Buchstabe */
		QString dmLegalTitlePoint(void) const;
		void setDmLegalTitlePoint(const QString &p);
		/* dmPersonalDelivery -- only for recipient or person with explicitly granted privileges */
		enum Type::NilBool dmPersonalDelivery(void) const;
		void setDmPersonalDelivery(enum Type::NilBool pd);
		/* dmAllowSubstDelivery -- substitutionary delivery, only for some recipients, e.g. courts */
		enum Type::NilBool dmAllowSubstDelivery(void) const;
		void setDmAllowSubstDelivery(enum Type::NilBool sd);
		/* dmType -- message type */
		QChar dmType(void) const;
		void setDmType(QChar t);

		/* Outgoing messages only. */
		/* dmOVM */
		enum Type::NilBool dmOVM(void) const;
		void dmOVM(enum Type::NilBool ovm);
		/* dmPublishOwnID */
		enum Type::NilBool dmPublishOwnID(void) const;
		void setDmPublishOwnID(enum Type::NilBool poi);

		/* Convenience conversion functions. */
		static
		enum Type::DmType char2DmType(QChar c);
		static
		QChar dmType2Char(enum Type::DmType t);

	private:
		void *m_dataPtr;
	};

}
