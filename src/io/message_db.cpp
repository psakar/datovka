

#include <QDebug>
#include <QDir>
#include <QJsonObject>
#include <QJsonValue>
#include <QSqlError>
#include <QSqlQuery>


#include "message_db.h"
#include "src/common.h"
#include "src/io/db_tables.h"
#include "src/io/pkcs7.h"


const QVector<QString> MessageDb::receivedItemIds = {"dmID", "dmAnnotation",
    "dmSender", "dmDeliveryTime", "dmAcceptanceTime"};


const QVector<QString> MessageDb::sentItemIds = {"dmID", "dmAnnotation",
    "dmRecipient", "dmDeliveryTime", "dmAcceptanceTime", "dmMessageStatus"};


const QVector<QString> MessageDb::msgAttribs2 = {"dmSenderIdent",
    "dmSenderRefNumber", "dmRecipientIdent", "dmRecipientRefNumber",
    "dmToHands", "dmLegalTitleLaw", "dmLegalTitleYear", "dmLegalTitleSect",
    "dmLegalTitlePar", "dmLegalTitlePoint"};


const QVector<QString> MessageDb::msgStatus = {"dmDeliveryTime",
    "dmAcceptanceTime", "dmMessageStatus"};


/* ========================================================================= */
/*
 * Convert viewed data in date/time columns.
 */
QVariant dbTableModel::data(const QModelIndex &index, int role) const
/* ========================================================================= */
{
	if ((Qt::DisplayRole == role) &&
	    (this->headerData(index.column(), Qt::Horizontal,
	         ROLE_DB_ENTRY_TYPE).toInt() == DB_DATETIME)) {
		/* Convert date on display. */
		return dateTimeStrFromDbFormat(
		    QSqlQueryModel::data(index, role).toString(),
		    dateTimeDisplayFormat);
	} else {
		return QSqlQueryModel::data(index, role);
	}
}


/* ========================================================================= */
MessageDb::MessageDb(const QString &connectionName, QObject *parent)
/* ========================================================================= */
    : QObject(parent),
    m_sqlModel()
{
	m_db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
}


/* ========================================================================= */
MessageDb::~MessageDb(void)
/* ========================================================================= */
{
	m_db.close();
}


/* ========================================================================= */
/*
 * Open database file.
 */
bool MessageDb::openDb(const QString &fileName)
/* ========================================================================= */
{
	m_db.setDatabaseName(QDir::toNativeSeparators(fileName));

	m_db.setDatabaseName(fileName);

	return m_db.open();
}


/* ========================================================================= */
/*
 * Return received messages within past 90 days;
 */
QAbstractTableModel * MessageDb::msgsRcvdModel(const QString &recipDbId)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr = "SELECT ";
	for (int i = 0; i < (receivedItemIds.size() - 1); ++i) {
		queryStr += receivedItemIds[i] + ", ";
	}
	queryStr += receivedItemIds.last();
	queryStr += " FROM messages WHERE dbIDRecipient = '" + recipDbId + "'";
//	// qDebug() << queryStr;
	query.prepare(queryStr);
	query.exec();

	m_sqlModel.setQuery(query);
	for (int i = 0; i < receivedItemIds.size(); ++i) {
		/* Description. */
		m_sqlModel.setHeaderData(i, Qt::Horizontal,
		    msgsTbl.attrProps.value(receivedItemIds[i]).desc);
		/* Data type. */
		m_sqlModel.setHeaderData(i, Qt::Horizontal,
		    msgsTbl.attrProps.value(receivedItemIds[i]).type,
		    ROLE_DB_ENTRY_TYPE);
	}

	return &m_sqlModel;
}


/* ========================================================================= */
/*
 * Return received messages within past 90 days.
 */
QAbstractTableModel * MessageDb::msgsRcvdWithin90DaysModel(
    const QString &recipDbId)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr = "SELECT ";
	for (int i = 0; i < (receivedItemIds.size() - 1); ++i) {
		queryStr += receivedItemIds[i] + ", ";
	}
	queryStr += receivedItemIds.last();
	queryStr += " FROM messages WHERE "
	    "(dbIDRecipient = '" + recipDbId + "')"
	    " and "
	    "(dmDeliveryTime >= date('now','-90 day'))";
//	// qDebug() << queryStr;
	query.prepare(queryStr);
	query.exec();

	m_sqlModel.setQuery(query);
	for (int i = 0; i < receivedItemIds.size(); ++i) {
		/* Description. */
		m_sqlModel.setHeaderData(i, Qt::Horizontal,
		    msgsTbl.attrProps.value(receivedItemIds[i]).desc);
		/* Data type. */
		m_sqlModel.setHeaderData(i, Qt::Horizontal,
		    msgsTbl.attrProps.value(receivedItemIds[i]).type,
		    ROLE_DB_ENTRY_TYPE);
	}

	return &m_sqlModel;
}


/* ========================================================================= */
/*
 * Return received messages within given year.
 */
QAbstractTableModel * MessageDb::msgsRcvdInYearModel(const QString &recipDbId,
    const QString &year)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr = "SELECT ";
	for (int i = 0; i < (receivedItemIds.size() - 1); ++i) {
		queryStr += receivedItemIds[i] + ", ";
	}
	queryStr += receivedItemIds.last();
	queryStr += " FROM messages WHERE "
	    "(dbIDRecipient = '" + recipDbId + "')"
	    " and "
	    "(strftime('%Y', dmDeliveryTime) = '" + year + "')";
//	// qDebug() << queryStr;
	query.prepare(queryStr);
	query.exec();

	m_sqlModel.setQuery(query);
	for (int i = 0; i < receivedItemIds.size(); ++i) {
		/* Description. */
		m_sqlModel.setHeaderData(i, Qt::Horizontal,
		    msgsTbl.attrProps.value(receivedItemIds[i]).desc);
		/* Data type. */
		m_sqlModel.setHeaderData(i, Qt::Horizontal,
		    msgsTbl.attrProps.value(receivedItemIds[i]).type,
		    ROLE_DB_ENTRY_TYPE);
	}

	return &m_sqlModel;
}


/* ========================================================================= */
/*
 * Return list of years (strings) in database.
 */
QList<QString> MessageDb::msgsRcvdYears(const QString &recipDbId) const
/* ========================================================================= */
{
	QList<QString> yearList;
	QSqlQuery query(m_db);
	QString queryStr = "SELECT DISTINCT strftime('%Y', dmDeliveryTime) "
	    "FROM messages WHERE "
	    "dbIDRecipient = '" + recipDbId + "'"
	    " ORDER BY dmDeliveryTime ASC";

//	// qDebug() << "Generating received year list" << recipDbId;
//	// qDebug() << queryStr;
	query.prepare(queryStr);
	if (query.exec()) {
		query.first();
		while (query.isValid()) {
//			// qDebug() << query.value(0).toString();
			yearList.append(query.value(0).toString());
			query.next();
		}
	}

	return yearList;
}


/* ========================================================================= */
/*
 * Return list of years and number of messages in database.
 */
QList< QPair<QString, int> > MessageDb::msgsRcvdYearlyCounts(
    const QString &recipDbId) const
/* ========================================================================= */
{
	QList< QPair<QString, int> > yearlyCounts;
	QList<QString> yearList = msgsRcvdYears(recipDbId);
	QSqlQuery query(m_db);
	QString queryStr;

	for (int i = 0; i < yearList.size(); ++i) {
		queryStr = "SELECT COUNT(*) AS nrRecords FROM messages WHERE "
		    "(dbIDRecipient = '" + recipDbId + "')"
		    " and "
		    "(strftime('%Y', dmDeliveryTime) = '" + yearList[i] + "')";
//		// qDebug() << queryStr;
		query.prepare(queryStr);
		if (query.exec() && query.isActive()) {
			query.first();
			yearlyCounts.append(QPair<QString, int>(yearList[i],
			    query.value(0).toInt()));
		}
	}

	return yearlyCounts;
}


/* ========================================================================= */
/*
 * Return sent messages model.
 */
QAbstractTableModel * MessageDb::msgsSntModel(const QString &sendDbId)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr = "SELECT ";
	for (int i = 0; i < (sentItemIds.size() - 1); ++i) {
		queryStr += sentItemIds[i] + ", ";
	}
	queryStr += sentItemIds.last();
	queryStr += " FROM messages WHERE dbIDSender = '" +
	    sendDbId + "'";
//	// qDebug() << queryStr;
	query.prepare(queryStr);
	query.exec();

	m_sqlModel.setQuery(query);
	for (int i = 0; i < sentItemIds.size(); ++i) {
		/* Description. */
		m_sqlModel.setHeaderData(i, Qt::Horizontal,
		    msgsTbl.attrProps.value(sentItemIds[i]).desc);
		/* Data type. */
		m_sqlModel.setHeaderData(i, Qt::Horizontal,
		    msgsTbl.attrProps.value(sentItemIds[i]).type,
		    ROLE_DB_ENTRY_TYPE);
	}

	return &m_sqlModel;
}


/* ========================================================================= */
/*
 * Return sent messages within past 90 days.
 */
QAbstractTableModel * MessageDb::msgsSntWithin90DaysModel(
    const QString &sendDbId)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr = "SELECT ";
	for (int i = 0; i < (sentItemIds.size() - 1); ++i) {
		queryStr += sentItemIds[i] + ", ";
	}
	queryStr += sentItemIds.last();
	queryStr += " FROM messages WHERE "
	    "(dbIDSender = '" + sendDbId + "')"
	    " and "
	    "(dmDeliveryTime >= date('now','-90 day'))";
//	// qDebug() << queryStr;
	query.prepare(queryStr);
	query.exec();

	m_sqlModel.setQuery(query);
	for (int i = 0; i < sentItemIds.size(); ++i) {
		/* Description. */
		m_sqlModel.setHeaderData(i, Qt::Horizontal,
		    msgsTbl.attrProps.value(sentItemIds[i]).desc);
		/* Data type. */
		m_sqlModel.setHeaderData(i, Qt::Horizontal,
		    msgsTbl.attrProps.value(sentItemIds[i]).type,
		    ROLE_DB_ENTRY_TYPE);
	}

	return &m_sqlModel;
}


/* ========================================================================= */
/*
 * Return sent messages within given year.
 */
QAbstractTableModel * MessageDb::msgsSntInYearModel(const QString &sendDbId,
    const QString &year)
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr = "SELECT ";
	for (int i = 0; i < (sentItemIds.size() - 1); ++i) {
		queryStr += sentItemIds[i] + ", ";
	}
	queryStr += sentItemIds.last();
	queryStr += " FROM messages WHERE "
	    "(dbIDSender = '" + sendDbId + "')"
	    " and "
	    "(strftime('%Y', dmDeliveryTime) = '" + year + "')";
//	// qDebug() << queryStr;
	query.prepare(queryStr);
	query.exec();

	m_sqlModel.setQuery(query);
	for (int i = 0; i < sentItemIds.size(); ++i) {
		/* Description. */
		m_sqlModel.setHeaderData(i, Qt::Horizontal,
		    msgsTbl.attrProps.value(sentItemIds[i]).desc);
		/* Data type. */
		m_sqlModel.setHeaderData(i, Qt::Horizontal,
		    msgsTbl.attrProps.value(sentItemIds[i]).type,
		    ROLE_DB_ENTRY_TYPE);
	}

	return &m_sqlModel;
}


/* ========================================================================= */
/*
 * Return list of years (strings) in database.
 */
QList<QString> MessageDb::msgsSntYears(const QString &sendDbId) const
/* ========================================================================= */
{
	QList<QString> yearList;
	QSqlQuery query(m_db);
	QString queryStr = "SELECT DISTINCT strftime('%Y', dmDeliveryTime) "
	    "FROM messages WHERE "
	    "dbIDSender = '" + sendDbId + "'"
	    " ORDER BY dmDeliveryTime ASC";

//	// qDebug() << "Generating received year list" << recipDbId;
//	// qDebug() << queryStr;
	query.prepare(queryStr);
	if (query.exec()) {
		query.first();
		while (query.isValid()) {
//			// qDebug() << query.value(0).toString();
			yearList.append(query.value(0).toString());
			query.next();
		}
	}

	return yearList;
}


/* ========================================================================= */
/*
 * Return list of years and number of messages in database.
 */
QList< QPair<QString, int> > MessageDb::msgsSntYearlyCounts(
    const QString &sendDbId) const
/* ========================================================================= */
{
	QList< QPair<QString, int> > yearlyCounts;
	QList<QString> yearList = msgsSntYears(sendDbId);
	QSqlQuery query(m_db);
	QString queryStr;

	for (int i = 0; i < yearList.size(); ++i) {
		queryStr = "SELECT COUNT(*) AS nrRecords FROM messages WHERE "
		    "(dbIDSender = '" + sendDbId + "')"
		    " and "
		    "(strftime('%Y', dmDeliveryTime) = '" + yearList[i] + "')";
//		// qDebug() << queryStr;
		query.prepare(queryStr);
		if (query.exec() && query.isActive()) {
			query.first();
			if (query.isValid()) {
				yearlyCounts.append(QPair<QString, int>(
				    yearList[i], query.value(0).toInt()));
			}
		}
	}

	return yearlyCounts;
}


/* ========================================================================= */
/*
 * Generate information for reply dialog.
 */
QVector<QString> MessageDb::msgsReplyDataTo(int dmId) const
/* ========================================================================= */
{
	QVector<QString> reply(4);
	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "SELECT "
	    "dmAnnotation, dbIDSender, dmSender, dmSenderAddress"
	    " FROM messages WHERE "
	    "dmID = " + QString::number(dmId);
//	// qDebug() << queryStr;
	query.prepare(queryStr);
	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			reply[0] = query.value(0).toString();
			reply[1] = query.value(1).toString();
			reply[2] = query.value(2).toString();
			reply[3] = query.value(3).toString();
		}
	}

	return reply;
}


/* ========================================================================= */
/*
 * Return contact list from message db.
 */
QList< QVector<QString> > MessageDb::uniqueContacts(void)
/* ========================================================================= */
{
	QList<QVector<QString>> list_contacts;
	QSqlQuery query(m_db);
	QString queryStr = "SELECT DISTINCT "
	   "dbIDSender, dmSender, dmSenderAddress"
	   " FROM messages";
	query.prepare(queryStr);
	if (query.exec()) {
		query.first();
		while (query.isValid()) {
			QVector<QString> contact;
			contact.append(query.value(0).toString());
			contact.append(query.value(1).toString());
			contact.append(query.value(2).toString());
			list_contacts.append(contact);
			query.next();
		}
	}
	return list_contacts;
}


/* ========================================================================= */
/*
 * Return message HTML formatted description.
 */
QString MessageDb::descriptionHtml(int dmId, bool showId, bool warnOld) const
/* ========================================================================= */
{
	QString html;
	QSqlQuery query(m_db);
	QString queryStr;

	html += indentDivStart;
	html += "<h3>" + tr("Identification") + "</h3>";
	if (showId) {
		html += strongAccountInfoLine(tr("ID"), QString::number(dmId));
	}

	queryStr = "SELECT "
	    "dmAnnotation, _dmType, dmSender, dmSenderAddress, "
	    "dmRecipient, dmRecipientAddress"
	    " FROM messages WHERE "
	    "dmID = " + QString::number(dmId);
	// qDebug() << queryStr;
	query.prepare(queryStr);
	if (query.exec() && query.isActive()) {
		query.first();
		html += strongAccountInfoLine(tr("Subject"),
		    query.value(0).toString());
		if (!query.value(1).toString().isEmpty() &&
		    (!dmTypeToText(query.value(1).toString()).isEmpty())) {
			html += strongAccountInfoLine(tr("Message type"),
			    dmTypeToText(query.value(1).toString()));
		}

		html += "<br/>";

		/* Information about message author. */
		html += strongAccountInfoLine("From",
		    query.value(2).toString());
		html += strongAccountInfoLine("Sender Address",
		    query.value(3).toString());
		/* Custom data. */
		QJsonDocument customData = smsgdCustomData(dmId);
		if (!customData.isEmpty() && customData.isObject()) {
			QJsonValue value =
			    customData.object().value("message_author");
			if (value.isObject()) {
				html += strongAccountInfoLine(
				    tr("Message author"),
				    value.toObject().value(
				        "authorName").toString() + ", " +
				    authorTypeToText(value.toObject().value(
				        "userType").toString()));
			}
		}

		html += "<br/>";

		html += strongAccountInfoLine(tr("To"),
		    query.value(4).toString());
		html += strongAccountInfoLine(tr("Recipient Address"),
		    query.value(5).toString());
	}

	queryStr = "SELECT ";
	for (int i = 0; i < (msgAttribs2.size() - 1); ++i) {
		queryStr += msgAttribs2[i] + ", ";
	}
	queryStr += msgAttribs2.last();
	queryStr += " FROM messages WHERE "
	    "dmID = " + QString::number(dmId);
	// qDebug() << queryStr;
	query.prepare(queryStr);
	if (query.exec() && query.isActive()) {
		query.first();
		for (int i = 0; i < msgAttribs2.size(); ++i) {
			if (!query.value(i).toString().isEmpty()) {
				html += strongAccountInfoLine(
				    msgsTbl.attrProps[msgAttribs2[i]].desc,
				    query.value(i).toString());
			}
		}
	}

	html += "<h3>" + tr("Status") + "</h3>";
	/* Status. */
	queryStr = "SELECT ";
	for (int i = 0; i < (msgStatus.size() - 1); ++i) {
		queryStr += msgStatus[i] + ", ";
	}
	queryStr += msgStatus.last();
	queryStr += " FROM messages WHERE "
	    "dmID = " + QString::number(dmId);
	// qDebug() << queryStr;
	query.prepare(queryStr);
	if (query.exec() && query.isActive()) {
		query.first();
		html += strongAccountInfoLine(
		    msgsTbl.attrProps[msgStatus[0]].desc,
		    dateTimeStrFromDbFormat(query.value(0).toString(),
		        dateTimeDisplayFormat));
		html += strongAccountInfoLine(
		    msgsTbl.attrProps[msgStatus[1]].desc,
		    dateTimeStrFromDbFormat(query.value(1).toString(),
		        dateTimeDisplayFormat));
		html += strongAccountInfoLine(
		    msgsTbl.attrProps[msgStatus[2]].desc,
		    QString::number(query.value(2).toInt()) + " -- " +
		    msgStatusToText(query.value(2).toInt()));
	}
	/* Events. */
	queryStr = "SELECT "
	    "dmEventTime, dmEventDescr"
	    " FROM events WHERE "
	    "message_id = " + QString::number(dmId) +
	    " ORDER BY dmEventTime ASC";
	// qDebug() << queryStr;
	query.prepare(queryStr);
	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			html += strongAccountInfoLine(tr("Events"), "");
		}
		while (query.isValid()) {
			html += indentDivStart +
			    strongAccountInfoLine(
			        dateTimeStrFromDbFormat(
			            query.value(0).toString(),
			            dateTimeDisplayFormat),
			        query.value(1).toString()) +
			    divEnd;
			query.next();
		} 
	}
	/* Attachments. */
	queryStr = "SELECT COUNT(*) AS nrFiles "
	    " FROM files WHERE "
	    "message_id = " + QString::number(dmId);
	// qDebug() << queryStr;
	query.prepare(queryStr);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid() &&
	    (query.value(0).toInt() > 0)) {
		html += strongAccountInfoLine(tr("Attachments"), 
		    QString::number(query.value(0).toInt()) + " " +
		    tr("(downloaded and ready)"));
	} else {
		queryStr = "SELECT "
		    "dmAttachmentSize"
		    " FROM messages WHERE "
		    "dmID = " + QString::number(dmId);
		// qDebug() << queryStr;
		query.prepare(queryStr);
		if (query.exec() && query.isActive() &&
		    query.first() && query.isValid() &&
		   (query.value(0).toInt() > 0)) {
			html += strongAccountInfoLine(tr("Attachments"),
			    tr("not downloaded yet, ~") +
			    QString::number(query.value(0).toInt()) +
			    tr(" KB; use 'Download' to get them."));
		} else {
			html += strongAccountInfoLine(tr("Attachments"), 
			    tr("(not available)"));
		}
	}
	if (warnOld) {
		/* TODO */
	}

	html += "<h3>" + tr("Signature") + "</h3>";
	/* Signature. */
	if (!msgsVarificationAttempted(dmId)) {
		/* Verification no attempted. */
		html += strongAccountInfoLine(tr("Message signature"),
		    tr("Not present"));
		/* TODO -- Enable verification button. */
	} else if (!msgsVerified(dmId)) {
		html += strongAccountInfoLine(tr("Message signature"),
		    tr("Invalid")  + " -- " +
		    tr("Message signature and content do not correspond!"));
	} else {
		html += strongAccountInfoLine(tr("Message signature"),
		    tr("Valid"));
		/* Check signing certificate. */
		// qDebug() << msgsVerificationDate(dmId);
		bool verified = msgCertValidAtDate(dmId,
		    msgsVerificationDate(dmId), !globPref.check_crl);
		QString verifiedText = verified ? tr("Valid") : tr("Invalid");
		if (!globPref.check_crl) {
			verifiedText += " (" +
			    tr("Certificate revocation check is turned off!") +
			    ")";
		}
		html += strongAccountInfoLine(tr("Signing certificate"),
		    verifiedText);
		/* TODO */
	}

	/* TODO */

	html += divEnd;

//	html += QString::number(dmId);
	// qDebug() << html;
	/* TODO */

	return html;
}


/* ========================================================================= */
/*
 * Returns true if verification attempt was performed.
 */
bool MessageDb::msgsVarificationAttempted(int dmId) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;
	bool ok;

	queryStr = "SELECT "
	    "is_verified"
	    " FROM messages WHERE "
	    "dmID = " + QString::number(dmId);
	// qDebug() << queryStr;
	query.prepare(queryStr);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		/* If no value is set then the conversion will fail. */
		query.value(0).toInt(&ok);
		return ok;
	}

	return false;
}


/* ========================================================================= */
/*
 * Returns whether message is verified.
 */
bool MessageDb::msgsVerified(int dmId) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "SELECT "
	    "is_verified"
	    " FROM messages WHERE "
	    "dmID = " + QString::number(dmId);
//	// qDebug() << queryStr;
	query.prepare(queryStr);
	if (query.exec() && query.isActive() &&
	    query.first() && query.isValid()) {
		return query.value(0).toBool();
	}

	return false;
}


/* ========================================================================= */
/*
 * Returns verification date.
 */
QDateTime MessageDb::msgsVerificationDate(int dmId) const
/* ========================================================================= */
{
	QSqlQuery query(m_db);
	QString queryStr;

	if (GlobPreferences::DOWNLOAD_DATE ==
	    globPref.certificate_validation_date) {

		queryStr = "SELECT "
		    "download_date"
		    " FROM supplementary_message_data WHERE "
		    "message_id = " + QString::number(dmId);
		// qDebug() << queryStr;
		query.prepare(queryStr);
		if (query.exec() && query.isActive() &&
		    query.first() && query.isValid()) {
			// qDebug() << "dateTime" << query.value(0).toString();
			QDateTime dateTime =
			    dateTimeFromDbFormat(query.value(0).toString());

			if (dateTime.isValid()) {
				return dateTime;
			}
		}
	}

	return QDateTime::currentDateTime();
}


/* ========================================================================= */
/*
 * Read data from supplementary message data table.
 */
QJsonDocument MessageDb::smsgdCustomData(int msgId) const
/* ========================================================================= */
{
	QJsonDocument jsonDoc;
	QSqlQuery query(m_db);
	QString queryStr;

	queryStr = "SELECT "
	    "custom_data"
	    " FROM supplementary_message_data WHERE "
	    "message_id = "  + QString::number(msgId);
//	// qDebug() << queryStr;
	query.prepare(queryStr);
	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			jsonDoc = QJsonDocument::fromJson(
			    query.value(0).toByteArray());
		}
	}

	return jsonDoc;
}


/* ========================================================================= */
/*
 * Certificates related to given message.
 */
QList<QSslCertificate> MessageDb::msgCerts(int dmId) const
/* ========================================================================= */
{
	QList<QSslCertificate> certList;
	QSqlQuery query(m_db);
	QString queryStr;
	QList<int> certIds;

	queryStr = "SELECT "
	    "certificate_id"
	    " FROM message_certificate_data WHERE "
	    "message_id = " + QString::number(dmId);
	// qDebug() << queryStr;
	query.prepare(queryStr);
	if (query.exec() && query.isActive()) {
		query.first();
		if (query.isValid()) {
			bool ok;
			int ret;
			ret = query.value(0).toInt(&ok);
			Q_ASSERT(ok);
			if (ok) {
				certIds.append(ret);
			}
		}
	}

	if (certIds.size() > 0) {
		queryStr = "SELECT "
		    "der_data"
		    " FROM certificate_data WHERE ";
		for (int i = 0; i < (certIds.size() - 1); ++i) {
			queryStr += "(id = " + QString::number(certIds[i]) +
			    ") or ";
		}
		queryStr += "(id = " + QString::number(certIds.last()) + ")";
		// qDebug() << queryStr;
		query.prepare(queryStr);
		if (query.exec() && query.isActive()) {
			query.first();
			while (query.isValid()) {
				QList<QSslCertificate> certs =
				    QSslCertificate::fromData(
				        QByteArray::fromBase64(
				            query.value(0).toByteArray()),
				        QSsl::Der);
				Q_ASSERT(1 == certs.size());

				certList.append(certs.first());

				query.next();
			}
		}
	}

	return certList;
}


/* ========================================================================= */
/*
 * Check whether message signature was valid at given date.
 */
bool MessageDb::msgCertValidAtDate(int dmId, const QDateTime &dateTime,
    bool ignoreMissingCrlCheck) const
/* ========================================================================= */
{
	QList<QSslCertificate> certList = msgCerts(dmId);

	if (certList.size() > 0) {
		Q_ASSERT(1 == certList.size());

		const QSslCertificate &cert = certList.first();

		bool timeValid = certTimeValidAtTime(cert, dateTime);
		bool crlValid = ignoreMissingCrlCheck; /* true; */
		if (globPref.check_crl) {
			crlValid = certCrlValidAtTime(cert, dateTime);
		}

		return timeValid && crlValid;
	}

	return false;
}


/* ========================================================================= */
/*
 * Adds _dmType column.
 */
bool  MessageDb::addDmtypeColumn(void)
/* ========================================================================= */
{
	if (false == m_db.isOpen()) {
		return false;
	}

	/*
	 * Create _dmType column if it does not exist.
	 */
	QSqlQuery query(m_db);
	query.prepare("SELECT _dmType FROM messages LIMIT 1");
	if (false == query.exec()) {
		query.clear();
		query.prepare("ALTER TABLE messages ADD COLUMN _dmType TEXT");
		query.exec();
	}

	return true;
}


/* ========================================================================= */
dbContainer::dbContainer(void)
/* ========================================================================= */
    : QMap<QString, MessageDb *>()
{
}


/* ========================================================================= */
dbContainer::~dbContainer(void)
/* ========================================================================= */
{
	QMap<QString, MessageDb *>::iterator i;

	for (i = this->begin(); i != this->end(); ++i) {
		delete i.value();
	}
}


/* ========================================================================= */
MessageDb * dbContainer::accessMessageDb(const QString &key,
    const QString &locDir, bool testing)
/* ========================================================================= */
{
	MessageDb *db;

	/* Already opened. */
	if (this->find(key) != this->end()) {
//		// qDebug() << key << "db found";
		return (*this)[key];
	}

//	// qDebug() << "creating new" << key;
	db = new MessageDb(key);

//	// qDebug() << "searching for file" << key << "in" << locDir;
	/* TODO -- Handle file name deviations! */
//	// qDebug() << "opening";
	/*
	 * Test accounts have ___1 in their names, ___0 relates to standard
	 * accounts.
	 */
	db->openDb(locDir + "/" + key + "___" + (testing ? "1" : "0") + ".db");

	this->insert(key, db);
	return db;
}
