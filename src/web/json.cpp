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

JsonLayer jsonlayer;

JsonLayer::JsonLayer(QObject *parent)
    : QObject(parent)
{
}

JsonLayer::~JsonLayer(void)
{
}

/* TODO - mojeID login sekvence - only for testing */
void JsonLayer::mojeIDtest(void)
{
	QByteArray reply;

	QUrl url(MOJEID_BASE_URL);
	netmanager.createPostRequest(url, QByteArray("this_is_mojeid_form=1"),
	   reply);

	QUrl url2(MOJEID_BASE_URL2);
	netmanager.createPostRequest(url2, QByteArray(), reply);

	QUrl url3(MOJEID_BASE_URL3);
	netmanager.createPostRequest(url3, QByteArray(), reply);
}

bool JsonLayer::loginToWebDatovka(void) {

	QUrl url(WEBDATOVKA_LOGIN_URL);
	QByteArray reply;
	return netmanager.createGetRequest(url, reply);
}


bool JsonLayer::isLoggedToWebDatovka(void)
{
	if (cookie.name().isEmpty()) {
		return loginToWebDatovka();
	}

	return true;
}


bool JsonLayer::pingServer(QString &errStr)
{
	QByteArray reply;

	if (!isLoggedToWebDatovka()) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	netmanager.createGetRequest(QUrl(QString(WEBDATOVKA_SERVICE_URL)
	    + "ping"), reply);

	QJsonDocument jsonResponse = QJsonDocument::fromJson(reply);
	QJsonObject jsonObject = jsonResponse.object();
	if (!jsonObject["success"].toBool()) {
		errStr = jsonObject["errmsg"].toString();
		return false;
	}

	return true;
}


bool JsonLayer::createAccount(const QString &name, QString &errStr)
{
	QByteArray reply;

	if (!isLoggedToWebDatovka()) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QVariantMap vMap;
	vMap.insert("name", name);
	netmanager.createPostRequest(QUrl(QString(WEBDATOVKA_SERVICE_URL)
	    + "newaccount"), QJsonDocument::fromVariant(vMap).toJson(),
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


bool JsonLayer::renameAccount(int accountID, const QString &newName,
    QString &errStr)
{
	QByteArray reply;

	if (!isLoggedToWebDatovka()) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QVariantMap vMap;
	vMap.insert("account", accountID);
	vMap.insert("name", newName);
	netmanager.createPostRequest(QUrl(QString(WEBDATOVKA_SERVICE_URL)
	    + "renameaccount"), QJsonDocument::fromVariant(vMap).toJson(),
	    reply);

	QJsonDocument jsonResponse = QJsonDocument::fromJson(reply);
	QJsonObject jsonObject = jsonResponse.object();
	if (!jsonObject["success"].toBool()) {
		errStr = jsonObject["errmsg"].toString();
		return false;
	}

	return true;
}


bool JsonLayer::deleteAccount(int accountID, QString &errStr)
{
	QByteArray reply;

	if (!isLoggedToWebDatovka()) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QVariantMap vMap;
	vMap.insert("account", accountID);
	netmanager.createPostRequest(QUrl(QString(WEBDATOVKA_SERVICE_URL)
	    + "deleteaccount"), QJsonDocument::fromVariant(vMap).toJson(),
	    reply);

	QJsonDocument jsonResponse = QJsonDocument::fromJson(reply);
	QJsonObject jsonObject = jsonResponse.object();
	if (!jsonObject["success"].toBool()) {
		errStr = jsonObject["errmsg"].toString();
		return false;
	}

	return true;
}


bool JsonLayer::getAccountList(QList<JsonLayer::AccountInfo> &accountList,
    QString &errStr)
{
	QByteArray reply;

	if (!isLoggedToWebDatovka()) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	netmanager.createGetRequest(QUrl(QString(WEBDATOVKA_SERVICE_URL)
	    + "getsettings"), reply);

	if (reply.isEmpty()) {
		return false;
	}

	return parseAccountList(reply, accountList, errStr);
}


bool JsonLayer::getAccountInfo(int accountID,
    JsonLayer::AccountInfo &accountInfo, QString &errStr)
{
	QByteArray reply;

	if (!isLoggedToWebDatovka()) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QVariantMap vMap;
	vMap.insert("account", accountID);
	netmanager.createPostRequest(QUrl(QString(WEBDATOVKA_SERVICE_URL)
	    + "accountinfo"), QJsonDocument::fromVariant(vMap).toJson(), reply);

	if (reply.isEmpty()) {
		return false;
	}

	return parseAccountInfo(reply, accountInfo, errStr);
}


bool JsonLayer::getUserInfo(int accountID,
    JsonLayer::UserInfo &userInfo, QString &errStr)
{
	QByteArray reply;

	if (!isLoggedToWebDatovka()) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QVariantMap vMap;
	vMap.insert("account", accountID);
	netmanager.createPostRequest(QUrl(QString(WEBDATOVKA_SERVICE_URL)
	    + "userinfo"), QJsonDocument::fromVariant(vMap).toJson(), reply);

	if (reply.isEmpty()) {
		errStr = tr("Reply content missing");
		return false;
	}

	return parseUserInfo(reply, userInfo, errStr);
}


bool JsonLayer::getMessageList(int accountID, int messageType, int limit,
    int offset, QList<MsgEnvelope> &messageList, QString &errStr)
{
	QByteArray reply;

	if (!isLoggedToWebDatovka()) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QVariantMap vMap;
	vMap.insert("account", accountID);
	vMap.insert("folder", messageType);
	vMap.insert("offset", offset);
	vMap.insert("limit", limit);
	netmanager.createPostRequest(QUrl(QString(WEBDATOVKA_SERVICE_URL)
	    + "messageslist"), QJsonDocument::fromVariant(vMap).toJson(), reply);

	if (reply.isEmpty()) {
		errStr = tr("Reply content missing");
		return false;
	}

	return parseMessageList(reply, messageList, errStr);
}


bool JsonLayer::syncAccount(int accountID, QString &errStr)
{
	QByteArray reply;

	if (!isLoggedToWebDatovka()) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QVariantMap vMap;
	vMap.insert("account", accountID);
	netmanager.createPostRequest(QUrl(QString(WEBDATOVKA_SERVICE_URL)
	    + "synchronize"), QJsonDocument::fromVariant(vMap).toJson(), reply);

	if (reply.isEmpty()) {
		errStr = tr("Reply content missing");
		return false;
	}

	return parseSyncAccount(reply, errStr);
}


QByteArray JsonLayer::downloadMessage(int msgId, QString &errStr)
{
	QByteArray reply;

	if (!isLoggedToWebDatovka()) {
		errStr = tr("User is not logged to mojeID");
		return QByteArray();
	}

	netmanager.createGetRequest(QUrl(QString(WEBDATOVKA_SERVICE_URL)
	    + "downloadsigned/" + QString::number(msgId)), reply);

	return reply;
}


QByteArray JsonLayer::downloadFile(int fileId, QString &errStr)
{
	QByteArray reply;

	if (!isLoggedToWebDatovka()) {
		errStr = tr("User is not logged to mojeID");
		return QByteArray();
	}

	netmanager.createGetRequest(QUrl(QString(WEBDATOVKA_SERVICE_URL)
	    + "downloadfile/" + QString::number(fileId)), reply);

	return reply;
}


bool JsonLayer::getTagList(QList<JsonLayer::Tag> &tagList, QString &errStr)
{
	QByteArray reply;

	if (!isLoggedToWebDatovka()) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QVariantMap vMap;
	netmanager.createPostRequest(QUrl(QString(WEBDATOVKA_SERVICE_URL)
	    + "listtags"), QJsonDocument::fromVariant(vMap).toJson(), reply);

	if (reply.isEmpty()) {
		errStr = tr("Reply content missing");
		return false;
	}

	return parseTagList(reply, tagList, errStr);
}


int JsonLayer::createTag(const QString &name, const QString &color,
    QString &errStr)
{
	QByteArray reply;

	if (!isLoggedToWebDatovka()) {
		errStr = tr("User is not logged to mojeID");
		return -1;
	}

	QVariantMap vMap;
	vMap.insert("name", name);
	vMap.insert("color", color);
	netmanager.createPostRequest(QUrl(QString(WEBDATOVKA_SERVICE_URL)
	    + "newtag"), QJsonDocument::fromVariant(vMap).toJson(), reply);

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


bool JsonLayer::updateTag(int tagId, const QString &name,
    const QString &color, QString &errStr)
{
	QByteArray reply;

	if (!isLoggedToWebDatovka()) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QVariantMap vMap;
	vMap.insert("id", tagId);
	vMap.insert("name", name);
	vMap.insert("color", color);
	netmanager.createPostRequest(QUrl(QString(WEBDATOVKA_SERVICE_URL)
	    + "edittag"), QJsonDocument::fromVariant(vMap).toJson(), reply);

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


bool JsonLayer::deleteTag(int tagId, QString &errStr)
{
	QByteArray reply;

	if (!isLoggedToWebDatovka()) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QVariantMap vMap;
	vMap.insert("id", tagId);
	netmanager.createPostRequest(QUrl(QString(WEBDATOVKA_SERVICE_URL)
	    + "deletetag"), QJsonDocument::fromVariant(vMap).toJson(), reply);

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


bool JsonLayer::assignTag(int tagId, int msgId, QString &errStr)
{
	QByteArray reply;

	if (!isLoggedToWebDatovka()) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QVariantMap vMap;
	vMap.insert("id", tagId);
	vMap.insert("msgid", msgId);
	netmanager.createPostRequest(QUrl(QString(WEBDATOVKA_SERVICE_URL)
	    + "tagmsg/add"), QJsonDocument::fromVariant(vMap).toJson(), reply);

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


bool JsonLayer::removeTag(int tagId, int msgId, QString &errStr)
{
	QByteArray reply;

	if (!isLoggedToWebDatovka()) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QVariantMap vMap;
	vMap.insert("id", tagId);
	vMap.insert("msgid", msgId);
	netmanager.createPostRequest(QUrl(QString(WEBDATOVKA_SERVICE_URL)
	    + "tagmsg/remove"), QJsonDocument::fromVariant(vMap).toJson(),
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


bool JsonLayer::removeAllTags(int msgId, QString &errStr)
{
	QByteArray reply;

	if (!isLoggedToWebDatovka()) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QVariantMap vMap;
	vMap.insert("msgid", msgId);
	netmanager.createPostRequest(QUrl(QString(WEBDATOVKA_SERVICE_URL)
	    + "tagmsg/removeall"), QJsonDocument::fromVariant(vMap).toJson(),
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


bool JsonLayer::searchRecipient(const QString &word, int position,
    QList<JsonLayer::Recipient> &resultList, bool &hasMore,
    QString &errStr)
{
	QByteArray reply;

	if (!isLoggedToWebDatovka()) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QVariantMap vMap;
	vMap.insert("word", word);
	vMap.insert("position", position);
	netmanager.createPostRequest(QUrl(QString(WEBDATOVKA_SERVICE_URL)
	    + "searchrecipient"), QJsonDocument::fromVariant(vMap).toJson(),
	    reply);

	if (reply.isEmpty()) {
		errStr = tr("Reply content missing");
		return false;
	}

	return parseSearchRecipient(reply, resultList, hasMore, errStr);
}


bool JsonLayer::parseAccountList(const QByteArray &content,
    QList<JsonLayer::AccountInfo> &accountList, QString &errStr)
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
		JsonLayer::AccountInfo accountInfo;
		accountInfo.key = DB_MOJEID_NAME_PREFIX +
		    QString::number(obj["id"].toInt());
		if (!obj["name"].toString().isEmpty()) {
			accountInfo._acntName = obj["name"].toString();
		} else {
			accountInfo._acntName = obj["isdsName"].toString();
		}
		accountInfo.dbType = convertDbTypeToString(obj["boxType"].toInt());
		accountInfo.dbID = obj["boxID"].toString();
		if (obj.contains("ico")) {
			accountInfo.ic = obj["ico"].toString();
		}
		if (obj.contains("pdzReceiving")) {
			accountInfo.dbOpenAddressing = obj["pdzReceiving"].toBool();
		} else {
			accountInfo.dbOpenAddressing = false;
		}
		if (obj.contains("effectiveOVM")) {
			accountInfo.dbEffectiveOVM = obj["effectiveOVM"].toBool();
		} else {
			accountInfo.dbEffectiveOVM = false;
		}
		accountList.append(accountInfo);
	}

	return true;
}


bool JsonLayer::parseMessageList(const QByteArray &content,
    QList<MsgEnvelope> &messageList, QString &errStr)
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
		MsgEnvelope mEvnelope;
		mEvnelope._tagList.clear();
		mEvnelope.id = obj["id"].toInt();
		mEvnelope.dmDeliveryTime = obj["date"].toString();
		mEvnelope._read = obj["read"].toBool();
/*
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
*/
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


bool JsonLayer::parseAccountInfo(const QByteArray &content,
    JsonLayer::AccountInfo &accountInfo, QString &errStr)
{
	QJsonDocument jsonResponse = QJsonDocument::fromJson(content);
	QJsonObject jsonObject = jsonResponse.object();
	if (!jsonObject["success"].toBool()) {
		errStr = jsonObject["errmsg"].toString();
		return false;
	}

	QJsonObject account = jsonObject["account"].toObject();
	accountInfo.key = account["id"].toString();
	accountInfo.dbID = account["dbID"].toString();
	accountInfo.dbType = account["dbType"].toString();
	accountInfo.ic = account["ic"].toString();
	accountInfo.pnFirstName = account["pnFirstName"].toString();
	accountInfo.pnMiddleName = account["pnMiddleName"].toString();
	accountInfo.pnLastName = account["pnLastName"].toString();
	accountInfo.pnLastNameAtBirth = account["pnLastNameAtBirth"].toString();
	accountInfo.firmName = account["firmName"].toString();
	accountInfo.biDate = account["biDate"].toString();
	accountInfo.biCity = account["biCity"].toString();
	accountInfo.biCounty = account["biCounty"].toString();
	accountInfo.biState = account["biState"].toString();
	accountInfo.adCity = account["adCity"].toString();
	accountInfo.adStreet = account["adStreet"].toString();
	accountInfo.adNumberInStreet = account["adNumberInStreet"].toString();
	accountInfo.adNumberInMunicipality = account["adNumberInMunicipality"].toString();
	accountInfo.adZipCode = account["adZipCode"].toString();
	accountInfo.adState = account["adState"].toString();
	accountInfo.nationality = account["nationality"].toString();
	accountInfo.identifier = account["identifier"].toString();
	accountInfo.registryCode = account["registryCode"].toString();
	accountInfo.dbState = account["dbState"].toInt();
	accountInfo.dbEffectiveOVM = account["dbEffectiveOVM"].toBool();
	accountInfo.dbOpenAddressing = account["dbOpenAddressing"].toBool();

	return true;
}


bool JsonLayer::parseUserInfo(const QByteArray &content,
    JsonLayer::UserInfo &userInfo, QString &errStr)
{
	QJsonDocument jsonResponse = QJsonDocument::fromJson(content);
	QJsonObject jsonObject = jsonResponse.object();
	if (!jsonObject["success"].toBool()) {
		errStr = jsonObject["errmsg"].toString();
		return false;
	}

	QJsonObject user = jsonObject["user"].toObject();
	userInfo.key = user["id"].toString();
	userInfo.pnFirstName = user["pnFirstName"].toString();
	userInfo.pnMiddleName = user["pnMiddleName"].toString();
	userInfo.pnLastName = user["pnLastName"].toString();
	userInfo.pnFirstName = user["pnFirstName"].toString();
	userInfo.pnMiddleName = user["pnMiddleName"].toString();
	userInfo.pnLastName = user["pnLastName"].toString();
	userInfo.pnLastNameAtBirth = user["pnLastNameAtBirth"].toString();
	userInfo.adCity = user["adCity"].toString();
	userInfo.adStreet = user["adStreet"].toString();
	userInfo.adNumberInStreet = user["adNumberInStreet"].toString();
	userInfo.adNumberInMunicipality = user["adNumberInMunicipality"].toString();
	userInfo.adZipCode = user["adZipCode"].toString();
	userInfo.adState = user["adState"].toString();
	userInfo.biDate = user["biDate"].toString();
	userInfo.userType = user["userType"].toString();
	userInfo.userPrivils = user["userPrivils"].toInt();
	userInfo.ic = user["ic"].toInt();
	userInfo.firmName = user["firmName"].toString();
	userInfo.caStreet = user["caStreet"].toString();
	userInfo.caCity = user["caCity"].toString();
	userInfo.caZipCode = user["caZipCode"].toString();
	userInfo.caState = user["caState"].toString();

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
		rec.id = obj["id"].toInt();
		rec.name = obj["name"].toString();
		rec.address = obj["address"].toString();
		rec.type = obj["type"].toInt();
		rec.effectiveOVM = obj["effectiveOVM"].toBool();
		resultList.append(rec);
	}

	return true;
}
