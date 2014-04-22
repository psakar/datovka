

#include <QDateTime>
#include <QString>

#include "dbs.h"


const QString MsgsTbl::tabName("messages");


const QVector< QPair<QString, dbEntryType> > MsgsTbl::knownAttrs = {
	{"dmID", DB_INTEGER},
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
};


const QMap<QString, AttrProp> MsgsTbl::attrProps = {
	{"dmID",                  {DB_INTEGER, QObject::tr("ID")}},
	{"is_verified",           {DB_BOOLEAN, ""}},
	{"_origin",               {DB_TEXT, ""}},
	{"dbIDSender",            {DB_TEXT, ""}},
	{"dmSender",              {DB_TEXT, QObject::tr("Sender")}},
	{"dmSenderAddress",       {DB_TEXT, ""}},
	{"dmSenderType",          {DB_INTEGER, ""}},
	{"dmRecipient",           {DB_TEXT, QObject::tr("Recipient")}},
	{"dmRecipientAddress",    {DB_TEXT, ""}},
	{"dmAmbiguousRecipient",  {DB_TEXT, ""}},
	{"dmSenderOrgUnit",       {DB_TEXT, ""}},
	{"dmSenderOrgUnitNum",    {DB_TEXT, ""}},
	{"dbIDRecipient",         {DB_TEXT, ""}},
	{"dmRecipientOrgUnit",    {DB_TEXT, ""}},
	{"dmRecipientOrgUnitNum", {DB_TEXT, ""}},
	{"dmToHands",             {DB_TEXT, ""}},
	{"dmAnnotation",          {DB_TEXT, QObject::tr("Title")}},
	{"dmRecipientRefNumber",  {DB_TEXT, ""}},
	{"dmSenderRefNumber",     {DB_TEXT, ""}},
	{"dmRecipientIdent",      {DB_TEXT, ""}},
	{"dmSenderIdent",         {DB_TEXT, ""}},
	{"dmLegalTitleLaw",       {DB_TEXT, ""}},
	{"dmLegalTitleYear",      {DB_TEXT, ""}},
	{"dmLegalTitleSect",      {DB_TEXT, ""}},
	{"dmLegalTitlePar",       {DB_TEXT, ""}},
	{"dmLegalTitlePoint",     {DB_TEXT, ""}},
	{"dmPersonalDelivery",    {DB_BOOLEAN, ""}},
	{"dmAllowSubstDelivery",  {DB_BOOLEAN, ""}},
	{"dmQTimestamp",          {DB_TEXT, ""}},
	{"dmDeliveryTime",        {DB_DATETIME, QObject::tr("Delivered")}},
	{"dmAcceptanceTime",      {DB_DATETIME, QObject::tr("Accepted")}},
	{"dmMessageStatus",       {DB_INTEGER, QObject::tr("Status")}},
	{"dmAttachmentSize",      {DB_INTEGER, ""}},
	{"_dmType",               {DB_TEXT, ""}}
};


const QString SmsgdTbl::tabName("supplementary_message_data");


const QVector< QPair<QString, dbEntryType> > SmsgdTbl::knownAttrs = {
	{"message_id", DB_INTEGER},
	{"message_type", DB_INTEGER},
	{"read_locally", DB_BOOLEAN},
	{"download_date", DB_DATETIME},
	{"custom_data", DB_TEXT}
};


const QMap<QString, AttrProp> SmsgdTbl::attrProps = {
	{"message_id",    {DB_INTEGER, ""}},
	{"message_type",  {DB_INTEGER, ""}},
	{"read_locally",  {DB_BOOLEAN, ""}},
	{"download_date", {DB_DATETIME, ""}},
	{"custom_data",   {DB_TEXT, ""}}
};


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
QString dateTimeFromDbFormat(const QString &dateTimeStr, const QString &tgtFmt)
/* ========================================================================= */
{
	QDateTime date = QDateTime::fromString(dateTimeStr, dbDateTimeFormat);
	if (date.isNull() || !date.isValid()) {
		/* Try the faulty format. */
		date = QDateTime::fromString(dateTimeStr,
		    dbFaultyDateTimeFormat);
	}
	if (date.isValid()) {
		return date.toString(tgtFmt);
	} else {
		return dateTimeStr;
	}
}
