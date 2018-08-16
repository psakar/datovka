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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
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

#include <QCommandLineParser>
#include <QString>
#include <QStringList>

#include "src/datovka_shared/isds/types.h"

namespace CLI {

	class CmdComposePrivate;

	/*!
	 * @brief Encapsulates the --compose CLI option and the arguments.
	 *
	 * @note The class is used to pass content to the send message dialogue
	 * from command line.
	 */
	class CmdCompose {
		Q_DECLARE_PRIVATE(CmdCompose)
		Q_DECLARE_TR_FUNCTIONS(CmdCompose)

	private:
		CmdCompose(void);

	public:
		CmdCompose(const CmdCompose &other);
#ifdef Q_COMPILER_RVALUE_REFS
		CmdCompose(CmdCompose &&other) Q_DECL_NOEXCEPT;
#endif /* Q_COMPILER_RVALUE_REFS */
		~CmdCompose(void);

		CmdCompose &operator=(const CmdCompose &other) Q_DECL_NOTHROW;
#ifdef Q_COMPILER_RVALUE_REFS
		CmdCompose &operator=(CmdCompose &&other) Q_DECL_NOTHROW;
#endif /* Q_COMPILER_RVALUE_REFS */

		friend void swap(CmdCompose &first, CmdCompose &second) Q_DECL_NOTHROW;

		bool isNull(void) const;

		static
		bool installParserOpt(QCommandLineParser &parser);

		static
		bool isSet(const QCommandLineParser &parser);

		/*!
		 * @brief Creates an object instance from command line options.
		 *
		 * @param[in] parser Command-line option parser.
		 * @return Non-null instance if all options recognised,
		 *     null instance on error.
		 */
		static
		CmdCompose value(const QCommandLineParser &parser);

		/*!
		 * @brief Creates an object from a string containing serialised
		 *     data.
		 *
		 * @param[in] content Serialised content. Follows same syntax
		 *                    as the command-line option argument.
		 * @return Null instance on any error.
		 */
		static
		CmdCompose deserialise(const QString &content);

		/*!
		 * @brief Dumps content into a string. The string follows
		 *     the same syntax as the command-line option argument.
		 *
		 * @return Serialised data.
		 */
		QString serialise(void) const;

		/*
		 * Convenience methods for number to string conversion.
		 */
		QString dmLegalTitleLawStr(void) const;
		bool setDmLegalTitleLawStr(const QString &l);
		QString dmLegalTitleYearStr(void) const;
		bool setDmLegalTitleYearStr(const QString &y);
		const QString &dmPersonalDeliveryStr(void) const;
		bool setDmPersonalDeliveryStr(const QString &pd);
		const QString &dmAllowSubstDeliveryStr(void) const;
		bool setDmAllowSubstDeliveryStr(const QString &sd);
		const QString &dmPublishOwnIDStr(void) const;
		bool setDmPublishOwnIDStr(const QString &poi);

		/* dbIDRecipient */
		const QStringList &dbIDRecipient(void) const;
		void setDbIDRecipient(const QStringList &rbil);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDbIDRecipient(QStringList &&rbil);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dmAnnotation */
		const QString &dmAnnotation(void) const;
		void setDmAnnotation(const QString &a);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDmAnnotation(QString &&a);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dmToHands -- optional */
		const QString &dmToHands(void) const;
		void setDmToHands(const QString &th);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDmToHands(QString &&th);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dmRecipientRefNumber */
		const QString &dmRecipientRefNumber(void) const;
		void setDmRecipientRefNumber(const QString &rrn);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDmRecipientRefNumber(QString &&rrn);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dmSenderRefNumber */
		const QString &dmSenderRefNumber(void) const;
		void setDmSenderRefNumber(const QString &srn);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDmSenderRefNumber(QString &&srn);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dmRecipientIdent */
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
		/* dmLegalTitlePoint */
		const QString &dmLegalTitlePoint(void) const;
		void setDmLegalTitlePoint(const QString &p);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDmLegalTitlePoint(QString &&p);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* dmPersonalDelivery */
		enum Isds::Type::NilBool dmPersonalDelivery(void) const;
		void setDmPersonalDelivery(enum Isds::Type::NilBool pd);
		/* dmAllowSubstDelivery */
		enum Isds::Type::NilBool dmAllowSubstDelivery(void) const;
		void setDmAllowSubstDelivery(enum Isds::Type::NilBool sd);
		/* dmPublishOwnID */
		enum Isds::Type::NilBool dmPublishOwnID(void) const;
		void setDmPublishOwnID(enum Isds::Type::NilBool poi);
		/* dmAttachment */
		const QStringList &dmAttachment(void) const;
		void setDmAttachment(const QStringList &al);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDmAttachment(QStringList &&al);
#endif /* Q_COMPILER_RVALUE_REFS */

	private:
		QScopedPointer<CmdComposePrivate> d_ptr; // std::unique_ptr ?
	};

	void swap(CmdCompose &first, CmdCompose &second) Q_DECL_NOTHROW;

}
