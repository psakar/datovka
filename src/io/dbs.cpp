

#include <QDateTime>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

#include "dbs.h"


/* ========================================================================= */
/*
 * Converts db types strings.
 */
const QString & dbEntryTypeStr(dbEntryType entryType)
/* ========================================================================= */
{
	static const QString integer("INTEGER");
	static const QString text("TEXT");
	static const QString boolean("BOOLEAN");
	static const QString datetime("DATETIME");
	static const QString invalid;

	switch (entryType) {
	case DB_INTEGER:
		return integer;
		break;
	case DB_TEXT:
		return text;
		break;
	case DB_BOOLEAN:
		return boolean;
		break;
	case DB_DATETIME:
		return datetime;
		break;
	default:
		Q_ASSERT(0);
		return invalid;
		break;
	}
}


/* ========================================================================= */
/*
 * Constructor.
 */
Tbl::Tbl(const QString &name,
    const QVector< QPair<QString, dbEntryType> > &attrs,
    const QMap<QString, AttrProp> &props,
    const QMap<QString, QString> &colCons,
    const QString &tblCons)
    : tabName(name),
    knownAttrs(attrs),
    attrProps(props),
    colConstraints(colCons),
    tblConstraint(tblCons)
/* ========================================================================= */
{
}


/* ========================================================================= */
/*
 * Return true if table in database exists.
 */
bool Tbl::existsInDb(const QSqlDatabase &db) const
/* ========================================================================= */
{
	Q_ASSERT(db.isValid());
	if (!db.isValid()) {
		return false;
	}

	Q_ASSERT(db.isOpen());
	if (!db.isOpen()) {
		return false;
	}

	QSqlQuery query(db);
	QString queryStr = "SELECT "
	    "name"
	    " FROM sqlite_master WHERE "
	    "type = 'table'"
	    " and "
	    "name = '" + tabName + "'";
	//qDebug() << queryStr;
	query.prepare(queryStr);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		return true;
	}

	return false;
}


/* ========================================================================= */
/*
 * Create empty table in supplied database.
 */
bool Tbl::createEmpty(QSqlDatabase &db) const
/* ========================================================================= */
{
	Q_ASSERT(db.isValid());
	if (!db.isValid()) {
		return false;
	}

	Q_ASSERT(db.isOpen());
	if (!db.isOpen()) {
		return false;
	}

	QSqlQuery query(db);
	QString queryStr = "CREATE TABLE IF NOT EXISTS " + tabName + " (\n";
	for (int i = 0; i < knownAttrs.size(); ++i) {
		queryStr += "        " + knownAttrs[i].first + " " +
		    dbEntryTypeStr(knownAttrs[i].second);
		if (colConstraints.end() !=
		    colConstraints.find(knownAttrs[i].first)) {
			queryStr += " " +
			    colConstraints.value(knownAttrs[i].first);
		}
		if ((knownAttrs.size() - 1) != i) {
			queryStr += ",\n";
		}
	}
	queryStr += tblConstraint;
	queryStr += "\n)";
	//qDebug() << queryStr;
	query.prepare(queryStr);
	if (query.exec()) {
		return true;
	}

	return false;
}


const QMap<QString, QString> Tbl::emptyColConstraints;

const QString Tbl::emptyTblConstraint;


namespace MsgsTbl {
	const QString tabName("messages");

	const QVector< QPair<QString, dbEntryType> > knownAttrs = {
	{"dmID", DB_INTEGER}, /* NOT NULL */
	{"is_verified", DB_BOOLEAN},
	{"_origin", DB_TEXT},
	{"dbIDSender", DB_TEXT},
	{"dmSender", DB_TEXT},
	{"dmSenderAddress", DB_TEXT},
	{"dmSenderType", DB_INTEGER},
	{"dmRecipient", DB_TEXT},
	{"dmRecipientAddress", DB_TEXT},
	{"dmAmbiguousRecipient", DB_TEXT},
	{"dmSenderOrgUnit", DB_TEXT},
	{"dmSenderOrgUnitNum", DB_TEXT},
	{"dbIDRecipient", DB_TEXT},
	{"dmRecipientOrgUnit", DB_TEXT},
	{"dmRecipientOrgUnitNum", DB_TEXT},
	{"dmToHands", DB_TEXT},
	{"dmAnnotation", DB_TEXT},
	{"dmRecipientRefNumber", DB_TEXT},
	{"dmSenderRefNumber", DB_TEXT},
	{"dmRecipientIdent", DB_TEXT},
	{"dmSenderIdent", DB_TEXT},
	{"dmLegalTitleLaw", DB_TEXT},
	{"dmLegalTitleYear", DB_TEXT},
	{"dmLegalTitleSect", DB_TEXT},
	{"dmLegalTitlePar", DB_TEXT},
	{"dmLegalTitlePoint", DB_TEXT},
	{"dmPersonalDelivery", DB_BOOLEAN},
	{"dmAllowSubstDelivery", DB_BOOLEAN},
	{"dmQTimestamp", DB_TEXT},
	{"dmDeliveryTime", DB_DATETIME},
	{"dmAcceptanceTime", DB_DATETIME},
	{"dmMessageStatus", DB_INTEGER},
	{"dmAttachmentSize", DB_INTEGER},
	{"_dmType", DB_TEXT}
	/*
	 * PRIMARY KEY ("dmID"),
	 * CHECK (is_verified IN (0, 1)),
	 * CHECK ("dmPersonalDelivery" IN (0, 1)),
	 * CHECK ("dmAllowSubstDelivery" IN (0, 1))
	*/
	};

	const QMap<QString, QString> colConstraints = {
	    {"dmID", "NOT NULL"}
	};

	const QString tblConstraint(
	    ",\n"
	    "        PRIMARY KEY (dmID),\n"
	    "        CHECK (is_verified IN (0, 1)),\n"
	    "        CHECK (dmPersonalDelivery IN (0, 1)),\n"
	    "        CHECK (dmAllowSubstDelivery IN (0, 1))"
	);

	const QMap<QString, AttrProp> attrProps = {
	{"dmID",                  {DB_INTEGER, QObject::tr("ID")}},
	{"is_verified",           {DB_BOOLEAN, ""}},
	{"_origin",               {DB_TEXT, ""}},
	{"dbIDSender",            {DB_TEXT, ""}},
	{"dmSender",              {DB_TEXT, QObject::tr("Sender")}},
	{"dmSenderAddress",       {DB_TEXT, QObject::tr("Sender address")}},
	{"dmSenderType",          {DB_INTEGER, ""}},
	{"dmRecipient",           {DB_TEXT, QObject::tr("Recipient")}},
	{"dmRecipientAddress",    {DB_TEXT, QObject::tr("Recipient address")}},
	{"dmAmbiguousRecipient",  {DB_TEXT, ""}},
	{"dmSenderOrgUnit",       {DB_TEXT, ""}},
	{"dmSenderOrgUnitNum",    {DB_TEXT, ""}},
	{"dbIDRecipient",         {DB_TEXT, ""}},
	{"dmRecipientOrgUnit",    {DB_TEXT, ""}},
	{"dmRecipientOrgUnitNum", {DB_TEXT, ""}},
	{"dmToHands",             {DB_TEXT, QObject::tr("To hands")}},
	{"dmAnnotation",          {DB_TEXT, QObject::tr("Title")}},
	{"dmRecipientRefNumber",  {DB_TEXT, QObject::tr("Your reference number")}},
	{"dmSenderRefNumber",     {DB_TEXT, QObject::tr("Our reference number")}},
	{"dmRecipientIdent",      {DB_TEXT, QObject::tr("Your file mark")}},
	{"dmSenderIdent",         {DB_TEXT, QObject::tr("Our file mark")}},
	{"dmLegalTitleLaw",       {DB_TEXT, QObject::tr("Law")}},
	{"dmLegalTitleYear",      {DB_TEXT, QObject::tr("Year")}},
	{"dmLegalTitleSect",      {DB_TEXT, QObject::tr("Selection")}},
	{"dmLegalTitlePar",       {DB_TEXT, QObject::tr("Paragraph")}},
	{"dmLegalTitlePoint",     {DB_TEXT, QObject::tr("Letter")}},
	{"dmPersonalDelivery",    {DB_BOOLEAN, ""}},
	{"dmAllowSubstDelivery",  {DB_BOOLEAN, ""}},
	{"dmQTimestamp",          {DB_TEXT, ""}},
	{"dmDeliveryTime",        {DB_DATETIME, QObject::tr("Delivered")}},
	{"dmAcceptanceTime",      {DB_DATETIME, QObject::tr("Accepted")}},
	{"dmMessageStatus",       {DB_INTEGER, QObject::tr("Status")}},
	{"dmAttachmentSize",      {DB_INTEGER, QObject::tr("Attachment size")}},
	{"_dmType",               {DB_TEXT, ""}}
	};
} /* namespace MsgsTbl */
const Tbl msgsTbl(MsgsTbl::tabName, MsgsTbl::knownAttrs, MsgsTbl::attrProps,
    MsgsTbl::colConstraints, MsgsTbl::tblConstraint);


namespace FlsTbl {
	const QString tabName("files");

	const QVector< QPair<QString, dbEntryType> > knownAttrs = {
	{"id", DB_INTEGER}, /* NOT NULL*/
	{"message_id",  DB_INTEGER},
	{"_dmFileDescr", DB_TEXT},
	{"_dmUpFileGuid", DB_TEXT},
	{"_dmFileGuid", DB_TEXT},
	{"_dmMimeType", DB_TEXT},
	{"_dmFormat", DB_TEXT},
	{"_dmFileMetaType", DB_TEXT},
	{"dmEncodedContent", DB_TEXT}
	/*
	 * PRIMARY KEY (id),
	 * FOREIGN KEY(message_id) REFERENCES messages ("dmID")
	 */
	};

	const QMap<QString, QString> colConstraints = {
	    {"id", "NOT NULL"}
	};

	const QString &tblConstraint(
	    ",\n"
	    "        PRIMARY KEY (id),\n"
	    "        FOREIGN KEY(message_id) REFERENCES messages (dmID)"
	);

	const QMap<QString, AttrProp> attrProps = {
	{"id",               {DB_INTEGER, ""}},
	{"message_id",       {DB_INTEGER, ""}},
	{"_dmFileDescr",     {DB_TEXT, ""}},
	{"_dmUpFileGuid",    {DB_TEXT, ""}},
	{"_dmFileGuid",      {DB_TEXT, ""}},
	{"_dmMimeType",      {DB_TEXT, ""}},
	{"_dmFormat",        {DB_TEXT, ""}},
	{"_dmFileMetaType",  {DB_TEXT, ""}},
	{"dmEncodedContent", {DB_TEXT, ""}}
	};
} /* namespace FlsTbl */
const Tbl flsTbl(FlsTbl::tabName, FlsTbl::knownAttrs, FlsTbl::attrProps,
    FlsTbl::colConstraints, FlsTbl::tblConstraint);


namespace HshsTbl {
	const QString tabName("hashes");

	const QVector< QPair<QString, dbEntryType> > knownAttrs = {
	{"id", DB_INTEGER}, /* NOT NULL */
	{"message_id", DB_INTEGER},
	{"value", DB_TEXT},
	{"_algorithm", DB_TEXT}
	/*
	 * PRIMARY KEY (id),
	 * FOREIGN KEY(message_id) REFERENCES messages ("dmID")
	 */
	};

	const QMap<QString, QString> colConstraints = {
	    {"id", "NOT NULL"}
	};

	const QString &tblConstraint(
	    ",\n"
	    "        PRIMARY KEY (id),\n"
	    "        FOREIGN KEY(message_id) REFERENCES messages (dmID)"
	);

	const QMap<QString, AttrProp> attrProps = {
	{"id",         {DB_INTEGER, ""}},
	{"message_id", {DB_INTEGER, ""}},
	{"value",      {DB_TEXT, ""}},
	{"_algorithm", {DB_TEXT, ""}}
	};
} /* namespace HshsTbl */
const Tbl hshsTbl(HshsTbl::tabName, HshsTbl::knownAttrs, HshsTbl::attrProps,
    HshsTbl::colConstraints, HshsTbl::tblConstraint);


namespace EvntsTbl {
	const QString tabName("events");

	const QVector< QPair<QString, dbEntryType> > knownAttrs = {
	{"id", DB_INTEGER}, /* NOT NULL */
	{"message_id", DB_INTEGER},
	{"dmEventTime", DB_TEXT},
	{"dmEventDescr", DB_TEXT}
	/*
	 * PRIMARY KEY (id),
	 * FOREIGN KEY(message_id) REFERENCES messages ("dmID")
	 */
	};

	const QMap<QString, QString> colConstraints = {
	    {"id", "NOT NULL"}
	};

	const QString &tblConstraint(
	    ",\n"
	    "        PRIMARY KEY (id),\n"
	    "        FOREIGN KEY(message_id) REFERENCES messages (dmID)"
	);

	const QMap<QString, AttrProp> attrProps = {
	{"id",           {DB_INTEGER, ""}},
	{"message_id",   {DB_INTEGER, ""}},
	{"dmEventTime",  {DB_TEXT, ""}},
	{"dmEventDescr", {DB_TEXT, ""}}
	};
} /* namespace EvntsTbl */
const Tbl evntsTbl(EvntsTbl::tabName, EvntsTbl::knownAttrs,
    EvntsTbl::attrProps, EvntsTbl::colConstraints, EvntsTbl::tblConstraint);


namespace RwmsgdtTbl {
	const QString tabName("raw_message_data");

	const QVector< QPair<QString, dbEntryType> > knownAttrs = {
	{"message_id", DB_INTEGER}, /* NOT NULL */
	{"message_type", DB_INTEGER},
	{"data", DB_TEXT}
	/*
	 * PRIMARY KEY (message_id),
	 * FOREIGN KEY(message_id) REFERENCES messages ("dmID")
	 */
	};

	const QMap<QString, QString> colConstraints = {
	    {"message_id", "NOT NULL"}
	};

	const QString &tblConstraint(
	    ",\n"
	    "        PRIMARY KEY (message_id),\n"
	    "        FOREIGN KEY(message_id) REFERENCES messages (dmID)"
	);

	const QMap<QString, AttrProp> attrProps = {
	{"message_id",   {DB_INTEGER, ""}},
	{"message_type", {DB_INTEGER, ""}},
	{"data",         {DB_TEXT, ""}}
	};
} /* namespace RwmsgdtTbl */
const Tbl rwmsgdtTbl(RwmsgdtTbl::tabName, RwmsgdtTbl::knownAttrs,
    RwmsgdtTbl::attrProps, RwmsgdtTbl::colConstraints,
    RwmsgdtTbl::tblConstraint);


namespace RwdlvrinfdtTbl {
	const QString tabName("raw_delivery_info_data");

	const QVector< QPair<QString, dbEntryType> > knownAttrs = {
	{"message_id", DB_INTEGER}, /* NOT NULL */
	{"data", DB_TEXT}
	/*
	 * PRIMARY KEY (message_id),
	 * FOREIGN KEY(message_id) REFERENCES messages ("dmID")
	 */
	};

	const QMap<QString, QString> colConstraints = {
	    {"message_id", "NOT NULL"}
	};

	const QString &tblConstraint(
	    ",\n"
	    "        PRIMARY KEY (message_id),\n"
	    "        FOREIGN KEY(message_id) REFERENCES messages (dmID)"
	);

	const QMap<QString, AttrProp> attrProps = {
	{"message_id", {DB_INTEGER, ""}},
	{"data",       {DB_TEXT, ""}}
	};
} /* namespace RwdlvrinfdtTbl */
const Tbl rwdlvrinfdtTbl(RwdlvrinfdtTbl::tabName, RwdlvrinfdtTbl::knownAttrs,
    RwdlvrinfdtTbl::attrProps, RwdlvrinfdtTbl::colConstraints,
    RwdlvrinfdtTbl::tblConstraint);


namespace SmsgdtTbl {
	const QString tabName("supplementary_message_data");

	const QVector< QPair<QString, dbEntryType> > knownAttrs = {
	{"message_id", DB_INTEGER}, /* NOT NULL */
	{"message_type", DB_INTEGER},
	{"read_locally", DB_BOOLEAN},
	{"download_date", DB_DATETIME},
	{"custom_data", DB_TEXT}
	/*
	 * PRIMARY KEY (message_id),
	 * FOREIGN KEY(message_id) REFERENCES messages ("dmID"),
	 * CHECK (read_locally IN (0, 1))
	 */
	};

	const QMap<QString, QString> colConstraints = {
	    {"message_id", "NOT NULL"}
	};

	const QString &tblConstraint(
	    ",\n"
	    "        PRIMARY KEY (message_id),\n"
	    "        FOREIGN KEY(message_id) REFERENCES messages (dmID),\n"
	    "        CHECK (read_locally IN (0, 1))"
	);

	const QMap<QString, AttrProp> attrProps = {
	{"message_id",    {DB_INTEGER, ""}},
	{"message_type",  {DB_INTEGER, ""}},
	{"read_locally",  {DB_BOOLEAN, ""}},
	{"download_date", {DB_DATETIME, ""}},
	{"custom_data",   {DB_TEXT, ""}}
	};
} /* namespace SmsgdtTbl */
const Tbl smsgdtTbl(SmsgdtTbl::tabName, SmsgdtTbl::knownAttrs,
    SmsgdtTbl::attrProps, SmsgdtTbl::colConstraints, SmsgdtTbl::tblConstraint);


namespace CrtdtTbl {
	const QString tabName("certificate_data");

	const QVector< QPair<QString, dbEntryType> > knownAttrs = {
	{"id", DB_INTEGER}, /* NOT NULL */
	{"der_data", DB_TEXT}
	/*
	 * PRIMARY KEY (id),
	 * UNIQUE (der_data)
	 */
	};

	const QMap<QString, QString> colConstraints = {
	    {"id", "NOT NULL"}
	};

	const QString &tblConstraint(
	    ",\n"
	    "        PRIMARY KEY (id),\n"
	    "        UNIQUE (der_data)"
	);

	const QMap<QString, AttrProp> attrProps = {
	{"id",       {DB_INTEGER, ""}},
	{"der_data", {DB_TEXT, ""}}
	};
} /* namespace CrtdtTbl */
const Tbl crtdtTbl(CrtdtTbl::tabName, CrtdtTbl::knownAttrs,
    CrtdtTbl::attrProps, CrtdtTbl::colConstraints, CrtdtTbl::tblConstraint);


namespace MsgcrtdtTbl {
	const QString tabName("message_certificate_data");

	const QVector< QPair<QString, dbEntryType> > knownAttrs = {
	{"message_id", DB_INTEGER},
	{"certificate_id", DB_INTEGER}
	/*
	 * FOREIGN KEY(message_id) REFERENCES messages ("dmID"),
	 * FOREIGN KEY(certificate_id) REFERENCES certificate_data (id)
	 */
	};

	const QMap<QString, QString> colConstraints; /* Empty. */

	const QString &tblConstraint(
	    ",\n"
	    "        FOREIGN KEY(message_id) REFERENCES messages (dmID),\n"
	    "        FOREIGN KEY(certificate_id) REFERENCES certificate_data (id)"
	);

	const QMap<QString, AttrProp> attrProps = {
	{"message_id",     {DB_INTEGER, ""}},
	{"certificate_id", {DB_INTEGER, ""}}
	};
} /* namespace MsgcrtdtTbl */
const Tbl msgcrtdtTbl(MsgcrtdtTbl::tabName, MsgcrtdtTbl::knownAttrs,
    MsgcrtdtTbl::attrProps, MsgcrtdtTbl::colConstraints,
    MsgcrtdtTbl::tblConstraint);


/*!
 * @brief Date/time format stored in db.
 *
 * @note Old implementation of datovka is likely to contain a bug.
 * Milliseconds are probably stored as microseconds.
 */
static
const QString dbFaultyDateTimeFormat("yyyy-MM-dd HH:mm:ss.000zzz");
static
const QString dbDateTimeFormat("yyyy-MM-dd HH:mm:ss.zzz");


/* ========================================================================= */
/*
 * Converts date from database format into desired format if possible.
 */
QDateTime dateTimeFromDbFormat(const QString &dateTimeDbStr)
/* ========================================================================= */
{
	QDateTime dateTime = QDateTime::fromString(dateTimeDbStr,
	    dbDateTimeFormat);
	if (dateTime.isNull() || !dateTime.isValid()) {
		/* Try the faulty format. */
		dateTime = QDateTime::fromString(dateTimeDbStr,
		    dbFaultyDateTimeFormat);
	}
	if (dateTime.isNull() || !dateTime.isValid()) {
		/* Try to ignore 3 rightmost characters. */
		dateTime = QDateTime::fromString(
		    dateTimeDbStr.left(dateTimeDbStr.size() - 3),
		    dbDateTimeFormat);
	}

	return dateTime;
}


/* ========================================================================= */
/*
 * Converts date from database format into desired format if possible.
 */
QString dateTimeStrFromDbFormat(const QString &dateTimeDbStr,
    const QString &tgtFmt)
/* ========================================================================= */
{
	QDateTime dateTime = dateTimeFromDbFormat(dateTimeDbStr);

	if (dateTime.isValid()) {
		return dateTime.toString(tgtFmt);
	} else {
		return QString();
	}
}
