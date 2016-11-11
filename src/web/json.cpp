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
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QInputDialog>

#include "src/common.h"
#include "src/web/json.h"
#include "src/web/net.h"
#include "src/io/wd_sessions.h"
#include "src/gui/dlg_login_mojeid.h"

JsonLayer jsonlayer;

JsonLayer::JsonLayer(QObject *parent)
    : QObject(parent)
{
}

JsonLayer::~JsonLayer(void)
{
}

QNetworkCookie JsonLayer::fakeLoginWebDatovka(void)
{
	QUrl url(QString(WEBDATOVKA_SERVICE_URL) + "desktoplogin");
	QByteArray reply;
	QNetworkCookie sessionid;
	netmanager.createGetRequestWebDatovka(url, sessionid, reply);

	for (int i = 0; i < cookieList.size(); ++i) {
		if (cookieList.at(i).name() == COOKIE_SESSION_ID) {
			sessionid = cookieList.at(i);
		}
	}

	return sessionid;
}


bool JsonLayer::startLoginToWebDatovka(QUrl &lastUrl)
{
	QByteArray reply;
	QNetworkCookie sessionid;
	QByteArray postData;

	// STEP 1: call webdatovka for start login procedure (GET).
	//         We should obtain url for redirect
	//         and openconnect parameter list (postdata).
	QUrl url(QString(WEBDATOVKA_SERVICE_URL) + "desktoplogin");
	netmanager.createPostRequestWebDatovka(url, sessionid, postData, reply);
	lastUrl = url;
	QJsonDocument jsonResponse = QJsonDocument::fromJson(reply);
	QJsonObject jsonObject = jsonResponse.object();
	url.setUrl(jsonObject["url"].toString());
	if (!url.isValid()) {
		return false;
	}

	// Create formdata string from json
	QJsonObject postdata = jsonObject["postdata"].toObject();
	QVariantMap map = postdata.toVariantMap();
	QVariantMap::iterator i;
	for (i = map.begin(); i != map.end(); ++i) {
		postData.append(i.key()).append("=").append(i.value().toString()).append("&");
	}

	// STEP 2: Call mojeID for starting of login procedure (POST).
	//         We send openconnect parameter list as postdata.
	netmanager.createPostRequestMojeId(url, lastUrl, postData, reply);
	lastUrl = url;

	return true;
}


bool JsonLayer::loginMethodChanged(int method, QString &lastUrl, QString &token)
{
	QByteArray reply;
	QUrl url;
	if (method == USER_NAME) {
		url.setUrl(MOJEID_URL_PASSWORD);
	} else if (method == CERTIFICATE) {
		url.setUrl(MOJEID_URL_CERTIFICATE);
	} else {
		url.setUrl(MOJEID_URL_OTP);
	}

	// STEP 3: Call to mojeID on new login endpoint (GET).
	//          We should obtain html page. From page we must get token.
	netmanager.createGetRequestMojeId(url, QUrl(lastUrl), reply);
	lastUrl = url.toString();

	QString html(reply);
	QRegularExpression exp("<input +type=\"hidden\" +name=\"token\" +value=\"([^\"]*)\"");
	// Save new token string from webpage
	QRegularExpressionMatch * match = new QRegularExpressionMatch();
	if (html.contains(exp, match)) {
		token = match->captured(1);
	}

	return true;
}

bool JsonLayer::loginToMojeID(const QString &lastUrl,
   const QString &token, const QString &username,
    const QString &pwd, const QString &otp, const QString &certPath,
    QString &errStr, QNetworkCookie &sessionid)
{
	QByteArray reply;
	QUrl lUrl(lastUrl);
	QUrl url;

	errStr = tr("Login to mojeID failed. You must choose correct "
	    "login method and enter correct login data. Try again.");

	// STEP 4: Send credential to mojeID endpoint (POST).
	//         We send login data, csrfmiddlewaretoken
	//         and mojeid token as content. OTP or certificate data optionaly.
	url.setUrl(MOJEID_URL_CERTIFICATE);

	QString str = "csrfmiddlewaretoken=";
	for (int i = 0; i < cookieList.size(); ++i) {
		if (cookieList.at(i).name() == COOKIE_CSRFTOKEN) {
			str += cookieList.at(i).value();
		}
	}
	str += "&token=" + token;
	str += "&username=" + username;
	if (!pwd.isEmpty()) {
		url.setUrl(MOJEID_URL_PASSWORD);
		str += "&password=" + pwd;
	}
	if (!otp.isEmpty()) {
		url.setUrl(MOJEID_URL_OTP);
		str += "&otp_token=" + otp;
	}
	str += "&allow=prihlasit+se";
	QByteArray data;
	data.append(str);
	netmanager.createPostRequestMojeId(url, lUrl, data, reply);
	lUrl = url;

	if (lUrl.toString() == MOJEID_URL_CERTIFICATE) {

		url.setUrl(MOJEID_URL_SSLLOGIN);

		QFile certFile(certPath);
		if (!certFile.open(QIODevice::ReadOnly)) {
			errStr = tr("Cannot open client certificate from path '%1'").arg(certPath);
			return false;
		}

		bool ok;
		const QString passPhrase = QInputDialog::getText(0,
		    tr("Certificate password"),
		    tr("Enter certificate password:"),
		    QLineEdit::Password, NULL, &ok);

		QFileInfo finfo(certFile);
		QString ext = finfo.suffix().toLower();
		if (ext == "pem") {
			QByteArray certData = certFile.readAll();
			QSslCertificate cert(certData, QSsl::Pem);
			QSslKey key(certData, QSsl::Rsa, QSsl::Pem,
			    QSsl::PrivateKey, passPhrase.toUtf8());
			netmanager.createPostRequestMojeIdCert(url, lUrl,
			    data, cert, key, reply);
		// is PKCS12 format
		} else if (ext == "p12" || ext == "pfx") {
			QSslCertificate cert;
			QSslKey key;
			QList<QSslCertificate> importedCerts;
			if (QSslCertificate::importPkcs12(&certFile, &key,
			    &cert, &importedCerts, passPhrase.toUtf8())) {
				netmanager.createPostRequestMojeIdCert(url,
				    lUrl, data, cert, key, reply);
			} else {
				errStr = tr("Cannot parse client certificate from path '%1'").arg(certPath);
				return false;
			}
		}
		lUrl = url;
	}

	// STEP 5: send confiramtion to mojeID endpoint (POST).
	//         We send new csrfmiddlewaretoken
	//         and mojeid token and other values as content.
	url.setUrl(MOJEID_URL_CONFIRM);
	str = "csrfmiddlewaretoken=";
	for (int i = 0; i < cookieList.size(); ++i) {
		if (cookieList.at(i).name() == COOKIE_CSRFTOKEN) {
			str += cookieList.at(i).value();
		}
	}
	str += "&token=" + token;
	str += "&username=" + username;
	str += "&first_name=first_name";
	str += "&last_name=last_name";
	str += "&email_default=email_default";
	str += "&allow=OK";
	data.clear();
	data.append(str);
	netmanager.createPostRequestMojeId(url, lUrl, data, reply);
	lUrl = url;

	/* TODO - repeat login proccess if confirmation failed */

	for (int i = 0; i < cookieList.size(); ++i) {
		if (cookieList.at(i).name() == COOKIE_SESSION_ID) {
			sessionid = cookieList.at(i);
		}
	}

	// STEP 6: redirect mojeID responce to webdatovka (GET).
	//         We send mojeid parametrs in get url.
	url.setUrl(netmanager.newUrl);
	netmanager.createGetRequestWebDatovka(url, sessionid, reply);

	QJsonDocument jsonResponse = QJsonDocument::fromJson(reply);
	QJsonObject jsonObject = jsonResponse.object();
	if (!jsonObject["success"].toBool()) {
		if (!jsonObject["errmsg"].toString().isEmpty()) {
			errStr = jsonObject["errmsg"].toString();
		}
	} else {
		if (!jsonObject["warning"].toString().isEmpty()) {
			errStr = jsonObject["warning"].toString();
		}
		// Now, you are logged to webdatovka and you have sessionid cookie.
		// Next requests/operations only via new sessionid.
		for (int i = 0; i < cookieList.size(); ++i) {
			if (cookieList.at(i).name() == COOKIE_SESSION_ID) {
				sessionid = cookieList.at(i);
			}
		}
		return true;
	}

	return false;
}


bool JsonLayer::isLoggedToWebDatovka(const QString &userName,
    QNetworkCookie &sessionid)
{
	sessionid = wdSessions.sessionCookie(userName);
	if (sessionid.value().isEmpty()) {
		return false;
	}

	return true;
}


bool JsonLayer::updateSessionId(const QString &userName)
{
	QNetworkCookie sessionid;

	for (int i = 0; i < cookieList.size(); ++i) {
		if (cookieList.at(i).name() == COOKIE_SESSION_ID) {
			sessionid = cookieList.at(i);
		}
	}

	return wdSessions.setSessionCookie(userName, sessionid);
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

	updateSessionId(userName);

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

	updateSessionId(userName);

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


bool JsonLayer::renameAccount(const QString &userName, int accountID,
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

	updateSessionId(userName);


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

	updateSessionId(userName);

	QJsonDocument jsonResponse = QJsonDocument::fromJson(reply);
	QJsonObject jsonObject = jsonResponse.object();
	if (!jsonObject["success"].toBool()) {
		errStr = jsonObject["errmsg"].toString();
		return false;
	}

	return true;
}


bool JsonLayer::getAccountList(const QNetworkCookie &sessionid, int &userId,
    QList<JsonLayer::AccountData> &accountList,  QString &errStr)
{
	QByteArray reply;

	netmanager.createGetRequestWebDatovka(
	    QUrl(QString(WEBDATOVKA_SERVICE_URL) + "accountlist"), sessionid,
	    reply);

	if (reply.isEmpty()) {
		return false;
	}

	return parseAccountList(reply, userId, accountList, errStr);
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

	updateSessionId(userName);

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

	updateSessionId(userName);

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

	updateSessionId(userName);

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

	updateSessionId(userName);

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

	updateSessionId(userName);

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

	updateSessionId(userName);

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

	updateSessionId(userName);

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

	updateSessionId(userName);

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

	updateSessionId(userName);

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

	updateSessionId(userName);

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

	updateSessionId(userName);

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

	updateSessionId(userName);

	if (reply.isEmpty()) {
		errStr = tr("Reply content missing");
		return false;
	}

	return parseSearchRecipient(reply, resultList, hasMore, errStr);
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
	if (!envelope.dmRecipientIdent.isEmpty()) {
		msgEnvelope["dm_recipient_ident"] = envelope.dmRecipientIdent;
	}
	if (!envelope.dmRecipientRefNumber.isEmpty()) {
		msgEnvelope["dm_recipient_ref_number"] =
		    envelope.dmRecipientRefNumber;
	}
	if (!envelope.dmRecipientRefNumber.isEmpty()) {
		msgEnvelope["dm_sender_ident"] = envelope.dmSenderIdent;
	}
	if (!envelope.dmRecipientRefNumber.isEmpty()) {
		msgEnvelope["dm_sender_ref_number"] = envelope.dmSenderRefNumber;
	}
	if (!envelope.dmRecipientRefNumber.isEmpty()) {
		msgEnvelope["dm_legal_title_law"] = envelope.dmLegalTitleLaw;
	}
	if (!envelope.dmRecipientRefNumber.isEmpty()) {
		msgEnvelope["dm_legal_title_par"] = envelope.dmLegalTitlePar;
	}
	if (!envelope.dmRecipientRefNumber.isEmpty()) {
		msgEnvelope["dm_legal_title_point"] = envelope.dmLegalTitlePoint;
	}
	if (!envelope.dmRecipientRefNumber.isEmpty()) {
		msgEnvelope["dm_legal_title_sect"] = envelope.dmLegalTitleSect;
	}
	if (!envelope.dmRecipientRefNumber.isEmpty()) {
		msgEnvelope["dm_legal_title_year"] = envelope.dmLegalTitleYear;
	}
	msgEnvelope["dm_personal_delivery"] = envelope.dmPersonalDelivery;
	msgEnvelope["dm_allow_subst_delivery"] = envelope.dmAllowSubstDelivery;
	msgEnvelope["dm_publish_own_id"] = envelope.dmPublishOwnID;
	msgEnvelope["dm_ovm"] = envelope.dmOVM;
	msgEnvelope["account"] = accountID;
	msgEnvelope["rcpt"] = recipienList;

	netmanager.createPostRequestWebDatovka(
	    QUrl(QString(WEBDATOVKA_SERVICE_URL) + "saveenvelope"), sessionid,
	    QJsonDocument(msgEnvelope).toJson(QJsonDocument::Compact),
	    reply);

	updateSessionId(userName);

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
		    QUrl(QString(WEBDATOVKA_SERVICE_URL) + "uploadonefile"),
		    sessionid, draftId, file.fName,
		    QByteArray::fromBase64(file.fContent), reply);

		updateSessionId(userName);

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

	updateSessionId(userName);

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


bool JsonLayer::deleteMessage(const QString &userName, int msgId,
    QString &errStr)
{
	QByteArray reply;
	QNetworkCookie sessionid;

	if (!isLoggedToWebDatovka(userName, sessionid)) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QJsonObject rootObj;
	QJsonArray array;
	array.append(msgId);
	rootObj["msg_id"] = array;

	netmanager.createPostRequestWebDatovka(
	    QUrl(QString(WEBDATOVKA_SERVICE_URL) + "deletemessage"), sessionid,
	    QJsonDocument(rootObj).toJson(QJsonDocument::Compact),
	    reply);

	updateSessionId(userName);

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


bool JsonLayer::markMessageAsRead(const QString &userName, int msgId,
    QString &errStr)
{
	QByteArray reply;
	QNetworkCookie sessionid;

	if (!isLoggedToWebDatovka(userName, sessionid)) {
		errStr = tr("User is not logged to mojeID");
		return false;
	}

	QJsonObject rootObj;
	QJsonArray array;
	array.append(msgId);
	rootObj["msg_id"] = array;

	netmanager.createPostRequestWebDatovka(
	    QUrl(QString(WEBDATOVKA_SERVICE_URL) + "markasread"), sessionid,
	    QJsonDocument(rootObj).toJson(QJsonDocument::Compact),
	    reply);

	updateSessionId(userName);

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

bool JsonLayer::parseAccountList(const QByteArray &content, int &userId,
    QList<JsonLayer::AccountData> &accountList, QString &errStr)
{
	QJsonDocument jsonResponse = QJsonDocument::fromJson(content);
	QJsonObject jsonObject = jsonResponse.object();
	if (!jsonObject["success"].toBool()) {
		errStr = jsonObject["errmsg"].toString();
		return false;
	}

	userId = jsonObject["userId"].toInt();
	QJsonArray jsonArray = jsonObject["accountsList"].toArray();

	foreach (const QJsonValue &value, jsonArray) {
		QJsonObject obj = value.toObject();
		JsonLayer::AccountData aData;
		aData.userId = userId;
		aData.accountId = obj["id"].toInt();
		aData.name = obj["name"].toString();
		QJsonObject owner = obj["owner"].toObject();
		QJsonObject user = obj["user"].toObject();

		aData.ownerInfo.key = obj["id"].toString();
		aData.ownerInfo.dbID = owner["db_id"].toString();
		aData.ownerInfo.dbType = owner["db_type"].toString();
		aData.ownerInfo.ic = owner["ic_db"].toString().isEmpty() ?
		    NULL : owner["ic_db"].toString();
		aData.ownerInfo.pnFirstName =
		    owner["pn_first_name"].toString().isEmpty() ?
		    NULL : owner["pn_first_name"].toString();
		aData.ownerInfo.pnMiddleName =
		    owner["pn_middle_name"].toString().isEmpty() ?
		    NULL : owner["pn_middle_name"].toString();
		aData.ownerInfo.pnLastName =
		    owner["pn_last_name"].toString().isEmpty() ?
		    NULL : owner["pn_last_name"].toString();
		aData.ownerInfo.pnLastNameAtBirth =
		    owner["pn_last_name_at_birth"].toString().isEmpty() ?
		    NULL : owner["pn_last_name_at_birth"].toString();
		aData.ownerInfo.firmName =
		    owner["firm_name"].toString().isEmpty() ?
		    NULL : owner["firm_name"].toString();
		aData.ownerInfo.biDate =
		    owner["bi_date"].toString().isEmpty() ?
		    NULL : owner["bi_date"].toString();
		aData.ownerInfo.biCity =
		    owner["bi_city"].toString().isEmpty() ?
		    NULL : owner["bi_city"].toString();
		aData.ownerInfo.biCounty =
		    owner["bi_county"].toString().isEmpty() ?
		    NULL : owner["bi_county"].toString();
		aData.ownerInfo.biState =
		    owner["bi_state"].toString().isEmpty() ?
		    NULL : owner["bi_state"].toString();
		aData.ownerInfo.adCity =
		    owner["ad_city"].toString().isEmpty() ?
		    NULL : owner["ad_city"].toString();
		aData.ownerInfo.adStreet =
		    owner["ad_street"].toString().isEmpty() ?
		    NULL : owner["ad_street"].toString();
		aData.ownerInfo.adNumberInStreet =
		    owner["ad_number_in_street"].toString().isEmpty() ?
		    NULL : owner["ad_number_in_street"].toString();
		aData.ownerInfo.adNumberInMunicipality =
		    owner["ad_number_in_municipality"].toString().isEmpty() ?
		    NULL : owner["ad_number_in_municipality"].toString();
		aData.ownerInfo.adZipCode =
		    owner["ad_zip_code"].toString().isEmpty() ?
		    NULL : owner["ad_zip_code"].toString();
		aData.ownerInfo.adState =
		    owner["ad_state"].toString().isEmpty() ?
		    NULL : owner["ad_state"].toString();
		aData.ownerInfo.nationality =
		    owner["nationality"].toString().isEmpty() ?
		    NULL : owner["nationality"].toString();
		aData.ownerInfo.identifier =
		    owner["identifier"].toString().isEmpty() ?
		    NULL : owner["identifier"].toString();
		aData.ownerInfo.registryCode =
		    owner["registry_code"].toString().isEmpty() ?
		    NULL : owner["registry_code"].toString();
		aData.ownerInfo.dbState = owner["db_state"].toInt();
		aData.ownerInfo.dbEffectiveOVM =
		    owner["db_effective_ovm"].toBool();
		aData.ownerInfo.dbOpenAddressing =
		    owner["db_open_addressing"].toBool();

		aData.userInfo.key = obj["user_id"].toString();
		aData.userInfo.pnFirstName =
		    user["pn_first_name"].toString().isEmpty() ?
		    NULL : user["pn_first_name"].toString();
		aData.userInfo.pnMiddleName =
		    user["pn_middle_name"].toString().isEmpty() ?
		    NULL : user["pn_middle_name"].toString();
		aData.userInfo.pnLastName =
		    user["pn_last_name"].toString().isEmpty() ?
		    NULL : user["pn_last_name"].toString();
		aData.userInfo.pnLastNameAtBirth =
		    user["pn_last_name_at_birth"].toString().isEmpty() ?
		    NULL : user["pn_last_name_at_birth"].toString();
		aData.userInfo.adCity = user["ad_city"].toString().isEmpty() ?
		    NULL : user["ad_city"].toString();
		aData.userInfo.adStreet =
		    user["ad_street"].toString().isEmpty() ?
		    NULL : user["ad_street"].toString();
		aData.userInfo.adNumberInStreet =
		    user["ad_number_in_street"].toString().isEmpty() ?
		    NULL : user["ad_number_in_street"].toString();
		aData.userInfo.adNumberInMunicipality =
		    user["ad_number_in_municipality"].toString().isEmpty() ?
		    NULL : user["ad_number_in_municipality"].toString();
		aData.userInfo.adZipCode =
		    user["ad_zip_code"].toString().isEmpty() ?
		    NULL : user["ad_zip_code"].toString();
		aData.userInfo.adState =
		    user["ad_state"].toString().isEmpty() ?
		    NULL : user["ad_state"].toString();
		aData.userInfo.biDate = user["bi_date"].toString().isEmpty() ?
		    NULL : user["bi_date"].toString();
		aData.userInfo.userType =
		    user["user_type"].toString().isEmpty() ?
		    NULL : user["user_type"].toString();
		aData.userInfo.userPrivils = user["user_privils"].toInt();
		aData.userInfo.ic = user["ic_db"].toInt();
		aData.userInfo.firmName =
		    user["firm_name"].toString().isEmpty() ?
		    NULL : user["firm_name"].toString();
		aData.userInfo.caStreet =
		    user["ca_street"].toString().isEmpty() ?
		    NULL : user["ca_street"].toString();
		aData.userInfo.caCity = user["ca_city"].toString().isEmpty() ?
		    NULL : user["ca_city"].toString();
		aData.userInfo.caZipCode =
		    user["ca_zip_code"].toString().isEmpty() ?
		    NULL : user["ca_zip_code"].toString();
		aData.userInfo.caState =
		    user["ca_state"].toString().isEmpty() ?
		    NULL : user["ca_state"].toString();

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
