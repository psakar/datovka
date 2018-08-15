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

namespace CLI {

	class CmdComposePrivate;

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

		/*
		 * @brief
		 */
		static
		CmdCompose value(const QCommandLineParser &parser);

		static
		CmdCompose deserialise(const QString &content);

		QString serialise(void) const;

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
