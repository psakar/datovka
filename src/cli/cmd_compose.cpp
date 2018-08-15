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

#include <QCommandLineOption>
#include <QStringBuilder>

#include "src/cli/cmd_compose.h"
#include "src/cli/cmd_tokeniser.h"
#include "src/datovka_shared/log/log.h"

static const QString longOpt("compose");

static const QString dbIDRecipientStr("dbIDRecipient");
static const QString dmAnnotationStr("dmAnnotation");
static const QString dmAttachmentStr("dmAttachment");

/* Null objects - for convenience. */
static const QString nullString;
static const QStringList emptyList;

/*!
 * @brief PIMPL CmdCompose class.
 */
class CLI::CmdComposePrivate {
	//Q_DISABLE_COPY(CmdComposePrivate)
public:
	CmdComposePrivate(void)
	    : m_dbIDRecipient(),
	    m_dmAnnotation(),
	    m_dmAttachment()
	{ }

	QStringList m_dbIDRecipient;
	QString m_dmAnnotation;
	QStringList m_dmAttachment;
};

CLI::CmdCompose::CmdCompose(void)
    : d_ptr(Q_NULLPTR)
{
}

CLI::CmdCompose::CmdCompose(const CmdCompose &other)
    : d_ptr((other.d_func() != Q_NULLPTR) ? (new (std::nothrow) CmdComposePrivate) : Q_NULLPTR)
{
	Q_D(CmdCompose);
	if (d == Q_NULLPTR) {
		return;
	}

	*d = *other.d_func();
}

#ifdef Q_COMPILER_RVALUE_REFS
CLI::CmdCompose::CmdCompose(CmdCompose &&other) Q_DECL_NOEXCEPT
    : d_ptr(other.d_ptr.take()) //d_ptr(std::move(other.d_ptr))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

CLI::CmdCompose::~CmdCompose(void)
{
}

/*!
 * @brief Ensures private compose command presence.
 *
 * @note Returns if private compose command could not be allocated.
 */
#define ensureCmdComposePrivate(_x_) \
	do { \
		if (Q_UNLIKELY(d_ptr == Q_NULLPTR)) { \
			CmdComposePrivate *p = new (std::nothrow) CmdComposePrivate; \
			if (Q_UNLIKELY(p == Q_NULLPTR)) { \
				Q_ASSERT(0); \
				return _x_; \
			} \
			d_ptr.reset(p); \
		} \
	} while (0)

CLI::CmdCompose &CLI::CmdCompose::operator=(const CmdCompose &other) Q_DECL_NOTHROW
{
	if (other.d_func() == Q_NULLPTR) {
		d_ptr.reset(Q_NULLPTR);
		return *this;
	}
	ensureCmdComposePrivate(*this);
	Q_D(CmdCompose);

	*d = *other.d_func();

	return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
CLI::CmdCompose &CLI::CmdCompose::operator=(CmdCompose &&other) Q_DECL_NOTHROW
{
	swap(*this, other);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */

bool CLI::CmdCompose::isNull(void) const
{
	Q_D(const CmdCompose);
	return d == Q_NULLPTR;
}

bool CLI::CmdCompose::installParserOpt(QCommandLineParser &parser)
{
	return parser.addOption(QCommandLineOption(longOpt,
	    tr("Brings up the create message window and fill in the supplied data."),
	    tr("message-options")));
}

bool CLI::CmdCompose::isSet(const QCommandLineParser &parser)
{
	return parser.isSet(longOpt);
}

CLI::CmdCompose CLI::CmdCompose::value(const QCommandLineParser &parser)
{
	if (Q_UNLIKELY(!isSet(parser))) {
		return CmdCompose();
	}

	return deserialise(parser.value(longOpt));
}

CLI::CmdCompose CLI::CmdCompose::deserialise(const QString &content)
{
	if (Q_UNLIKELY(content.isEmpty())) {
		return CmdCompose();
	}

	bool iOk = false;
	const QList< QPair<QString, QString> > opts(
	    tokeniseCmdOption(content, &iOk));
	if (!iOk) {
		return CmdCompose();
	}

	CmdCompose cmdCompose;

	typedef QPair<QString, QString> TokenPair;
	foreach (const TokenPair &pair, opts) {
		if (Q_UNLIKELY(pair.second.isEmpty())) {
			return CmdCompose();
		}

		if (dbIDRecipientStr == pair.first) {
			if (!cmdCompose.dbIDRecipient().isEmpty()) {
				return CmdCompose(); /* Already set. */
			}
			cmdCompose.setDbIDRecipient(pair.second.split(QChar(';')));
		} else if (dmAnnotationStr == pair.first) {
			if (!cmdCompose.dmAnnotation().isNull()) {
				return CmdCompose(); /* Already set. */
			}
			cmdCompose.setDmAnnotation(pair.second);
		} else if (dmAttachmentStr == pair.first) {
			if (!cmdCompose.dmAttachment().isEmpty()) {
				return CmdCompose(); /* Already set. */
			}
			cmdCompose.setDmAttachment(pair.second.split(QChar(';')));
		} else {
			logErrorNL("Unknown option '%s'.",
			    pair.first.toUtf8().constData());
			return CmdCompose();
		}
	}

	return cmdCompose;
}

QString CLI::CmdCompose::serialise(void) const
{
	QStringList serialised;

	if (!dbIDRecipient().isEmpty()) {
		serialised.append(dbIDRecipientStr % QStringLiteral("='") % dbIDRecipient().join(QChar(';')) % QStringLiteral("'"));
	}
	if (!dmAnnotation().isEmpty()) {
		serialised.append(dmAnnotationStr % QStringLiteral("='") % dmAnnotation() % QStringLiteral("'"));
	}
	if (!dmAttachment().isEmpty()) {
		serialised.append(dmAttachmentStr % QStringLiteral("='") % dmAttachment().join(QChar(';')) % QStringLiteral("'"));
	}

	return serialised.join(QStringLiteral(","));
}

const QStringList &CLI::CmdCompose::dbIDRecipient(void) const
{
	Q_D(const CmdCompose);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return emptyList;
	}
	return d->m_dbIDRecipient;
}

void CLI::CmdCompose::setDbIDRecipient(const QStringList &rbil)
{
	ensureCmdComposePrivate();
	Q_D(CmdCompose);
	d->m_dbIDRecipient = rbil;
}

#ifdef Q_COMPILER_RVALUE_REFS
void CLI::CmdCompose::setDbIDRecipient(QStringList &&rbil)
{
	ensureCmdComposePrivate();
	Q_D(CmdCompose);
	d->m_dbIDRecipient = rbil;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &CLI::CmdCompose::dmAnnotation(void) const
{
	Q_D(const CmdCompose);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dmAnnotation;
}

void CLI::CmdCompose::setDmAnnotation(const QString &a)
{
	ensureCmdComposePrivate();
	Q_D(CmdCompose);
	d->m_dmAnnotation = a;
}

#ifdef Q_COMPILER_RVALUE_REFS
void CLI::CmdCompose::setDmAnnotation(QString &&a)
{
	ensureCmdComposePrivate();
	Q_D(CmdCompose);
	d->m_dmAnnotation = a;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QStringList &CLI::CmdCompose::dmAttachment(void) const
{
	Q_D(const CmdCompose);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return emptyList;
	}
	return d->m_dmAttachment;
}

void CLI::CmdCompose::setDmAttachment(const QStringList &al)
{
	ensureCmdComposePrivate();
	Q_D(CmdCompose);
	d->m_dmAttachment = al;
}

#ifdef Q_COMPILER_RVALUE_REFS
void CLI::CmdCompose::setDmAttachment(QStringList &&al)
{
	ensureCmdComposePrivate();
	Q_D(CmdCompose);
	d->m_dmAttachment = al;
}
#endif /* Q_COMPILER_RVALUE_REFS */

void CLI::swap(CmdCompose &first, CmdCompose &second) Q_DECL_NOTHROW
{
	using std::swap;
	swap(first.d_ptr, second.d_ptr);
}
