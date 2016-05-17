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

#include "src/models/accounts_model.h"
#include "src/io/message_db.h"

class JsonLayer : public QObject {
	Q_OBJECT

public:

	/*!
	 * @brief Holds information about an account.
	 */
	struct AccountInfo {
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
		QString _acntName;
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
		QString id;
		QString name;
		QString address;
		int type;
		bool effectiveOVM;
	};

	class MsgEnvelope {
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

	JsonLayer(QObject *parent = 0);
	~JsonLayer(void);

	void mojeIDtest(void);

	bool loginToWebDatovka(void);

	bool pingServer(QString &errStr);

	bool createAccount(const QString &name, QString &errStr);

	bool renameAccount(int accountID, const QString &newName,
	    QString &errStr);

	bool deleteAccount(int accountID, QString &errStr);

	bool getAccountList(QList<JsonLayer::AccountInfo> &accountList,
	    QString &errStr);

	bool getAccountInfo(int accountID,
	    JsonLayer::AccountInfo &accountInfo, QString &errStr);

	bool getUserInfo(int accountID,
	    JsonLayer::UserInfo &userInfo, QString &errStr);

	bool getMessageList(int accountID, int messageType, int limit,
	    int offset, QList<MsgEnvelope> &messageList, QString &errStr);

	bool syncAccount(int accountID, QString &errStr);

	QByteArray downloadMessage(int msgId, QString &errStr);

	QByteArray downloadFile(int fileId, QString &errStr);

	bool getTagList(QList<JsonLayer::Tag> &tagList, QString &errStr);

	int createTag(const QString &name, const QString &color,
	    QString &errStr);

	bool updateTag(int tagId, const QString &name,
	    const QString &color, QString &errStr);

	bool deleteTag(int tagId, QString &errStr);

	bool assignTag(int tagId, int msgId, QString &errStr);

	bool removeTag(int tagId, int msgId, QString &errStr);

	bool removeAllTags(int msgId, QString &errStr);

	bool searchRecipient(int accountID, const QString &word, int position,
	    QList<JsonLayer::Recipient> &resultList, bool &hasMore,
	    QString &errStr);

private:

	bool isLoggedToWebDatovka(void);

	bool parseAccountList(const QByteArray &content,
	    QList<JsonLayer::AccountInfo> &accountList, QString &errStr);

	bool parseMessageList(const QByteArray &content,
	    QList<MsgEnvelope> &messageList, QString &errStr);

	bool parseSyncAccount(const QByteArray &content, QString &errStr);

	bool parseAccountInfo(const QByteArray &content,
	    JsonLayer::AccountInfo &accountInfo, QString &errStr);

	bool parseUserInfo(const QByteArray &content,
	    JsonLayer::UserInfo &userInfo, QString &errStr);

	bool parseTagList(const QByteArray &content,
	    QList<JsonLayer::Tag> &tagList, QString &errStr);

	bool parseSearchRecipient(const QByteArray &content,
	    QList<JsonLayer::Recipient> &resultList, bool &hasMore,
	    QString &errStr);
};

extern JsonLayer jsonlayer;

#endif /* _JSON_H_ */
