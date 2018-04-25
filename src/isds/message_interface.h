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
#include <QScopedPointer>
#include <QString>

#include "src/isds/types.h"

/*
 * https://stackoverflow.com/questions/25250171/how-to-use-the-qts-pimpl-idiom
 *
 * QScopedPtr -> std::unique_ptr ?
 */

/*
 * Structures originating from pril_2/WS_ISDS_Manipulace_s_datovymi_zpravami.pdf.
 */

namespace Isds {

	class HashPrivate;
	/*!
	 * @brief Described in dmBaseTypes.xsd as type tHash
	 */
	class Hash {
		Q_DECLARE_PRIVATE(Hash)

	public:
		Hash(void);
		Hash(const Hash &other);
#ifdef Q_COMPILER_RVALUE_REFS
		Hash(Hash &&other) Q_DECL_NOEXCEPT;
#endif /* Q_COMPILER_RVALUE_REFS */
		~Hash(void);

		Hash &operator=(const Hash &other) Q_DECL_NOTHROW;
#ifdef Q_COMPILER_RVALUE_REFS
		Hash &operator=(Hash &&other) Q_DECL_NOTHROW;
#endif /* Q_COMPILER_RVALUE_REFS */

		bool operator==(const Hash &other) const;

		friend void swap(Hash &first, Hash &second) Q_DECL_NOTHROW;

		bool isNull(void) const;

		/* algorithm */
		enum Type::HashAlg algorithm(void) const;
		void setAlgorithm(enum Type::HashAlg a);
		/* __item */
		const QByteArray &value(void) const;
		void setValue(const QByteArray &v);
#ifdef Q_COMPILER_RVALUE_REFS
		void setValue(QByteArray &&v);
#endif /* Q_COMPILER_RVALUE_REFS */

		friend Hash libisds2hash(const struct isds_hash *ih, bool *ok);

	private:
		QScopedPointer<HashPrivate> d_ptr; // std::unique_ptr ?
	};

	void swap(Hash &first, Hash &second) Q_DECL_NOTHROW;

	Hash libisds2hash(const struct isds_hash *ih, bool *ok = Q_NULLPTR);
	struct isds_hash *hash2libisds(const Hash &h, bool *ok = Q_NULLPTR);

	class EventPrivate;
	/*!
	 * @brief Described in dmBaseTypes.xsd as type tEvent.
	 */
	class Event {
		Q_DECLARE_PRIVATE(Event)

	public:
		Event(void);
		Event(const Event &other);
#ifdef Q_COMPILER_RVALUE_REFS
		Event(Event &&other) Q_DECL_NOEXCEPT;
#endif /* Q_COMPILER_RVALUE_REFS */
		~Event(void);

		Event &operator=(const Event &other) Q_DECL_NOTHROW;
#ifdef Q_COMPILER_RVALUE_REFS
		Event &operator=(Event &&other) Q_DECL_NOTHROW;
#endif /* Q_COMPILER_RVALUE_REFS */

		bool operator==(const Event &other) const;

		friend void swap(Event &first, Event &second) Q_DECL_NOTHROW;

		bool isNull(void) const;

		const QDateTime &time(void) const;
		void setTime(const QDateTime &t);
#ifdef Q_COMPILER_RVALUE_REFS
		void setTime(QDateTime &&t);
#endif /* Q_COMPILER_RVALUE_REFS */
		enum Type::Event type(void) const;
		//void setType(enum Type::Event e); /* Type is determined from description. */
		const QString &descr(void) const;
		void setDescr(const QString &descr);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDescr(QString &&descr);
#endif /* Q_COMPILER_RVALUE_REFS */

		friend Event libisds2event(const struct isds_event *ie,
		    bool *ok);

	private:
		QScopedPointer<EventPrivate> d_ptr; // std::unique_ptr ?
	};

	void swap(Event &first, Event &second) Q_DECL_NOTHROW;

	Event libisds2event(const struct isds_event *ie, bool *ok = Q_NULLPTR);
	struct isds_event *event2libisds(const Event &e, bool *ok = Q_NULLPTR);

	class EnvelopePrivate;
	/*!
	 * @brief Described in dmBaseTypes.xsd as group gMessageEnvelope
	 *     pril_2/WS_ISDS_Manipulace_s_datovymi_zpravami.pdf
	 *     section 2.1 (CreateMessage), section 2.4 (MessageDownload, SignedMessageDownload)
	 */
	class Envelope {
		Q_DECLARE_PRIVATE(Envelope)

	public:
		Envelope(void);
		Envelope(const Envelope &other);
#ifdef Q_COMPILER_RVALUE_REFS
		Envelope(Envelope &&other) Q_DECL_NOEXCEPT;
#endif /* Q_COMPILER_RVALUE_REFS */
		~Envelope(void);

		Envelope &operator=(const Envelope &other) Q_DECL_NOTHROW;
#ifdef Q_COMPILER_RVALUE_REFS
		Envelope &operator=(Envelope &&other) Q_DECL_NOTHROW;
#endif /* Q_COMPILER_RVALUE_REFS */

		bool operator==(const Envelope &other) const;

		friend void swap(Envelope &first, Envelope &second) Q_DECL_NOTHROW;

		bool isNull(void) const;

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
		const QString &dmID(void) const;
		void setDmID(const QString &id);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDmID(QString &&id);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dbIDSender -- sender box identifier, max. 7 characters */
		const QString &dbIDSender(void) const;
		void setDbIDSender(const QString &sbi);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDbIDSender(QString &&sbi);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dmSender -- sender name, max. 100 characters */
		const QString &dmSender(void) const;
		void setDmSender(const QString &sn);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDmSender(QString &&sn);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dmSenderAddress -- sender address, max. 100 characters */
		const QString &dmSenderAddress(void) const;
		void setDmSenderAddress(const QString &sa);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDmSenderAddress(QString &&sa);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dmSenderType -- roughly specified sender box type */
		enum Type::DbType dmSenderType(void) const;
		void setDmSenderType(enum Type::DbType st);
		/* dmRecipient -- recipient name, max. 100 characters */
		const QString &dmRecipient(void) const;
		void setDmRecipient(const QString &rn);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDmRecipient(QString &&rn);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dmRecipientAddress -- recipient address, max. 100 characters */
		const QString &dmRecipientAddress(void) const;
		void setDmRecipientAddress(const QString &ra);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDmRecipientAddress(QString &&ra);
#endif /* Q_COMPILER_RVALUE_REFS */
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
		const QDateTime &dmDeliveryTime(void) const;
		void setDmDeliveryTime(const QDateTime &dt);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDmDeliveryTime(QDateTime &&dt);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dmAcceptanceTime */
		const QDateTime &dmAcceptanceTime(void) const;
		void setDmAcceptanceTime(const QDateTime &at);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDmAcceptanceTime(QDateTime &&at);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dmHash */
		const Hash &dmHash(void) const;
		void setDmHash(const Hash &h);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDmHash(Hash &&h);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dmQTimestamp -- qualified time stamp, optional */
		const QByteArray &dmQTimestamp(void) const;
		void setDmQTimestamp(const QByteArray &ts);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDmQTimestamp(QByteArray &&ts);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dmEvents -- list of events the message has passed through. */
		const QList<Event> &dmEvents(void) const;
		void setDmEvents(const QList<Event> &el);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDmEvents(QList<Event> &&el);
#endif /* Q_COMPILER_RVALUE_REFS */

		/* dmSenderOrgUnit -- sender organisation unit, optional */
		const QString &dmSenderOrgUnit(void) const;
		void setDmSenderOrgUnit(const QString &sou);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDmSenderOrgUnit(QString &&sou);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dmSenderOrgUnitNum */
		qint64 dmSenderOrgUnitNum(void) const;
		void setDmSenderOrgUnitNum(qint64 soun);
		/* dbIDRecipient -- recipient box identifier, max. 7 characters, mandatory */
		const QString &dbIDRecipient(void) const;
		void setDbIDRecipient(const QString &rbi);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDbIDRecipient(QString &&rbi);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dmRecipientOrgUnit -- recipient organisation unit, optional */
		const QString &dmRecipientOrgUnit(void) const;
		void setDmRecipientOrgUnit(const QString &rou);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDmRecipientOrgUnit(QString &&rou);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dmRecipientOrgUnitNum */
		qint64 dmRecipientOrgUnitNum(void) const;
		void setDmRecipientOrgUnitNum(qint64 roun);
		/* dmToHands -- optional */
		const QString &dmToHands(void) const;
		void setDmToHands(const QString &th);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDmToHands(QString &&th);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dmAnnotation -- subject/title, mac 255 characters */
		const QString &dmAnnotation(void) const;
		void setDmAnnotation(const QString &a);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDmAnnotation(QString &&a);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dmRecipientRefNumber -- recipient reference identifier, max. 50 characters, optional */
		const QString &dmRecipientRefNumber(void) const;
		void setDmRecipientRefNumber(const QString &rrn);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDmRecipientRefNumber(QString &&rrn);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dmSenderRefNumber -- sender reference identifier, max. 50 characters, optional */
		const QString &dmSenderRefNumber(void) const;
		void setDmSenderRefNumber(const QString &srn);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDmSenderRefNumber(QString &&srn);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dmRecipientIdent -- CZ: spisova znacka, DE: Aktenzeichen, max. 50 characters, optional */
		const QString &dmRecipientIdent(void) const;
		void setDmRecipientIdent(const QString &ri);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDmRecipientIdent(QString &&ri);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dmSenderIdent */
		const QString &dmSenderIdent(void) const;
		void setDmSenderIdent(const QString &si);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDmSenderIdent(QString &&si);
#endif /* Q_COMPILER_RVALUE_REFS */

		/* Act addressing. */
		/* dmLegalTitleLaw */
		qint64 dmLegalTitleLaw(void) const;
		void setDmLegalTitleLaw(qint64 l);
		/* dmLegalTitleYear */
		qint64 dmLegalTitleYear(void) const;
		void setDmLegalTitleYear(qint64 y);
		/* dmLegalTitleSect */
		const QString &dmLegalTitleSect(void) const;
		void setDmLegalTitleSect(const QString &s);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDmLegalTitleSect(QString &&s);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dmLegalTitlePar */
		const QString &dmLegalTitlePar(void) const;
		void setDmLegalTitlePar(const QString &p);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDmLegalTitlePar(QString &&p);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dmLegalTitlePoint -- CZ: pismeno, DE: Buchstabe */
		const QString &dmLegalTitlePoint(void) const;
		void setDmLegalTitlePoint(const QString &p);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDmLegalTitlePoint(QString &&p);
#endif /* Q_COMPILER_RVALUE_REFS */
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
		void setDmOVM(enum Type::NilBool ovm);
		/* dmPublishOwnID */
		enum Type::NilBool dmPublishOwnID(void) const;
		void setDmPublishOwnID(enum Type::NilBool poi);

		/* Convenience conversion functions. */
		static
		enum Type::DmType char2DmType(QChar c);
		static
		QChar dmType2Char(enum Type::DmType t);

		friend Envelope libisds2envelope(const struct isds_envelope *ie,
		    bool *ok);

	private:
		QScopedPointer<EnvelopePrivate> d_ptr; // std::unique_ptr ?
	};

	void swap(Envelope &first, Envelope &second) Q_DECL_NOTHROW;

	Envelope libisds2envelope(const struct isds_envelope *ie,
	    bool *ok = Q_NULLPTR);
	struct isds_envelope *envelope2libisds(const Envelope &env,
	    bool *ok = Q_NULLPTR);

	class DocumentPrivate;
	/*!
	 * @brief Described in dmBaseTypes.xsd as type tFilesArray_dmFile.
	 *     pril_2/WS_ISDS_Manipulace_s_datovymi_zpravami.pdf
	 *     section 2.1 (CreateMessage).
	 */
	class Document {
		Q_DECLARE_PRIVATE(Document)

	public:
		Document(void);
		Document(const Document &other);
#ifdef Q_COMPILER_RVALUE_REFS
		Document(Document &&other) Q_DECL_NOEXCEPT;
#endif /* Q_COMPILER_RVALUE_REFS */
		~Document(void);

		Document &operator=(const Document &other) Q_DECL_NOTHROW;
#ifdef Q_COMPILER_RVALUE_REFS
		Document &operator=(Document &&other) Q_DECL_NOTHROW;
#endif /* Q_COMPILER_RVALUE_REFS */

		bool operator==(const Document &other) const;

		friend void swap(Document &first, Document &second) Q_DECL_NOTHROW;

		bool isNull(void) const;

		bool isXml(void) const; /* Inspired by libisds. */

		const QByteArray &binaryContent(void) const;
		void setBinaryContent(const QByteArray &bc);
#ifdef Q_COMPILER_RVALUE_REFS
		void setBinaryContent(QByteArray &&bc);
#endif /* Q_COMPILER_RVALUE_REFS */

		/* dmMimeType */
		const QString &mimeType(void) const;
		void setMimeType(const QString &mt);
#ifdef Q_COMPILER_RVALUE_REFS
		void setMimeType(QString &&mt);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dmFileMetaType */
		enum Type::FileMetaType fileMetaType(void) const;
		void setFileMetaType(enum Type::FileMetaType mt);
		/* dmFileGuid */
		const QString &fileGuid(void) const;
		void setFileGuid(const QString &g);
#ifdef Q_COMPILER_RVALUE_REFS
		void setFileGuid(QString &&g);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dmUpFileGuid */
		const QString &upFileGuid(void) const;
		void setUpFileGuid(const QString &ug);
#ifdef Q_COMPILER_RVALUE_REFS
		void setUpFileGuid(QString &&ug);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dmFileDescr */
		const QString &fileDescr(void) const;
		void setFileDescr(const QString &fd);
#ifdef Q_COMPILER_RVALUE_REFS
		void setFileDescr(QString &&fd);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dmFormat */
		const QString &format(void) const;
		void setFormat(const QString &f);
#ifdef Q_COMPILER_RVALUE_REFS
		void setFormat(QString &&f);
#endif /* Q_COMPILER_RVALUE_REFS */

		friend Document libisds2document(const struct isds_document *id,
		    bool *ok);

	private:
		QScopedPointer<DocumentPrivate> d_ptr; // std::unique_ptr ?
	};

	void swap(Document &first, Document &second) Q_DECL_NOTHROW;

	Document libisds2document(const struct isds_document *id,
	    bool *ok = Q_NULLPTR);
	struct isds_document *document2libisds(const Document &doc,
	    bool *ok = Q_NULLPTR);

	class MessagePrivate;
	/*!
	 * @Brief Described in dmBaseTypes.xsd as type tReturnedMessage.
	 *     pril_2/WS_ISDS_Manipulace_s_datovymi_zpravami.pdf
	 *     section 2.1 (CreateMessage).
	 */
	class Message {
		Q_DECLARE_PRIVATE(Message)

	public:
		Message(void);
		Message(const Message &other);
#ifdef Q_COMPILER_RVALUE_REFS
		Message(Message &&other) Q_DECL_NOEXCEPT;
#endif /* Q_COMPILER_RVALUE_REFS */
		~Message(void);

		Message &operator=(const Message &other) Q_DECL_NOTHROW;
#ifdef Q_COMPILER_RVALUE_REFS
		Message &operator=(Message &&other) Q_DECL_NOTHROW;
#endif /* Q_COMPILER_RVALUE_REFS */

		bool operator==(const Message &other) const;

		friend void swap(Message &first, Message &second) Q_DECL_NOTHROW;

		bool isNull(void) const;

		/* Raw message data. */
		const QByteArray &raw(void) const;
		void setRaw(const QByteArray &r);
#ifdef Q_COMPILER_RVALUE_REFS
		void setRaw(QByteArray &&r);
#endif /* Q_COMPILER_RVALUE_REFS */

		enum Type::RawType rawType(void) const;
		void setRawType(enum Type::RawType t);

		const Envelope &envelope(void) const;
		void setEnvelope(const Envelope &e);
#ifdef Q_COMPILER_RVALUE_REFS
		void setEnvelope(Envelope &&e);
#endif /* Q_COMPILER_RVALUE_REFS */

		const QList<Document> &documents(void) const;
		void setDocuments(const QList<Document> &dl);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDocuments(QList<Document> &&dl);
#endif /* Q_COMPILER_RVALUE_REFS */

		friend Message libisds2message(const struct isds_message *im,
		    bool *ok);

	private:
		QScopedPointer<MessagePrivate> d_ptr; // std::unique_ptr ?
	};

	void swap(Message &first, Message &second) Q_DECL_NOTHROW;

	Message libisds2message(const struct isds_message *im,
	    bool *ok = Q_NULLPTR);
	struct isds_message *message2libisds(const Message &m,
	    bool *ok = Q_NULLPTR);
}
