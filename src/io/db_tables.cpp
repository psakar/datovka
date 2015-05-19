/*
 * Copyright (C) 2014-2015 CZ.NIC
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


#include <QDebug>
#include <QMap>
#include <QObject>
#include <QPair>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QVariant>
#include <QVector>

#include "db_tables.h"


/* ========================================================================= */
/*
 * Converts db types strings.
 */
const QString & entryTypeStr(EntryType entryType)
/* ========================================================================= */
{
	static const QString integer("INTEGER");
	static const QString text("TEXT");
	static const QString boolean("BOOLEAN");
	static const QString datetime("DATETIME");
	static const QString date("DATE");
	static const QString invalid;

	switch (entryType) {
	case DB_INTEGER:
	case DB_INT_PROCESSING_STATE:
		return integer;
		break;
	case DB_TEXT:
		return text;
		break;
	case DB_BOOLEAN:
	case DB_BOOL_READ_LOCALLY:
	case DB_BOOL_ATTACHMENT_DOWNLOADED:
		return boolean;
		break;
	case DB_DATETIME:
		return datetime;
		break;
	case DB_DATE:
		return date;
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
    const QVector< QPair<QString, EntryType> > &attrs,
    const QMap<QString, AttrProp> &props,
    const QMap<QString, QString> &colCons,
    const QString &tblCons)
    : tabName(name),
    knownAttrs(attrs),
    attrPropsSources(props),
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
	    "name = :tabName";
	//qDebug() << queryStr;
	if (!query.prepare(queryStr)) {
		return false;
	}
	query.bindValue(":tabName", tabName);
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
		    entryTypeStr(knownAttrs[i].second);
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
	if (!query.prepare(queryStr) || !query.exec()) {
		qWarning() << query.lastError();
		return false;
	}

	return true;
}


/* ========================================================================= */
/*
 * Reloads the translation according to the selected
 *     localisation.
 */
void Tbl::reloadLocalisedDescription(void)
/* ========================================================================= */
{
	attrProps.clear();

	QMap<QString, AttrProp>::const_iterator it = attrPropsSources.begin();

	class AttrProp prop;
	for (; it != attrPropsSources.end(); ++it) {
		prop.type = it.value().type;
		prop.desc = QObject::tr(it.value().desc.toUtf8().constData());
		attrProps.insert(it.key(), prop);
	}
}


const QMap<QString, QString> Tbl::emptyColConstraints;

const QString Tbl::emptyTblConstraint;


namespace AccntinfTbl {
	const QString tabName("account_info");

	const QVector< QPair<QString, EntryType> > knownAttrs = {
	{"key", DB_TEXT}, /* NOT NULL */
	{"dbID", DB_TEXT}, /* NOT NULL */
	{"dbType", DB_TEXT},
	{"ic", DB_INTEGER},
	{"pnFirstName", DB_TEXT},
	{"pnMiddleName", DB_TEXT},
	{"pnLastName", DB_TEXT},
	{"pnLastNameAtBirth", DB_TEXT},
	{"firmName", DB_TEXT},
	{"biDate", DB_DATE},
	{"biCity", DB_TEXT},
	{"biCounty", DB_TEXT},
	{"biState", DB_TEXT},
	{"adCity", DB_TEXT},
	{"adStreet", DB_TEXT},
	{"adNumberInStreet", DB_TEXT},
	{"adNumberInMunicipality", DB_TEXT},
	{"adZipCode", DB_TEXT},
	{"adState", DB_TEXT},
	{"nationality", DB_TEXT},
	{"identifier", DB_TEXT},
	{"registryCode", DB_TEXT},
	{"dbState", DB_INTEGER},
	{"dbEffectiveOVM", DB_BOOLEAN},
	{"dbOpenAddressing", DB_BOOLEAN}
	};

	const QMap<QString, QString> colConstraints = {
	    {"key", "NOT NULL"},
	    {"dbID", "NOT NULL"}
	};

	const QString tblConstraint = {
	    ",\n"
	    "        PRIMARY KEY (key),\n"
	    "        CHECK (dbEffectiveOVM IN (0, 1)),\n"
	    "        CHECK (dbOpenAddressing IN (0, 1))"
	};

	QMap<QString, AttrProp> attrProps = {
	{"key",                    {DB_TEXT, ""}},
	{"dbID",                   {DB_TEXT, QObject::tr("Data box ID")}},
	{"dbType",                 {DB_TEXT, QObject::tr("Data box type")}},
	{"ic",                     {DB_INTEGER, QObject::tr("IČ")}},
	{"pnFirstName",            {DB_TEXT, QObject::tr("Given name")}},
	{"pnMiddleName",           {DB_TEXT, QObject::tr("Middle name")}},
	{"pnLastName",             {DB_TEXT, QObject::tr("Surname")}},
	{"pnLastNameAtBirth",      {DB_TEXT, QObject::tr("Surname at birth")}},
	{"firmName",               {DB_TEXT, QObject::tr("Firm name")}},
	{"biDate",                 {DB_DATE, QObject::tr("Date of birth")}},
	{"biCity",                 {DB_TEXT, QObject::tr("City of birth")}},
	{"biCounty",               {DB_TEXT, QObject::tr("County of birth")}},
	{"biState",                {DB_TEXT, QObject::tr("State of birth")}},
	{"adCity",                 {DB_TEXT, QObject::tr("City of residence")}},
	{"adStreet",               {DB_TEXT, QObject::tr("Street of residence")}},
	{"adNumberInStreet",       {DB_TEXT, QObject::tr("Number in street")}},
	{"adNumberInMunicipality", {DB_TEXT, QObject::tr("Number in municipality")}},
	{"adZipCode",              {DB_TEXT, QObject::tr("Zip code")}},
	{"adState",                {DB_TEXT, QObject::tr("State of residence")}},
	{"nationality",            {DB_TEXT, QObject::tr("Nationality")}},
	{"identifier",             {DB_TEXT, ""}}, //
	{"registryCode",           {DB_TEXT, ""}}, //
	{"dbState",                {DB_INTEGER, QObject::tr("Databox state")}},
	{"dbEffectiveOVM",         {DB_BOOLEAN, QObject::tr("Effective OVM")}},
	{"dbOpenAddressing",       {DB_BOOLEAN, QObject::tr("Open addressing")}}
	};
} /* namespace AccntinfTbl */
Tbl accntinfTbl(AccntinfTbl::tabName, AccntinfTbl::knownAttrs,
    AccntinfTbl::attrProps, AccntinfTbl::colConstraints,
    AccntinfTbl::tblConstraint);


namespace UserinfTbl {
	const QString tabName("user_info");

	const QVector< QPair<QString, EntryType> > knownAttrs = {
	{"key", DB_TEXT}, /* NOT NULL */
	{"userType", DB_TEXT},
	{"userPrivils", DB_INTEGER},
	{"pnFirstName", DB_TEXT},
	{"pnMiddleName", DB_TEXT},
	{"pnLastName", DB_TEXT},
	{"pnLastNameAtBirth", DB_TEXT},
	{"adCity", DB_TEXT},
	{"adStreet", DB_TEXT},
	{"adNumberInStreet", DB_TEXT},
	{"adNumberInMunicipality", DB_TEXT},
	{"adZipCode", DB_TEXT},
	{"adState", DB_TEXT},
	{"biDate", DB_DATE},
	{"ic", DB_INTEGER},
	{"firmName", DB_TEXT},
	{"caStreet", DB_TEXT},
	{"caCity", DB_TEXT},
	{"caZipCode", DB_TEXT},
	{"caState", DB_TEXT},
	};

	const QMap<QString, QString> colConstraints = {
	    {"key", "NOT NULL"}
	};

	const QString tblConstraint = {
	    ",\n"
	    "        PRIMARY KEY (key)"
	};

	QMap<QString, AttrProp> attrProps = {
	{"key",                    {DB_TEXT, ""}},
	{"userType",               {DB_TEXT, QObject::tr("User type")}},
	{"userPrivils",            {DB_INTEGER, QObject::tr("Permissions")}},
	{"pnFirstName",            {DB_TEXT, QObject::tr("Given name")}},
	{"pnMiddleName",           {DB_TEXT, QObject::tr("Middle name")}},
	{"pnLastName",             {DB_TEXT, QObject::tr("Surname")}},
	{"pnLastNameAtBirth",      {DB_TEXT, QObject::tr("Surname at birth")}},
	{"adCity",                 {DB_TEXT, QObject::tr("City")}},
	{"adStreet",               {DB_TEXT, QObject::tr("Street")}},
	{"adNumberInStreet",       {DB_TEXT, QObject::tr("Number in street")}},
	{"adNumberInMunicipality", {DB_TEXT, QObject::tr("Number in municipality")}},
	{"adZipCode",              {DB_TEXT, QObject::tr("Zip code")}},
	{"adState",                {DB_TEXT, QObject::tr("State")}},
	{"biDate",                 {DB_DATE, QObject::tr("Date of birth")}},
	{"ic",                     {DB_INTEGER, QObject::tr("IČ")}},
	{"firmName",               {DB_TEXT, QObject::tr("Firm name")}},
	{"caStreet",               {DB_TEXT, QObject::tr("Street of residence")}},
	{"caCity",                 {DB_TEXT, QObject::tr("City of residence")}},
	{"caZipCode",              {DB_TEXT, QObject::tr("Zip code")}},
	{"caState",                {DB_TEXT, QObject::tr("State of residence")}}
	};
} /* namespace UserinfTbl */
Tbl userinfTbl(UserinfTbl::tabName, UserinfTbl::knownAttrs,
    UserinfTbl::attrProps, UserinfTbl::colConstraints,
    UserinfTbl::tblConstraint);


namespace PwdexpdtTbl {
	const QString tabName("password_expiration_date");

	const QVector< QPair<QString, EntryType> > knownAttrs = {
	{"key", DB_TEXT}, /* NOT NULL */
	{"expDate", DB_TEXT}
	};

	const QMap<QString, QString> colConstraints = {
	    {"key", "NOT NULL"}
	};

	const QString tblConstraint = {
	    ",\n"
	    "        PRIMARY KEY (key)"
	};

	const QMap<QString, AttrProp> attrProps = {
	{"key",     {DB_TEXT, ""}},
	{"expDate", {DB_TEXT, ""}}
	};
} /* namespace PwdexpdtTbl */
Tbl pwdexpdtTbl(PwdexpdtTbl::tabName, PwdexpdtTbl::knownAttrs,
    PwdexpdtTbl::attrProps, PwdexpdtTbl::colConstraints,
    PwdexpdtTbl::tblConstraint);


namespace MsgsTbl {
	const QString tabName("messages");

	const QVector< QPair<QString, EntryType> > knownAttrs = {
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
Tbl msgsTbl(MsgsTbl::tabName, MsgsTbl::knownAttrs, MsgsTbl::attrProps,
    MsgsTbl::colConstraints, MsgsTbl::tblConstraint);


namespace FlsTbl {
	const QString tabName("files");

	const QVector< QPair<QString, EntryType> > knownAttrs = {
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
	{"_dmFileDescr",     {DB_TEXT, QObject::tr("File name")}},
	{"_dmUpFileGuid",    {DB_TEXT, ""}},
	{"_dmFileGuid",      {DB_TEXT, ""}},
	{"_dmMimeType",      {DB_TEXT, QObject::tr("Mime type")}},
	{"_dmFormat",        {DB_TEXT, ""}},
	{"_dmFileMetaType",  {DB_TEXT, ""}},
	{"dmEncodedContent", {DB_TEXT, ""}}
	};
} /* namespace FlsTbl */
Tbl flsTbl(FlsTbl::tabName, FlsTbl::knownAttrs, FlsTbl::attrProps,
    FlsTbl::colConstraints, FlsTbl::tblConstraint);


namespace HshsTbl {
	const QString tabName("hashes");

	const QVector< QPair<QString, EntryType> > knownAttrs = {
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
Tbl hshsTbl(HshsTbl::tabName, HshsTbl::knownAttrs, HshsTbl::attrProps,
    HshsTbl::colConstraints, HshsTbl::tblConstraint);


namespace EvntsTbl {
	const QString tabName("events");

	const QVector< QPair<QString, EntryType> > knownAttrs = {
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
Tbl evntsTbl(EvntsTbl::tabName, EvntsTbl::knownAttrs,
    EvntsTbl::attrProps, EvntsTbl::colConstraints, EvntsTbl::tblConstraint);


namespace RwmsgdtTbl {
	const QString tabName("raw_message_data");

	const QVector< QPair<QString, EntryType> > knownAttrs = {
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
Tbl rwmsgdtTbl(RwmsgdtTbl::tabName, RwmsgdtTbl::knownAttrs,
    RwmsgdtTbl::attrProps, RwmsgdtTbl::colConstraints,
    RwmsgdtTbl::tblConstraint);


namespace RwdlvrinfdtTbl {
	const QString tabName("raw_delivery_info_data");

	const QVector< QPair<QString, EntryType> > knownAttrs = {
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
Tbl rwdlvrinfdtTbl(RwdlvrinfdtTbl::tabName, RwdlvrinfdtTbl::knownAttrs,
    RwdlvrinfdtTbl::attrProps, RwdlvrinfdtTbl::colConstraints,
    RwdlvrinfdtTbl::tblConstraint);


namespace SmsgdtTbl {
	const QString tabName("supplementary_message_data");

	const QVector< QPair<QString, EntryType> > knownAttrs = {
	{"message_id", DB_INTEGER}, /* NOT NULL */
	{"message_type", DB_INTEGER},
	{"read_locally", DB_BOOL_READ_LOCALLY},
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
	{"read_locally",  {DB_BOOL_READ_LOCALLY, QObject::tr("Read locally")}},
	{"download_date", {DB_DATETIME, ""}},
	{"custom_data",   {DB_TEXT, ""}}
	};
} /* namespace SmsgdtTbl */
Tbl smsgdtTbl(SmsgdtTbl::tabName, SmsgdtTbl::knownAttrs,
    SmsgdtTbl::attrProps, SmsgdtTbl::colConstraints, SmsgdtTbl::tblConstraint);


namespace CrtdtTbl {
	const QString tabName("certificate_data");

	const QVector< QPair<QString, EntryType> > knownAttrs = {
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
Tbl crtdtTbl(CrtdtTbl::tabName, CrtdtTbl::knownAttrs,
    CrtdtTbl::attrProps, CrtdtTbl::colConstraints, CrtdtTbl::tblConstraint);


namespace MsgcrtdtTbl {
	const QString tabName("message_certificate_data");

	const QVector< QPair<QString, EntryType> > knownAttrs = {
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
Tbl msgcrtdtTbl(MsgcrtdtTbl::tabName, MsgcrtdtTbl::knownAttrs,
    MsgcrtdtTbl::attrProps, MsgcrtdtTbl::colConstraints,
    MsgcrtdtTbl::tblConstraint);


namespace PrcstTbl {
	const QString tabName("process_state");

	const QVector< QPair<QString, EntryType> > knownAttrs = {
	{"message_id", DB_INTEGER}, /* NOT NULL */
	{"state",      DB_INTEGER}
	/*
	 * PRIMARY KEY (message_id),
	 * FOREIGN KEY(message_id) REFERENCES messages ("dmID")
	 */
	};

	const QMap<QString, QString> colConstraints {
	    {"message_id", "NOT NULL"}
	};

	const QString &tblConstraint(
	    ",\n"
	    "        PRIMARY KEY (message_id),\n"
	    "        FOREIGN KEY(message_id) REFERENCES messages (dmID)"
	);

	const QMap<QString, AttrProp> attrProps = {
	{"message_id", {DB_INTEGER, ""}},
	{"state",      {DB_INTEGER, ""}}
	};
} /* namespace PrcstTbl */
Tbl prcstTbl(PrcstTbl::tabName, PrcstTbl::knownAttrs,
    PrcstTbl::attrProps, PrcstTbl::colConstraints,
    PrcstTbl::tblConstraint);

