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

#ifndef _JSON_H_
#define _JSON_H_

#include <QByteArray>
#include <QObject>
#include <QNetworkCookie>

#include "src/models/accounts_model.h"
#include "src/io/message_db.h"

class JsonLayer : public QObject {
	Q_OBJECT

public:

	/*!
	 * @brief Holds information about owner and his databox.
	 */
	struct OwnerInfo {
	public:
		QString key;
		QString dbID;
		QString dbType;
		QString ic;
		QString pnFirstName;
		QString pnMiddleName;
		QString pnLastName;
		QString pnLastNameAtBirth;
		QString firmName;
		QString biDate;
		QString biCity;
		QString biCounty;
		QString biState;
		QString adCity;
		QString adStreet;
		QString adNumberInStreet;
		QString adNumberInMunicipality;
		QString adZipCode;
		QString adState;
		QString nationality;
		QString identifier;
		QString registryCode;
		int dbState;
		bool dbEffectiveOVM;
		bool dbOpenAddressing;
	};

	/*!
	 * @brief Holds information about an user.
	 */
	struct UserInfo {
	public:
		QString key;
		QString pnFirstName;
		QString pnMiddleName;
		QString pnLastName;
		QString pnLastNameAtBirth;
		QString adCity;
		QString adStreet;
		QString adNumberInStreet;
		QString adNumberInMunicipality;
		QString adZipCode;
		QString adState;
		QString biDate;
		QString userType;
		int userPrivils;
		int ic;
		QString firmName;
		QString caStreet;
		QString caCity;
		QString caZipCode;
		QString caState;
	};

	/*!
	 * @brief Holds information about an tag properties.
	 */
	struct Tag {
	public:
		int id;
		QString name;
		QString color;
	};

	/*!
	 * @brief Holds data about recipient
	 */
	struct Recipient {
	public:
		QString recipientDbId;
		QString recipientName;
		QString recipientAddress;
		QString toHands;
	};

	/*!
	 * @brief Holds file data
	 */
	struct File {
	public:
		QString fName;
		QByteArray fContent;
	};

	class Envelope {
	public:
		int id;
		qint64 dmID;
		QString dbIDSender;
		QString dmSender;
		QString dmSenderAddress;
		int dmSenderType;
		QString dmRecipient;
		QString dmRecipientAddress;
		QString dmAmbiguousRecipient;
		QString dmSenderOrgUnit;
		QString dmSenderOrgUnitNum;
		QString dbIDRecipient;
		QString dmRecipientOrgUnit;
		QString dmRecipientOrgUnitNum;
		QString dmToHands;
		QString dmAnnotation;
		QString dmRecipientRefNumber;
		QString dmSenderRefNumber;
		QString dmRecipientIdent;
		QString dmSenderIdent;
		QString dmLegalTitleLaw;
		QString dmLegalTitleYear;
		QString dmLegalTitleSect;
		QString dmLegalTitlePar;
		QString dmLegalTitlePoint;
		bool dmPersonalDelivery;
		bool dmAllowSubstDelivery;
		QString dmQTimestamp;
		QString dmDeliveryTime;
		QString dmAcceptanceTime;
		int dmMessageStatus;
		int dmAttachmentSize;
		QString dmType;
		bool _read;
		QList<int> _tagList;
	};

	/*!
	 * @brief Holds account data and user info from webdatovka.
	 */
	struct AccountData {
	public:
		int userId;
		int accountId;
		QString name;
		JsonLayer::OwnerInfo ownerInfo;
		JsonLayer::UserInfo userInfo;
	};

	JsonLayer(QObject *parent = 0);
	~JsonLayer(void);


	QNetworkCookie fakeLoginWebDatovka(void);

	QNetworkCookie loginToMojeID(const QString &username,
	    const QString &pwd, const QString &otp);

	QNetworkCookie startLoginToWebDatovka(void);

	bool pingServer(const QString &userName, QString &errStr);

	bool createAccount(const QString &userName,
	    const QString &name, QString &errStr);

	bool renameAccount(const QString &userName,
	    int accountID, const QString &newName, QString &errStr);

	bool deleteAccount(const QString &userName,
	    int accountID, QString &errStr);

	bool getAccountList(const QNetworkCookie &sessionid,
	    QList<JsonLayer::AccountData> &accountList,
	    QString &errStr);

	bool getMessageList(const QString &userName,
	    int accountID, int messageType, int limit,
	    int offset, QList<JsonLayer::Envelope> &messageList,
	    QString &errStr);

	bool syncAccount(const QString &userName,
	    int accountID, QString &errStr);

	QByteArray downloadMessage(const QString &userName,
	    int msgId, QString &errStr);

	QByteArray downloadFile(const QString &userName,
	    int fileId, QString &errStr);

	bool getTagList(const QString &userName,
	    QList<JsonLayer::Tag> &tagList, QString &errStr);

	int createTag(const QString &userName,
	    const QString &name, const QString &color, QString &errStr);

	bool updateTag(const QString &userName, int tagId,
	    const QString &name, const QString &color, QString &errStr);

	bool deleteTag(const QString &userName,
	    int tagId, QString &errStr);

	bool assignTag(const QString &userName,
	    int tagId, int msgId, QString &errStr);

	bool removeTag(const QString &userName,
	    int tagId, int msgId, QString &errStr);

	bool removeAllTags(const QString &userName,
	    int msgId, QString &errStr);

	bool searchRecipient(const QString &userName,
	    int accountID, const QString &word, int position,
	    QList<JsonLayer::Recipient> &resultList, bool &hasMore,
	    QString &errStr);

	bool sendMessageAsJson(const QString &userName, int accountID,
	    const QList<JsonLayer::Recipient> &recipientList,
	    const JsonLayer::Envelope &envelope,
	    const QList<JsonLayer::File> &fileList,
	    QStringList &resultList, QString &errStr);

	bool sendMessage(const QString &userName, int accountID,
	    const QList<JsonLayer::Recipient> &recipientList,
	    const JsonLayer::Envelope &envelope,
	    const QList<JsonLayer::File> &fileList,
	    QStringList &resultList, QString &errStr);
private:

	bool isLoggedToWebDatovka(const QString &userName,
	    QNetworkCookie &sessionid);

	bool parseAccountList(const QByteArray &content,
	    QList<JsonLayer::AccountData> &accountList, QString &errStr);

	bool parseMessageList(const QByteArray &content,
	    QList<JsonLayer::Envelope> &messageList, QString &errStr);

	bool parseSyncAccount(const QByteArray &content, QString &errStr);

	bool parseTagList(const QByteArray &content,
	    QList<JsonLayer::Tag> &tagList, QString &errStr);

	bool parseSearchRecipient(const QByteArray &content,
	    QList<JsonLayer::Recipient> &resultList, bool &hasMore,
	    QString &errStr);
};

extern JsonLayer jsonlayer;

#endif /* _JSON_H_ */
