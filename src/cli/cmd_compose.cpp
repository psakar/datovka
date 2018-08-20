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
#include "src/datovka_shared/isds/internal_conversion.h"
#include "src/datovka_shared/log/log.h"

static const QString longOpt("compose");

static const QString dbIDRecipientStr("dbIDRecipient");
static const QString dmAnnotationStr("dmAnnotation");
static const QString dmToHandsStr("dmToHands");
static const QString dmRecipientRefNumberStr("dmRecipientRefNumber");
static const QString dmSenderRefNumberStr("dmSenderRefNumber");
static const QString dmRecipientIdentStr("dmRecipientIdent");
static const QString dmSenderIdentStr("dmSenderIdent");
static const QString dmLegalTitleLawStr("dmLegalTitleLaw");
static const QString dmLegalTitleYearStr("dmLegalTitleYear");
static const QString dmLegalTitleSectStr("dmLegalTitleSect");
static const QString dmLegalTitleParStr("dmLegalTitlePar");
static const QString dmLegalTitlePointStr("dmLegalTitlePoint");
static const QString dmPersonalDeliveryStr("dmPersonalDelivery");
static const QString dmAllowSubstDeliveryStr("dmAllowSubstDelivery");
//static const QString dmTypeStr("dmType"); /* payReplyCheckBox? */
//static const QString dmOVMStr("dmOVM");
static const QString dmPublishOwnIDStr("dmPublishOwnID");
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
	    : m_dbIDRecipient(), m_dmAnnotation(), m_dmToHands(),
	    m_dmRecipientRefNumber(), m_dmSenderRefNumber(),
	    m_dmRecipientIdent(), m_dmSenderIdent(), m_dmLegalTitleLaw(-1),
	    m_dmLegalTitleYear(-1), m_dmLegalTitleSect(), m_dmLegalTitlePar(),
	    m_dmLegalTitlePoint(), m_dmPersonalDelivery(Isds::Type::BOOL_NULL),
	    m_dmAllowSubstDelivery(Isds::Type::BOOL_NULL),
	    m_dmPublishOwnID(Isds::Type::BOOL_NULL), m_dmAttachment()
	{ }

	QStringList m_dbIDRecipient;
	QString m_dmAnnotation;
	QString m_dmToHands;
	QString m_dmRecipientRefNumber;
	QString m_dmSenderRefNumber;
	QString m_dmRecipientIdent;
	QString m_dmSenderIdent;
	qint64 m_dmLegalTitleLaw;
	qint64 m_dmLegalTitleYear;
	QString m_dmLegalTitleSect;
	QString m_dmLegalTitlePar;
	QString m_dmLegalTitlePoint;
	enum Isds::Type::NilBool m_dmPersonalDelivery;
	enum Isds::Type::NilBool m_dmAllowSubstDelivery;
	enum Isds::Type::NilBool m_dmPublishOwnID;
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
	return parser.addOption(QCommandLineOption(::longOpt,
	    tr("Brings up the send message dialogue window and fills in the supplied data."),
	    tr("message-options")));
}

bool CLI::CmdCompose::isSet(const QCommandLineParser &parser)
{
	return parser.isSet(::longOpt);
}

CLI::CmdCompose CLI::CmdCompose::value(const QCommandLineParser &parser)
{
	if (Q_UNLIKELY(!isSet(parser))) {
		return CmdCompose();
	}

	return deserialise(parser.value(::longOpt));
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

		if (::dbIDRecipientStr == pair.first) {
			if (!cmdCompose.dbIDRecipient().isEmpty()) {
				return CmdCompose(); /* Already set. */
			}
			cmdCompose.setDbIDRecipient(pair.second.split(QChar(';')));
		} else if (::dmAnnotationStr == pair.first) {
			if (!cmdCompose.dmAnnotation().isNull()) {
				return CmdCompose(); /* Already set. */
			}
			cmdCompose.setDmAnnotation(pair.second);
		} else if (::dmToHandsStr == pair.first) {
			if (!cmdCompose.dmToHands().isNull()) {
				return CmdCompose(); /* Already set. */
			}
			cmdCompose.setDmToHands(pair.second);
		} else if (::dmRecipientRefNumberStr == pair.first) {
			if (!cmdCompose.dmRecipientRefNumber().isNull()) {
				return CmdCompose(); /* Already set. */
			}
			cmdCompose.setDmRecipientRefNumber(pair.second);
		} else if (::dmSenderRefNumberStr == pair.first) {
			if (!cmdCompose.dmSenderRefNumber().isNull()) {
				return CmdCompose(); /* Already set. */
			}
			cmdCompose.setDmSenderRefNumber(pair.second);
		} else if (::dmRecipientIdentStr == pair.first) {
			if (!cmdCompose.dmRecipientIdent().isNull()) {
				return CmdCompose(); /* Already set. */
			}
			cmdCompose.setDmRecipientIdent(pair.second);
		} else if (::dmSenderIdentStr == pair.first) {
			if (!cmdCompose.dmSenderIdent().isNull()) {
				return CmdCompose(); /* Already set. */
			}
			cmdCompose.setDmSenderIdent(pair.second);
		} else if (::dmLegalTitleLawStr == pair.first) {
			if (!cmdCompose.dmLegalTitleLawStr().isNull() ||
			    !cmdCompose.setDmLegalTitleLawStr(pair.second)) {
				return CmdCompose(); /* Already set or not a number. */
			}
		} else if (::dmLegalTitleYearStr == pair.first) {
			if (!cmdCompose.dmLegalTitleYearStr().isNull() ||
			    !cmdCompose.setDmLegalTitleYearStr(pair.second)) {
				return CmdCompose(); /* Already set or not a number. */
			}
		} else if (::dmLegalTitleSectStr == pair.first) {
			if (!cmdCompose.dmLegalTitleSect().isNull()) {
				return CmdCompose(); /* Already set. */
			}
			cmdCompose.setDmLegalTitleSect(pair.second);
		} else if (::dmLegalTitleParStr == pair.first) {
			if (!cmdCompose.dmLegalTitlePar().isNull()) {
				return CmdCompose(); /* Already set. */
			}
			cmdCompose.setDmLegalTitlePar(pair.second);
		} else if (::dmLegalTitlePointStr == pair.first) {
			if (!cmdCompose.dmLegalTitlePoint().isNull()) {
				return CmdCompose(); /* Already set. */
			}
			cmdCompose.setDmLegalTitlePoint(pair.second);
		} else if (dmAttachmentStr == pair.first) {
			if (!cmdCompose.dmAttachment().isEmpty()) {
				return CmdCompose(); /* Already set. */
			}
			cmdCompose.setDmAttachment(pair.second.split(QChar(';')));
		} else if (::dmPersonalDeliveryStr == pair.first) {
			if (!cmdCompose.dmPersonalDeliveryStr().isNull() ||
			    !cmdCompose.setDmPersonalDeliveryStr(pair.second)) {
				return CmdCompose(); /* Already set or cannot be converted. */
			}
		} else if (::dmAllowSubstDeliveryStr == pair.first) {
			if (!cmdCompose.dmAllowSubstDeliveryStr().isNull() ||
			    !cmdCompose.setDmAllowSubstDeliveryStr(pair.second)) {
				return CmdCompose(); /* Already set or cannot be converted. */
			}
		} else if (::dmPublishOwnIDStr == pair.first) {
			if (!cmdCompose.dmPublishOwnIDStr().isNull() ||
			    !cmdCompose.setDmPublishOwnIDStr(pair.second)) {
				return CmdCompose(); /* Already set or cannot be converted. */
			}
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
		serialised.append(::dbIDRecipientStr % QStringLiteral("='") % dbIDRecipient().join(QChar(';')) % QStringLiteral("'"));
	}
	if (!dmAnnotation().isEmpty()) {
		serialised.append(::dmAnnotationStr % QStringLiteral("='") % dmAnnotation() % QStringLiteral("'"));
	}
	if (!dmToHands().isEmpty()) {
		serialised.append(::dmToHandsStr % QStringLiteral("='") % dmToHands() % QStringLiteral("'"));
	}
	if (!dmRecipientRefNumber().isEmpty()) {
		serialised.append(::dmRecipientRefNumberStr % QStringLiteral("='") % dmRecipientRefNumber() % QStringLiteral("'"));
	}
	if (!dmSenderRefNumber().isEmpty()) {
		serialised.append(::dmSenderRefNumberStr % QStringLiteral("='") % dmSenderRefNumber() % QStringLiteral("'"));
	}
	if (!dmRecipientIdent().isEmpty()) {
		serialised.append(::dmRecipientIdentStr % QStringLiteral("='") % dmRecipientIdent() % QStringLiteral("'"));
	}
	if (!dmSenderIdent().isEmpty()) {
		serialised.append(::dmSenderIdentStr % QStringLiteral("='") % dmSenderIdent() % QStringLiteral("'"));
	}
	if (!dmLegalTitleLawStr().isEmpty()) {
		serialised.append(::dmLegalTitleLawStr % QStringLiteral("='") % dmLegalTitleLawStr() % QStringLiteral("'"));
	}
	if (!dmLegalTitleYearStr().isEmpty()) {
		serialised.append(::dmLegalTitleYearStr % QStringLiteral("='") % dmLegalTitleYearStr() % QStringLiteral("'"));
	}
	if (!dmLegalTitleSect().isEmpty()) {
		serialised.append(::dmLegalTitleSectStr % QStringLiteral("='") % dmLegalTitleSect() % QStringLiteral("'"));
	}
	if (!dmLegalTitlePar().isEmpty()) {
		serialised.append(::dmLegalTitleParStr % QStringLiteral("='") % dmLegalTitlePar() % QStringLiteral("'"));
	}
	if (!dmLegalTitlePoint().isEmpty()) {
		serialised.append(::dmLegalTitlePointStr % QStringLiteral("='") % dmLegalTitlePoint() % QStringLiteral("'"));
	}
	if (!dmPersonalDeliveryStr().isEmpty()) {
		serialised.append(::dmPersonalDeliveryStr % QStringLiteral("='") % dmPersonalDeliveryStr() % QStringLiteral("'"));
	}
	if (!dmAllowSubstDeliveryStr().isEmpty()) {
		serialised.append(::dmAllowSubstDeliveryStr % QStringLiteral("='") % dmAllowSubstDeliveryStr() % QStringLiteral("'"));
	}
	if (!dmPublishOwnIDStr().isEmpty()) {
		serialised.append(::dmPublishOwnIDStr % QStringLiteral("='") % dmPublishOwnIDStr() % QStringLiteral("'"));
	}
	if (!dmAttachment().isEmpty()) {
		serialised.append(dmAttachmentStr % QStringLiteral("='") % dmAttachment().join(QChar(';')) % QStringLiteral("'"));
	}

	return serialised.join(QStringLiteral(","));
}

QString CLI::CmdCompose::dmLegalTitleLawStr(void) const
{
	return Isds::nonNegativeLong2String(dmLegalTitleLaw());
}

bool CLI::CmdCompose::setDmLegalTitleLawStr(const QString &l)
{
	bool ok = false;
	qint64 num = Isds::string2NonNegativeLong(l, &ok);
	if (!ok) {
		return false;
	}
	setDmLegalTitleLaw(num);
	return true;
}

QString CLI::CmdCompose::dmLegalTitleYearStr(void) const
{
	return Isds::nonNegativeLong2String(dmLegalTitleYear());
}

bool CLI::CmdCompose::setDmLegalTitleYearStr(const QString &y)
{
	bool ok = false;
	qint64 num = Isds::string2NonNegativeLong(y, &ok);
	if (!ok) {
		return false;
	}
	setDmLegalTitleYear(num);
	return true;
}

/*!
 * @brief Converts boolean type to string.
 *
 * @param[in] nilBool Boolean value.
 * @return String equivalent.
 */
static
const QString &nilBoolToString(enum Isds::Type::NilBool nilBool)
{
	static const QString zeroStr("0");
	static const QString oneStr("1");

	switch (nilBool) {
	case Isds::Type::BOOL_NULL:
		return nullString;
		break;
	case Isds::Type::BOOL_FALSE:
		return zeroStr;
		break;
	case Isds::Type::BOOL_TRUE:
		return oneStr;
		break;
	default:
		Q_ASSERT(0);
		return nullString;
		break;
	}
}

/*!
 * @brief Converts string to boolean type.
 *
 * @param[in]  str String containing a boolean value description.
 * @param[out] ok Set to true if value was successfully converted.
 * @return Boolean value.
 */
static
enum Isds::Type::NilBool stringToNilBool(const QString &str,
    bool *ok = Q_NULLPTR)
{
	if (str.isEmpty()) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return Isds::Type::BOOL_NULL;
	}

	const QString lower(str.toLower());
	if ((lower == QStringLiteral("0")) ||
	    (lower == QStringLiteral("false")) ||
	    (lower == QStringLiteral("no"))) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return Isds::Type::BOOL_FALSE;
	} else if ((lower == QStringLiteral("1")) ||
	    (lower == QStringLiteral("true")) ||
	    (lower == QStringLiteral("yes"))) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return Isds::Type::BOOL_TRUE;
	} else {
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return Isds::Type::BOOL_NULL;
	}
}

const QString &CLI::CmdCompose::dmPersonalDeliveryStr(void) const
{
	return nilBoolToString(dmPersonalDelivery());
}

bool CLI::CmdCompose::setDmPersonalDeliveryStr(const QString &pd)
{
	bool iOk = false;
	enum Isds::Type::NilBool nilBool = stringToNilBool(pd, &iOk);
	if (iOk) {
		setDmPersonalDelivery(nilBool);
	}
	return iOk;
}

const QString &CLI::CmdCompose::dmAllowSubstDeliveryStr(void) const
{
	return nilBoolToString(dmAllowSubstDelivery());
}

bool CLI::CmdCompose::setDmAllowSubstDeliveryStr(const QString &sd)
{
	bool iOk = false;
	enum Isds::Type::NilBool nilBool = stringToNilBool(sd, &iOk);
	if (iOk) {
		setDmAllowSubstDelivery(nilBool);
	}
	return iOk;
}

const QString &CLI::CmdCompose::dmPublishOwnIDStr(void) const
{
	return nilBoolToString(dmPublishOwnID());
}

bool CLI::CmdCompose::setDmPublishOwnIDStr(const QString &poi)
{
	bool iOk = false;
	enum Isds::Type::NilBool nilBool = stringToNilBool(poi, &iOk);
	if (iOk) {
		setDmPublishOwnID(nilBool);
	}
	return iOk;
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

const QString &CLI::CmdCompose::dmToHands(void) const
{
	Q_D(const CmdCompose);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dmToHands;
}

void CLI::CmdCompose::setDmToHands(const QString &th)
{
	ensureCmdComposePrivate();
	Q_D(CmdCompose);
	d->m_dmToHands = th;
}

#ifdef Q_COMPILER_RVALUE_REFS
void CLI::CmdCompose::setDmToHands(QString &&th)
{
	ensureCmdComposePrivate();
	Q_D(CmdCompose);
	d->m_dmToHands = th;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &CLI::CmdCompose::dmRecipientRefNumber(void) const
{
	Q_D(const CmdCompose);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dmRecipientRefNumber;
}

void CLI::CmdCompose::setDmRecipientRefNumber(const QString &rrn)
{
	ensureCmdComposePrivate();
	Q_D(CmdCompose);
	d->m_dmRecipientRefNumber = rrn;
}

#ifdef Q_COMPILER_RVALUE_REFS
void CLI::CmdCompose::setDmRecipientRefNumber(QString &&rrn)
{
	ensureCmdComposePrivate();
	Q_D(CmdCompose);
	d->m_dmRecipientRefNumber = rrn;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &CLI::CmdCompose::dmSenderRefNumber(void) const
{
	Q_D(const CmdCompose);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dmSenderRefNumber;
}

void CLI::CmdCompose::setDmSenderRefNumber(const QString &srn)
{
	ensureCmdComposePrivate();
	Q_D(CmdCompose);
	d->m_dmSenderRefNumber = srn;
}

#ifdef Q_COMPILER_RVALUE_REFS
void CLI::CmdCompose::setDmSenderRefNumber(QString &&srn)
{
	ensureCmdComposePrivate();
	Q_D(CmdCompose);
	d->m_dmSenderRefNumber = srn;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &CLI::CmdCompose::dmRecipientIdent(void) const
{
	Q_D(const CmdCompose);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dmRecipientIdent;
}

void CLI::CmdCompose::setDmRecipientIdent(const QString &ri)
{
	ensureCmdComposePrivate();
	Q_D(CmdCompose);
	d->m_dmRecipientIdent = ri;
}

#ifdef Q_COMPILER_RVALUE_REFS
void CLI::CmdCompose::setDmRecipientIdent(QString &&ri)
{
	ensureCmdComposePrivate();
	Q_D(CmdCompose);
	d->m_dmRecipientIdent = ri;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &CLI::CmdCompose::dmSenderIdent(void) const
{
	Q_D(const CmdCompose);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dmSenderIdent;
}

void CLI::CmdCompose::setDmSenderIdent(const QString &si)
{
	ensureCmdComposePrivate();
	Q_D(CmdCompose);
	d->m_dmSenderIdent = si;
}

#ifdef Q_COMPILER_RVALUE_REFS
void CLI::CmdCompose::setDmSenderIdent(QString &&si)
{
	ensureCmdComposePrivate();
	Q_D(CmdCompose);
	d->m_dmSenderIdent = si;
}
#endif /* Q_COMPILER_RVALUE_REFS */

qint64 CLI::CmdCompose::dmLegalTitleLaw(void) const
{
	Q_D(const CmdCompose);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return -1;
	}
	return d->m_dmLegalTitleLaw;
}

void CLI::CmdCompose::setDmLegalTitleLaw(qint64 l)
{
	ensureCmdComposePrivate();
	Q_D(CmdCompose);
	d->m_dmLegalTitleLaw = (l >= 0) ? l : -1;
}

qint64 CLI::CmdCompose::dmLegalTitleYear(void) const
{
	Q_D(const CmdCompose);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return -1;
	}
	return d->m_dmLegalTitleYear;
}

void CLI::CmdCompose::setDmLegalTitleYear(qint64 y)
{
	ensureCmdComposePrivate();
	Q_D(CmdCompose);
	d->m_dmLegalTitleYear = (y >= 0) ? y : -1;
}

const QString &CLI::CmdCompose::dmLegalTitleSect(void) const
{
	Q_D(const CmdCompose);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dmLegalTitleSect;
}

void CLI::CmdCompose::setDmLegalTitleSect(const QString &s)
{
	ensureCmdComposePrivate();
	Q_D(CmdCompose);
	d->m_dmLegalTitleSect = s;
}

#ifdef Q_COMPILER_RVALUE_REFS
void CLI::CmdCompose::setDmLegalTitleSect(QString &&s)
{
	ensureCmdComposePrivate();
	Q_D(CmdCompose);
	d->m_dmLegalTitleSect = s;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &CLI::CmdCompose::dmLegalTitlePar(void) const
{
	Q_D(const CmdCompose);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dmLegalTitlePar;
}

void CLI::CmdCompose::setDmLegalTitlePar(const QString &p)
{
	ensureCmdComposePrivate();
	Q_D(CmdCompose);
	d->m_dmLegalTitlePar = p;
}

#ifdef Q_COMPILER_RVALUE_REFS
void CLI::CmdCompose::setDmLegalTitlePar(QString &&p)
{
	ensureCmdComposePrivate();
	Q_D(CmdCompose);
	d->m_dmLegalTitlePar = p;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &CLI::CmdCompose::dmLegalTitlePoint(void) const
{
	Q_D(const CmdCompose);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}
	return d->m_dmLegalTitlePoint;
}

void CLI::CmdCompose::setDmLegalTitlePoint(const QString &p)
{
	ensureCmdComposePrivate();
	Q_D(CmdCompose);
	d->m_dmLegalTitlePoint = p;
}

#ifdef Q_COMPILER_RVALUE_REFS
void CLI::CmdCompose::setDmLegalTitlePoint(QString &&p)
{
	ensureCmdComposePrivate();
	Q_D(CmdCompose);
	d->m_dmLegalTitlePoint = p;
}
#endif /* Q_COMPILER_RVALUE_REFS */

enum Isds::Type::NilBool CLI::CmdCompose::dmPersonalDelivery(void) const
{
	Q_D(const CmdCompose);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return Isds::Type::BOOL_NULL;
	}
	return d->m_dmPersonalDelivery;
}

void CLI::CmdCompose::setDmPersonalDelivery(enum Isds::Type::NilBool pd)
{
	ensureCmdComposePrivate();
	Q_D(CmdCompose);
	d->m_dmPersonalDelivery = pd;
}

enum Isds::Type::NilBool CLI::CmdCompose::dmAllowSubstDelivery(void) const
{
	Q_D(const CmdCompose);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return Isds::Type::BOOL_NULL;
	}
	return d->m_dmAllowSubstDelivery;
}

void CLI::CmdCompose::setDmAllowSubstDelivery(enum Isds::Type::NilBool sd)
{
	ensureCmdComposePrivate();
	Q_D(CmdCompose);
	d->m_dmAllowSubstDelivery = sd;
}

enum Isds::Type::NilBool CLI::CmdCompose::dmPublishOwnID(void) const
{
	Q_D(const CmdCompose);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return Isds::Type::BOOL_NULL;
	}
	return d->m_dmPublishOwnID;
}

void CLI::CmdCompose::setDmPublishOwnID(enum Isds::Type::NilBool poi)
{
	ensureCmdComposePrivate();
	Q_D(CmdCompose);
	d->m_dmPublishOwnID = poi;
}

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
