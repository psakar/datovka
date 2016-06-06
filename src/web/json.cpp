/*
 * Copyright (C) 2014-2016 CZ.NIC
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
#include <QVariant>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "src/common.h"
#include "src/web/json.h"
#include "src/web/net.h"
#include "src/io/wd_sessions.h"

JsonLayer jsonlayer;

JsonLayer::JsonLayer(QObject *parent)
    : QObject(parent)
{
}

JsonLayer::~JsonLayer(void)
{
}

/* TODO - mojeID login sekvence - only for testing */
QByteArray JsonLayer::mojeIDtest(void)
{
	QByteArray reply;
#if 1
	QUrl url0(MOJEID_BASE_URL0);
	netmanager.createGetRequestMojeId(url0, reply);

	QUrl url1(MOJEID_BASE_URL1);
	netmanager.createPostRequestMojeId(url1, QByteArray("this_is_mojeid_form=1"),
	   reply);

	QUrl url2(MOJEID_BASE_URL2);
	netmanager.createPostRequestMojeId(url2, QByteArray("openid.pape.preferred_auth_policies=&openid.ns.pape=http://specs.openid.net/extensions/pape/1.0&openid.ns=http://specs.openid.net/auth/2.0&openid.realm=https://mojeid.cz/consumer/&openid.return_to=https://mojeid.cz/consumer/?janrain_nonce=2016-05-06T10%3A48%3A04Z3BEl1X&openid.ax.mode=fetch_request&openid.claimed_id=http://specs.openid.net/auth/2.0/identifier_select&openid.ns.sreg=http://openid.net/extensions/sreg/1.1&openid.ns.ax=http://openid.net/srv/ax/1.0&openid.assoc_handle={HMAC-SHA1}{5727d097}{XFt3tg==}&openid.mode=checkid_setup&openid.identity=http://specs.openid.net/auth/2.0/identifier_select"), reply);

	QUrl url3(MOJEID_BASE_URL3);
	netmanager.createGetRequestMojeId(url3, reply);

	QString str = "csrfmiddlewaretoken=";
	for (int i = 0; i < cookieList.size(); ++i) {
		if (cookieList.at(i).name() == COOKIE_CSRFTOKEN) {
			str += cookieList.at(i).value();
		}
	}

	str += "&token=zzzzzzz";
	str += "&username=xxxxx";
	str += "&password=yyyyyy";
	str += "&allow=Přihlásit+se";

	QByteArray dat;
	dat.append(str);

	netmanager.createPostRequestMojeId(url3, dat, reply);
#endif
	return reply;
}


QString JsonLayer::startLoginToWebDatovka(void) {

	QByteArray reply;
	QNetworkCookie sessionid;

	netmanager.createGetRequestWebDatovka(
	    QUrl(QString(WEBDATOVKA_SERVICE_URL) + "mojeid/login"),  sessionid,
	    reply);

	return QString();
}


QNetworkCookie JsonLayer::loginToWebDatovka(void) {

	QUrl url(WEBDATOVKA_LOGIN_URL);
	QByteArray reply;
	QNetworkCookie sessionid;

	netmanager.createGetRequestWebDatovka(url, sessionid, reply);

	if (cookieList.isEmpty() || cookieList.count() < 3) {
		return QNetworkCookie();
	}

	return cookieList.at(2);
}


bool JsonLayer::isLoggedToWebDatovka(const QString &userName,
    QNetworkCookie &sessionid)
{
	sessionid = wdSessions.sessionCookie(userName);
	if (sessionid.value().isEmpty()) {
		sessionid = loginToWebDatovka();
		if (sessionid.name().isEmpty()) {
				return false;
		}
		wdSessions.setSessionCookie(userName, sessionid);
	}

	return true;
}


bool JsonLayer::pingServer(const QString &userName, QString &errStr)
{
	QByteArray reply;
	QNetworkCookie sessionid;

	if (!isLoggedToWebDatovka(userName, sessionid)) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	netmanager.createGetRequestWebDatovka(
	    QUrl(QString(WEBDATOVKA_SERVICE_URL) + "ping"), sessionid, reply);

	QJsonDocument jsonResponse = QJsonDocument::fromJson(reply);
	QJsonObject jsonObject = jsonResponse.object();
	if (!jsonObject["success"].toBool()) {
		errStr = jsonObject["errmsg"].toString();
		return false;
	}

	return true;
}


bool JsonLayer::createAccount(const QString &userName,const QString &name,
    QString &errStr)
{
	QByteArray reply;
	QNetworkCookie sessionid;

	if (!isLoggedToWebDatovka(userName, sessionid)) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QJsonObject rootObj;
	rootObj["name"] = name;

	netmanager.createPostRequestWebDatovka(
	    QUrl(QString(WEBDATOVKA_SERVICE_URL) + "newaccount"), sessionid,
	    QJsonDocument(rootObj).toJson(QJsonDocument::Compact),
	    reply);

	QJsonDocument jsonResponse = QJsonDocument::fromJson(reply);
	QJsonObject jsonObject = jsonResponse.object();
	if (!jsonObject["success"].toBool()) {
		errStr = jsonObject["errmsg"].toString();
		return false;
	}

	/* TODO - parse new account data and store to db
	 * Here is not API implemented yet.
	 */

	return true;
}


bool JsonLayer::renameAccount(const QString &userName,int accountID,
    const QString &newName, QString &errStr)
{
	QByteArray reply;
	QNetworkCookie sessionid;

	if (!isLoggedToWebDatovka(userName, sessionid)) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QJsonObject rootObj;
	rootObj["account"] = accountID;
	rootObj["name"] = newName;

	netmanager.createPostRequestWebDatovka(
	    QUrl(QString(WEBDATOVKA_SERVICE_URL) + "renameaccount"), sessionid,
	    QJsonDocument(rootObj).toJson(QJsonDocument::Compact),
	    reply);

	QJsonDocument jsonResponse = QJsonDocument::fromJson(reply);
	QJsonObject jsonObject = jsonResponse.object();
	if (!jsonObject["success"].toBool()) {
		errStr = jsonObject["errmsg"].toString();
		return false;
	}

	return true;
}


bool JsonLayer::deleteAccount(const QString &userName, int accountID,
    QString &errStr)
{
	QByteArray reply;
	QNetworkCookie sessionid;

	if (!isLoggedToWebDatovka(userName, sessionid)) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QJsonObject rootObj;
	rootObj["account"] = accountID;

	netmanager.createPostRequestWebDatovka(
	    QUrl(QString(WEBDATOVKA_SERVICE_URL) + "deleteaccount"), sessionid,
	    QJsonDocument(rootObj).toJson(QJsonDocument::Compact),
	    reply);

	QJsonDocument jsonResponse = QJsonDocument::fromJson(reply);
	QJsonObject jsonObject = jsonResponse.object();
	if (!jsonObject["success"].toBool()) {
		errStr = jsonObject["errmsg"].toString();
		return false;
	}

	return true;
}


bool JsonLayer::getAccountList(const QNetworkCookie &sessionid,
    QList<JsonLayer::AccountData> &accountList, QString &errStr)
{
	QByteArray reply;

	netmanager.createGetRequestWebDatovka(
	    QUrl(QString(WEBDATOVKA_SERVICE_URL) + "accountlist"), sessionid,
	    reply);

	if (reply.isEmpty()) {
		return false;
	}

	return parseAccountList(reply, accountList, errStr);
}


bool JsonLayer::getMessageList(const QString &userName, int accountID,
    int messageType, int limit, int offset,
    QList<JsonLayer::Envelope> &messageList, QString &errStr)
{
	QByteArray reply;
	QNetworkCookie sessionid;

	if (!isLoggedToWebDatovka(userName, sessionid)) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QJsonObject rootObj;
	rootObj["account"] = accountID;
	rootObj["folder"] = messageType;
	rootObj["offset"] = offset;
	rootObj["limit"] = limit;

	netmanager.createPostRequestWebDatovka(
	    QUrl(QString(WEBDATOVKA_SERVICE_URL) + "msgenvelopelist"), sessionid,
	    QJsonDocument(rootObj).toJson(QJsonDocument::Compact),
	    reply);

	if (reply.isEmpty()) {
		errStr = tr("Reply content missing");
		return false;
	}

	return parseMessageList(reply, messageList, errStr);
}


bool JsonLayer::syncAccount(const QString &userName, int accountID,
    QString &errStr)
{
	QByteArray reply;
	QNetworkCookie sessionid;

	if (!isLoggedToWebDatovka(userName, sessionid)) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QJsonObject rootObj;
	rootObj["account"] = accountID;

	netmanager.createPostRequestWebDatovka(
	    QUrl(QString(WEBDATOVKA_SERVICE_URL) + "synchronize"), sessionid,
	    QJsonDocument(rootObj).toJson(QJsonDocument::Compact),
	    reply);

	if (reply.isEmpty()) {
		errStr = tr("Reply content missing");
		return false;
	}

	return parseSyncAccount(reply, errStr);
}


QByteArray JsonLayer::downloadMessage(const QString &userName, int msgId,
    QString &errStr)
{
	QByteArray reply;
	QNetworkCookie sessionid;

	if (!isLoggedToWebDatovka(userName, sessionid)) {
		errStr = tr("User is not logged to mojeID");
		return QByteArray();
	}

	netmanager.createGetRequestWebDatovka(
	    QUrl(QString(WEBDATOVKA_SERVICE_URL) + "downloadsigned/"
	    + QString::number(msgId)), sessionid, reply);

	return reply;
}


QByteArray JsonLayer::downloadFile(const QString &userName, int fileId,
    QString &errStr)
{
	QByteArray reply;
	QNetworkCookie sessionid;

	if (!isLoggedToWebDatovka(userName, sessionid)) {
		errStr = tr("User is not logged to mojeID");
		return QByteArray();
	}

	netmanager.createGetRequestWebDatovka(
	    QUrl(QString(WEBDATOVKA_SERVICE_URL) + "downloadfile/"
	    + QString::number(fileId)), sessionid, reply);

	return reply;
}


bool JsonLayer::getTagList(const QString &userName,
    QList<JsonLayer::Tag> &tagList, QString &errStr)
{
	QByteArray reply;
	QNetworkCookie sessionid;

	if (!isLoggedToWebDatovka(userName, sessionid)) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QJsonObject rootObj;
	netmanager.createPostRequestWebDatovka(
	    QUrl(QString(WEBDATOVKA_SERVICE_URL) + "listtags"), sessionid,
	    QJsonDocument(rootObj).toJson(QJsonDocument::Compact),
	    reply);

	if (reply.isEmpty()) {
		errStr = tr("Reply content missing");
		return false;
	}

	return parseTagList(reply, tagList, errStr);
}


int JsonLayer::createTag(const QString &userName, const QString &name,
    const QString &color, QString &errStr)
{
	QByteArray reply;
	QNetworkCookie sessionid;

	if (!isLoggedToWebDatovka(userName, sessionid)) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QJsonObject rootObj;
	rootObj["name"] = name;
	rootObj["color"] = color;

	netmanager.createPostRequestWebDatovka(
	    QUrl(QString(WEBDATOVKA_SERVICE_URL) + "newtag"), sessionid,
	    QJsonDocument(rootObj).toJson(QJsonDocument::Compact),
	    reply);

	if (reply.isEmpty()) {
		errStr = tr("Reply content missing");
		return -1;
	}

	QJsonDocument jsonResponse = QJsonDocument::fromJson(reply);
	QJsonObject jsonObject = jsonResponse.object();
	if (!jsonObject["success"].toBool()) {
		errStr = jsonObject["errmsg"].toString();
		return -1;
	}
	return jsonObject["id"].toInt();
}


bool JsonLayer::updateTag(const QString &userName, int tagId, const QString &name,
    const QString &color, QString &errStr)
{
	QByteArray reply;
	QNetworkCookie sessionid;

	if (!isLoggedToWebDatovka(userName, sessionid)) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QJsonObject rootObj;
	rootObj["id"] = tagId;
	rootObj["name"] = name;
	rootObj["color"] = color;

	netmanager.createPostRequestWebDatovka(
	    QUrl(QString(WEBDATOVKA_SERVICE_URL) + "edittag"), sessionid,
	    QJsonDocument(rootObj).toJson(QJsonDocument::Compact),
	    reply);

	if (reply.isEmpty()) {
		errStr = tr("Reply content missing");
		return false;
	}

	QJsonDocument jsonResponse = QJsonDocument::fromJson(reply);
	QJsonObject jsonObject = jsonResponse.object();
	if (!jsonObject["success"].toBool()) {
		errStr = jsonObject["errmsg"].toString();
		return false;
	}

	return true;
}


bool JsonLayer::deleteTag(const QString &userName, int tagId, QString &errStr)
{
	QByteArray reply;
	QNetworkCookie sessionid;

	if (!isLoggedToWebDatovka(userName, sessionid)) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QJsonObject rootObj;
	rootObj["id"] = tagId;

	netmanager.createPostRequestWebDatovka(
	    QUrl(QString(WEBDATOVKA_SERVICE_URL) + "deletetag"), sessionid,
	    QJsonDocument(rootObj).toJson(QJsonDocument::Compact),
	    reply);

	if (reply.isEmpty()) {
		errStr = tr("Reply content missing");
		return false;
	}

	QJsonDocument jsonResponse = QJsonDocument::fromJson(reply);
	QJsonObject jsonObject = jsonResponse.object();
	if (!jsonObject["success"].toBool()) {
		errStr = jsonObject["errmsg"].toString();
		return false;
	}

	return true;
}


bool JsonLayer::assignTag(const QString &userName, int tagId, int msgId,
    QString &errStr)
{
	QByteArray reply;
	QNetworkCookie sessionid;

	if (!isLoggedToWebDatovka(userName, sessionid)) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QJsonObject rootObj;
	rootObj["id"] = tagId;
	rootObj["msgid"] = msgId;

	netmanager.createPostRequestWebDatovka(
	    QUrl(QString(WEBDATOVKA_SERVICE_URL) + "tagmsg/add"), sessionid,
	    QJsonDocument(rootObj).toJson(QJsonDocument::Compact),
	    reply);

	if (reply.isEmpty()) {
		errStr = tr("Reply content missing");
		return false;
	}

	QJsonDocument jsonResponse = QJsonDocument::fromJson(reply);
	QJsonObject jsonObject = jsonResponse.object();
	if (!jsonObject["success"].toBool()) {
		errStr = jsonObject["errmsg"].toString();
		return false;
	}

	return true;
}


bool JsonLayer::removeTag(const QString &userName, int tagId, int msgId,
    QString &errStr)
{
	QByteArray reply;
	QNetworkCookie sessionid;

	if (!isLoggedToWebDatovka(userName, sessionid)) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QJsonObject rootObj;
	rootObj["id"] = tagId;
	rootObj["msgid"] = msgId;

	netmanager.createPostRequestWebDatovka(
	    QUrl(QString(WEBDATOVKA_SERVICE_URL) + "tagmsg/remove"), sessionid,
	    QJsonDocument(rootObj).toJson(QJsonDocument::Compact),
	    reply);

	if (reply.isEmpty()) {
		errStr = tr("Reply content missing");
		return false;
	}

	QJsonDocument jsonResponse = QJsonDocument::fromJson(reply);
	QJsonObject jsonObject = jsonResponse.object();
	if (!jsonObject["success"].toBool()) {
		errStr = jsonObject["errmsg"].toString();
		return false;
	}

	return true;
}


bool JsonLayer::removeAllTags(const QString &userName, int msgId,
    QString &errStr)
{
	QByteArray reply;
	QNetworkCookie sessionid;

	if (!isLoggedToWebDatovka(userName, sessionid)) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QJsonObject rootObj;
	rootObj["msgid"] = msgId;

	netmanager.createPostRequestWebDatovka(
	    QUrl(QString(WEBDATOVKA_SERVICE_URL) + "tagmsg/removeall"), sessionid,
	    QJsonDocument(rootObj).toJson(QJsonDocument::Compact),
	    reply);

	if (reply.isEmpty()) {
		errStr = tr("Reply content missing");
		return false;
	}

	QJsonDocument jsonResponse = QJsonDocument::fromJson(reply);
	QJsonObject jsonObject = jsonResponse.object();
	if (!jsonObject["success"].toBool()) {
		errStr = jsonObject["errmsg"].toString();
		return false;
	}

	return true;
}


bool JsonLayer::searchRecipient(const QString &userName, int accountID,
    const QString &word, int position,
    QList<JsonLayer::Recipient> &resultList, bool &hasMore,
    QString &errStr)
{
	QByteArray reply;
	QNetworkCookie sessionid;

	if (!isLoggedToWebDatovka(userName, sessionid)) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QJsonObject rootObj;
	rootObj["account"] = accountID;
	rootObj["needle"] = word;
	rootObj["position"] = position;

	netmanager.createPostRequestWebDatovka(
	    QUrl(QString(WEBDATOVKA_SERVICE_URL) + "searchrecipient"), sessionid,
	    QJsonDocument(rootObj).toJson(QJsonDocument::Compact),
	    reply);

	if (reply.isEmpty()) {
		errStr = tr("Reply content missing");
		return false;
	}

	return parseSearchRecipient(reply, resultList, hasMore, errStr);
}


bool JsonLayer::sendMessageAsJson(const QString &userName, int accountID,
    const QList<JsonLayer::Recipient> &recipientList,
    const JsonLayer::Envelope &envelope,
    const QList<JsonLayer::File> &fileList,
    QStringList &resultList, QString &errStr)
{
	QByteArray reply;
	QNetworkCookie sessionid;

	if (!isLoggedToWebDatovka(userName, sessionid)) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QJsonArray recipienList;
	foreach (const JsonLayer::Recipient &recipient, recipientList) {
		QJsonObject recData;
		recData["db_id_recipient"] = recipient.recipientDbId;
		recData["name"] = recipient.recipientName;
		recData["address"] = recipient.recipientAddress;
		recData["dm_to_hands"] = recipient.toHands;
		recipienList.append(recData);
	}

	QJsonObject msgEnvelope;
	msgEnvelope["dm_annotation"] = envelope.dmAnnotation;
	msgEnvelope["dm_recipient_ident"] = envelope.dmRecipientIdent;
	msgEnvelope["dm_recipient_ref_number"] = envelope.dmRecipientRefNumber;
	msgEnvelope["dm_sender_ident"] = envelope.dmSenderIdent;
	msgEnvelope["dm_sender_ref_number"] = envelope.dmSenderRefNumber;
	msgEnvelope["dm_legal_title_law"] = envelope.dmLegalTitleLaw;
	msgEnvelope["dm_legal_title_par"] = envelope.dmLegalTitlePar;
	msgEnvelope["dm_legal_title_point"] = envelope.dmLegalTitlePoint;
	msgEnvelope["dm_legal_title_sect"] = envelope.dmLegalTitleSect;
	msgEnvelope["dm_legal_title_year"] = envelope.dmLegalTitleYear;
	msgEnvelope["dm_personal_delivery"] = envelope.dmPersonalDelivery;
	msgEnvelope["dm_allow_subst_delivery"] = envelope.dmAllowSubstDelivery;

	QJsonArray files;
	foreach (const JsonLayer::File &file, fileList) {
		QJsonObject fileData;
		fileData["file_name"] = file.fName;
		fileData["file_content"] = QString(file.fContent.toBase64());
		files.append(fileData);
	}

	QJsonObject rootObj;
	rootObj["account"] = accountID;
	rootObj["recipients"] = recipienList;
	rootObj["envelope"] = msgEnvelope;
	rootObj["files"] = files;

	netmanager.createPostRequestWebDatovka(
	    QUrl(QString(WEBDATOVKA_SERVICE_URL) + "sendmessage"), sessionid,
	    QJsonDocument(rootObj).toJson(QJsonDocument::Compact),
	    reply);

	if (reply.isEmpty()) {
		errStr = tr("Reply content missing");
		return false;
	}

	QJsonDocument jsonResponse = QJsonDocument::fromJson(reply);
	QJsonObject jsonObject = jsonResponse.object();
	if (!jsonObject["success"].toBool()) {
		errStr = jsonObject["errmsg"].toString();
		return false;
	}

	QString result;
	QJsonArray errorArray = jsonObject["errors"].toArray();
	foreach (const QJsonValue &value, errorArray) {
		QJsonObject obj = value.toObject();
		result = obj["dbID"].toString() + "§" + obj["msg"].toString();
		resultList.append(result);
	}

	return true;
}


bool JsonLayer::sendMessage(const QString &userName, int accountID,
    const QList<JsonLayer::Recipient> &recipientList,
    const JsonLayer::Envelope &envelope,
    const QList<JsonLayer::File> &fileList,
    QStringList &resultList, QString &errStr)
{
	QByteArray reply;
	QNetworkCookie sessionid;

	if (!isLoggedToWebDatovka(userName, sessionid)) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QJsonArray recipienList;
	foreach (const JsonLayer::Recipient &recipient, recipientList) {
		QJsonObject recData;
		recData["db_id_recipient"] = recipient.recipientDbId;
		recData["name"] = recipient.recipientName;
		recData["address"] = recipient.recipientAddress;
		recData["dm_to_hands"] = recipient.toHands;
		recipienList.append(recData);
	}

	QJsonObject msgEnvelope;
	msgEnvelope["dm_annotation"] = envelope.dmAnnotation;
	msgEnvelope["dm_recipient_ident"] = envelope.dmRecipientIdent;
	msgEnvelope["dm_recipient_ref_number"] = envelope.dmRecipientRefNumber;
	msgEnvelope["dm_sender_ident"] = envelope.dmSenderIdent;
	msgEnvelope["dm_sender_ref_number"] = envelope.dmSenderRefNumber;
	msgEnvelope["dm_legal_title_law"] = envelope.dmLegalTitleLaw;
	msgEnvelope["dm_legal_title_par"] = envelope.dmLegalTitlePar;
	msgEnvelope["dm_legal_title_point"] = envelope.dmLegalTitlePoint;
	msgEnvelope["dm_legal_title_sect"] = envelope.dmLegalTitleSect;
	msgEnvelope["dm_legal_title_year"] = envelope.dmLegalTitleYear;
	msgEnvelope["dm_personal_delivery"] = envelope.dmPersonalDelivery;
	msgEnvelope["dm_allow_subst_delivery"] = envelope.dmAllowSubstDelivery;
	msgEnvelope["account"] = accountID;
	msgEnvelope["rcpt"] = recipienList;

	netmanager.createPostRequestWebDatovka(
	    QUrl(QString(WEBDATOVKA_SERVICE_URL) + "saveenvelope"), sessionid,
	    QJsonDocument(msgEnvelope).toJson(QJsonDocument::Compact),
	    reply);

	if (reply.isEmpty()) {
		errStr = tr("Reply content missing");
		return false;
	}
	QJsonDocument jsonResponse = QJsonDocument::fromJson(reply);
	QJsonObject jsonObject = jsonResponse.object();
	if (!jsonObject["success"].toBool()) {
		errStr = jsonObject["errmsg"].toString();
		return false;
	}

	int draftId = jsonObject["draft_id"].toInt();

	/* TODO - check returned bool */
	foreach (const JsonLayer::File &file, fileList) {
		netmanager.createPostRequestWebDatovkaSendFile(
		    QUrl(QString(WEBDATOVKA_SERVICE_URL) + "uploadfile"),
		    sessionid, draftId, file.fName, file.fContent, reply);

		if (reply.isEmpty()) {
			errStr = tr("Reply content missing");
			return false;
		}

		jsonResponse = QJsonDocument::fromJson(reply);
		jsonObject = jsonResponse.object();
		if (!jsonObject["success"].toBool()) {
			errStr = jsonObject["errmsg"].toString();
			return false;
		}
	}

	QJsonObject rootObj;
	rootObj["draft_id"] = draftId;
	netmanager.createPostRequestWebDatovka(
	    QUrl(QString(WEBDATOVKA_SERVICE_URL) + "senddraft"), sessionid,
	    QJsonDocument(rootObj).toJson(QJsonDocument::Compact),
	    reply);

	if (reply.isEmpty()) {
		errStr = tr("Reply content missing");
		return false;
	}

	jsonResponse = QJsonDocument::fromJson(reply);
	jsonObject = jsonResponse.object();
	if (!jsonObject["success"].toBool()) {
		errStr = jsonObject["errmsg"].toString();
		return false;
	}

	QString result;
	QJsonArray errorArray = jsonObject["errors"].toArray();
	foreach (const QJsonValue &value, errorArray) {
		QJsonObject obj = value.toObject();
		result = obj["dbID"].toString() + "§" + obj["msg"].toString();
		resultList.append(result);
	}

	return true;
}



bool JsonLayer::parseAccountList(const QByteArray &content,
    QList<JsonLayer::AccountData> &accountList, QString &errStr)
{
	QJsonDocument jsonResponse = QJsonDocument::fromJson(content);
	QJsonObject jsonObject = jsonResponse.object();
	if (!jsonObject["success"].toBool()) {
		errStr = jsonObject["errmsg"].toString();
		return false;
	}

	QJsonArray jsonArray = jsonObject["accountsList"].toArray();

	foreach (const QJsonValue &value, jsonArray) {
		QJsonObject obj = value.toObject();
		JsonLayer::AccountData aData;
		aData.userId = obj["userid"].toInt();
		aData.accountId = obj["id"].toInt();
		aData.name = obj["name"].toString();
		QJsonObject owner = obj["owner"].toObject();
		QJsonObject user = obj["user"].toObject();

		aData.ownerInfo.key = owner["id"].toString();
		aData.ownerInfo.dbID = owner["dbID"].toString();
		aData.ownerInfo.dbType = owner["dbType"].toString();
		aData.ownerInfo.ic = owner["ic"].toString();
		aData.ownerInfo.pnFirstName = owner["pnFirstName"].toString();
		aData.ownerInfo.pnMiddleName = owner["pnMiddleName"].toString();
		aData.ownerInfo.pnLastName = owner["pnLastName"].toString();
		aData.ownerInfo.pnLastNameAtBirth = owner["pnLastNameAtBirth"].toString();
		aData.ownerInfo.firmName = owner["firmName"].toString();
		aData.ownerInfo.biDate = owner["biDate"].toString();
		aData.ownerInfo.biCity = owner["biCity"].toString();
		aData.ownerInfo.biCounty = owner["biCounty"].toString();
		aData.ownerInfo.biState = owner["biState"].toString();
		aData.ownerInfo.adCity = owner["adCity"].toString();
		aData.ownerInfo.adStreet = owner["adStreet"].toString();
		aData.ownerInfo.adNumberInStreet = owner["adNumberInStreet"].toString();
		aData.ownerInfo.adNumberInMunicipality = owner["adNumberInMunicipality"].toString();
		aData.ownerInfo.adZipCode = owner["adZipCode"].toString();
		aData.ownerInfo.adState = owner["adState"].toString();
		aData.ownerInfo.nationality = owner["nationality"].toString();
		aData.ownerInfo.identifier = owner["identifier"].toString();
		aData.ownerInfo.registryCode = owner["registryCode"].toString();
		aData.ownerInfo.dbState = owner["dbState"].toInt();
		aData.ownerInfo.dbEffectiveOVM = owner["dbEffectiveOVM"].toBool();
		aData.ownerInfo.dbOpenAddressing = owner["dbOpenAddressing"].toBool();

		aData.userInfo.key = user["id"].toString();
		aData.userInfo.pnFirstName = user["pnFirstName"].toString();
		aData.userInfo.pnMiddleName = user["pnMiddleName"].toString();
		aData.userInfo.pnLastName = user["pnLastName"].toString();
		aData.userInfo.pnFirstName = user["pnFirstName"].toString();
		aData.userInfo.pnMiddleName = user["pnMiddleName"].toString();
		aData.userInfo.pnLastName = user["pnLastName"].toString();
		aData.userInfo.pnLastNameAtBirth = user["pnLastNameAtBirth"].toString();
		aData.userInfo.adCity = user["adCity"].toString();
		aData.userInfo.adStreet = user["adStreet"].toString();
		aData.userInfo.adNumberInStreet = user["adNumberInStreet"].toString();
		aData.userInfo.adNumberInMunicipality = user["adNumberInMunicipality"].toString();
		aData.userInfo.adZipCode = user["adZipCode"].toString();
		aData.userInfo.adState = user["adState"].toString();
		aData.userInfo.biDate = user["biDate"].toString();
		aData.userInfo.userType = user["userType"].toString();
		aData.userInfo.userPrivils = user["userPrivils"].toInt();
		aData.userInfo.ic = user["ic"].toInt();
		aData.userInfo.firmName = user["firmName"].toString();
		aData.userInfo.caStreet = user["caStreet"].toString();
		aData.userInfo.caCity = user["caCity"].toString();
		aData.userInfo.caZipCode = user["caZipCode"].toString();
		aData.userInfo.caState = user["caState"].toString();

		accountList.append(aData);
	}

	return true;
}


bool JsonLayer::parseMessageList(const QByteArray &content,
    QList<JsonLayer::Envelope> &messageList, QString &errStr)
{
	QJsonDocument jsonResponse = QJsonDocument::fromJson(content);
	QJsonObject jsonObject = jsonResponse.object();
	if (!jsonObject["success"].toBool()) {
		errStr = jsonObject["errmsg"].toString();
		return false;
	}

	QJsonArray messageArray = jsonObject["messages"].toArray();

	foreach (const QJsonValue &value, messageArray) {
		QJsonObject obj = value.toObject();
		Envelope mEvnelope;
		mEvnelope._tagList.clear();
		mEvnelope._read = obj["read"].toBool();

		QJsonObject envel = obj["envelope"].toObject();
		mEvnelope.id = envel["id"].toInt();
		mEvnelope.dmID = envel["dm_id"].toString().toLongLong();
		mEvnelope.dbIDRecipient = envel["db_id_recipient"].toString();
		mEvnelope.dbIDSender = envel["db_id_sender"].toString();
		mEvnelope.dmAcceptanceTime = envel["dm_acceptance_time"].toString();
		mEvnelope.dmAllowSubstDelivery = envel["dm_allow_subst_delivery"].toBool();
		mEvnelope.dmAmbiguousRecipient = envel["dm_ambiguous_recipient"].toBool();
		mEvnelope.dmAnnotation = envel["dm_annotation"].toString();
		mEvnelope.dmAttachmentSize = envel["dm_attachment_size"].toInt();
		mEvnelope.dmDeliveryTime = envel["dm_delivery_time"].toString();
		mEvnelope.dmLegalTitleLaw = envel["dm_legal_title_law"].toString();
		mEvnelope.dmLegalTitlePar = envel["dm_legal_title_par"].toString();
		mEvnelope.dmLegalTitlePoint = envel["dm_legal_title_point"].toString();
		mEvnelope.dmLegalTitleSect = envel["dm_legal_title_sect"].toString();
		mEvnelope.dmLegalTitleYear = envel["dm_legal_title_year"].toString();
		mEvnelope.dmMessageStatus = envel["dm_message_status"].toInt();
		mEvnelope.dmPersonalDelivery = envel["dm_personal_delivery"].toBool();
		mEvnelope.dmRecipient = envel["dm_recipient"].toString();
		mEvnelope.dmRecipientAddress = envel["dm_recipient_address"].toString();
		mEvnelope.dmRecipientIdent = envel["dm_recipient_ident"].toString();
		mEvnelope.dmRecipientOrgUnit = envel["dm_recipient_org_unit"].toString();
		mEvnelope.dmRecipientOrgUnitNum = envel["dm_recipient_org_unit_num"].toString();
		mEvnelope.dmRecipientRefNumber = envel["dm_recipient_ref_number"].toString();
		mEvnelope.dmSender = envel["dm_sender"].toString();
		mEvnelope.dmSenderAddress = envel["dm_sender_address"].toString();
		mEvnelope.dmSenderIdent = envel["dm_sender_ident"].toString();
		mEvnelope.dmSenderOrgUnit = envel["dm_sender_org_unit"].toString();
		mEvnelope.dmSenderOrgUnitNum = envel["dm_sender_org_unit_num"].toString();
		mEvnelope.dmSenderRefNumber = envel["dm_sender_ref_number"].toString();
		mEvnelope.dmSenderType = envel["dm_sender_type"].toInt();
		mEvnelope.dmToHands = envel["dm_to_hands"].toString();
		mEvnelope.dmType = envel["dm_type"].toInt();

		QJsonArray tagArray = obj["tags"].toArray();
		foreach (const QJsonValue &value, tagArray) {
			mEvnelope._tagList.append(value.toInt());
		}

		messageList.append(mEvnelope);
	}

	return true;
}


bool JsonLayer::parseSyncAccount(const QByteArray &content, QString &errStr)
{
	QJsonDocument jsonResponse = QJsonDocument::fromJson(content);
	QJsonObject jsonObject = jsonResponse.object();
	if (!jsonObject["success"].toBool()) {
		errStr = jsonObject["errmsg"].toString();
		return false;
	}

	return true;
}


bool JsonLayer::parseTagList(const QByteArray &content,
    QList<JsonLayer::Tag> &tagList, QString &errStr)
{
	QJsonDocument jsonResponse = QJsonDocument::fromJson(content);
	QJsonObject jsonObject = jsonResponse.object();
	if (!jsonObject["success"].toBool()) {
		errStr = jsonObject["errmsg"].toString();
		return false;
	}

	QJsonArray tagArray = jsonObject["tags"].toArray();

	foreach (const QJsonValue &value, tagArray) {
		QJsonObject obj = value.toObject();
		Tag tag;
		tag.id = obj["id"].toInt();
		tag.name = obj["name"].toString();
		tag.color = obj["color"].toString();
		tagList.append(tag);
	}

	return true;
}


bool JsonLayer::parseSearchRecipient(const QByteArray &content,
    QList<JsonLayer::Recipient> &resultList, bool &hasMore,
    QString &errStr)
{
	QJsonDocument jsonResponse = QJsonDocument::fromJson(content);
	QJsonObject jsonObject = jsonResponse.object();
	if (!jsonObject["success"].toBool()) {
		errStr = jsonObject["errmsg"].toString();
		return false;
	}

	hasMore = jsonObject["hasMore"].toBool();

	QJsonArray recipientArray = jsonObject["results"].toArray();

	foreach (const QJsonValue &value, recipientArray) {
		QJsonObject obj = value.toObject();
		Recipient rec;
		rec.recipientDbId = obj["id"].toString();
		rec.recipientName = obj["name"].toString();
		rec.recipientAddress = obj["address"].toString();
		rec.toHands = "";
		resultList.append(rec);
	}

	return true;
}
