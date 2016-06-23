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
#include <QUrl>

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

	/*!
	 * @brief Holds message envelope data from webdatovka.
	 */
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
		bool dmPublishOwnID;
		bool dmOVM;
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

	/*!
	 * @brief Fake login to Webdatovka - will be removed.
	 *
	 * @return sessionid from webdatovka.
	 */
	QNetworkCookie fakeLoginWebDatovka(void);

	/*!
	 * @brief First part of login to Webdatovka.
	 *        Call webdatovka to obtain openconnect data.
	 *
	 * @param[out] lastUrl - last url of login sequence.
	 * @return true if first part was successed.
	 */
	bool startLoginToWebDatovka(QUrl &lastUrl);

	/*!
	 * @brief Middle part of login to Webdatovka.
	 *
	 * @param[in] method      - selected login method.
	 * @param[in/out] lastUrl - last url of login sequence.
	 * @param[out] token      - html security token.
	 * @return true if middle part was successed.
	 */
	bool loginMethodChanged(int method, QString &lastUrl, QString &token);

	/*!
	 * @brief Last part of login to Webdatovka.
	 *
	 * @param[in] lastUrl    - last url of login sequence.
	 * @param[in] token      - html security token.
	 * @param[in] username   - mojeID username.
	 * @param[in] pwd        - mojeID password.
	 * @param[in] otp        - OTP password (optional).
	 * @param[in] certPath   - path to certificate file.
	 * @param[out] errStr    - contains an error string if unssucces.
	 * @param[out] sessionid - contains webdatovka sessionid.
	 * @return true if login success.
	 */
	bool loginToMojeID(const QString &lastUrl,
	    const QString &token, const QString &username,
	    const QString &pwd, const QString &otp, const QString &certPath,
	    QString &errStr, QNetworkCookie &sessionid);

	/*!
	 * @brief Ping to Webdatovka.
	 *
	 * @param[in] userName - account username.
	 * @param[out] errStr  - contains an error string if unssucces.
	 * @return true if success.
	 */
	bool pingServer(const QString &userName, QString &errStr);

	/*!
	 * @brief Add account into Webdatovka.
	 *
	 * @param[in] userName - account username.
	 * @param[in] name     - name of account.
	 * @param[out] errStr  - contains an error string if unssucces.
	 * @return true if success.
	 */
	bool createAccount(const QString &userName,
	    const QString &name, QString &errStr);

	/*!
	 * @brief Update account name in Webdatovka.
	 *
	 * @param[in] userName  - account username.
	 * @param[in] accountID - webdatovka id of account.
	 * @param[in] newName   - new name of account.
	 * @param[out] errStr   - contains an error string if unssucces.
	 * @return true if success.
	 */
	bool renameAccount(const QString &userName,
	    int accountID, const QString &newName, QString &errStr);

	/*!
	 * @brief Delete account from Webdatovka.
	 *
	 * @param[in] userName  - account username.
	 * @param[in] accountID - webdatovka id of account.
	 * @param[out] errStr   - contains an error string if unssucces.
	 * @return true if success.
	 */
	bool deleteAccount(const QString &userName,
	    int accountID, QString &errStr);

	/*!
	 * @brief Download all accounts from Webdatovka for one mojeID identity.
	 *
	 * @param[in] sessionid    - sessionid cookie of Webdatovka.
	 * @param[out] userId      - mojeID user id.
	 * @param[out] accountList - list of accounts.
	 * @param[out] errStr      - contains an error string if unssucces.
	 * @return true if success.
	 */
	bool getAccountList(const QNetworkCookie &sessionid, int &userId,
	    QList<JsonLayer::AccountData> &accountList,
	    QString &errStr);

	/*!
	 * @brief Download current message list (envelopes) from Webdatovka.
	 *
	 * @param[in] userName     - account username.
	 * @param[in] accountID    - webdatovka id of account.
	 * @param[in] messageType  - webadatovka message type (sent = -1, received = 1);
	 * @param[in] limit        - how many records mat be returned.
	 * @param[in] offset       - default is 0.
	 * @param[out] messageList - list of message envelopes.
	 * @param[out] errStr      - contains an error string if unssucces.
	 * @return true if success.
	 */
	bool getMessageList(const QString &userName,
	    int accountID, int messageType, int limit,
	    int offset, QList<JsonLayer::Envelope> &messageList,
	    QString &errStr);

	/*!
	 * @brief Synchronize one account with isds via Webdatovka.
	 *
	 * @param[in] userName  - account username.
	 * @param[in] accountID - webdatovka id of account.
	 * @param[out] errStr   - contains an error string if unssucces.
	 * @return true if success.
	 */
	bool syncAccount(const QString &userName,
	    int accountID, QString &errStr);

	/*!
	 * @brief Download complete message from Webdatovka.
	 *
	 * @param[in] userName - account username.
	 * @param[in] msgId    - webdatovka message id.
	 * @param[out] errStr  - contains an error string if unssucces.
	 * @return message zfo binary data or NULL.
	 */
	QByteArray downloadMessage(const QString &userName,
	    int msgId, QString &errStr);

	/*!
	 * @brief Download one file of message from Webdatovka.
	 *
	 * @param[in] userName - account username.
	 * @param[in] fileId   - webdatovka file id.
	 * @param[out] errStr  - contains an error string if unssucces.
	 * @return file binary data or NULL.
	 */
	QByteArray downloadFile(const QString &userName,
	    int fileId, QString &errStr);

	/*!
	 * @brief Download tag list from Webdatovka for username.
	 *
	 * @param[in] userName - account username.
	 * @param[out] tagList - list of tag attributes.
	 * @param[out] errStr  - contains an error string if unssucces.
	 * @return true if success.
	 */
	bool getTagList(const QString &userName,
	    QList<JsonLayer::Tag> &tagList, QString &errStr);

	/*!
	 * @brief Create tag in Webdatovka for username.
	 *
	 * @param[in] userName - account username.
	 * @param[in] name     - tag name.
	 * @param[in] color    - tag color.
	 * @param[out] errStr  - contains an error string if unssucces.
	 * @return true webdatovka id of new tag if success else -1.
	 */
	int createTag(const QString &userName,
	    const QString &name, const QString &color, QString &errStr);

	/*!
	 * @brief Update tag data into Webdatovka for username.
	 *
	 * @param[in] userName - account username.
	 * @param[in] tagId    - webdatovka tag id.
	 * @param[in] name     - tag name.
	 * @param[in] color    - tag color.
	 * @param[out] errStr  - contains an error string if unssucces.
	 * @return true if success.
	 */
	bool updateTag(const QString &userName, int tagId,
	    const QString &name, const QString &color, QString &errStr);

	/*!
	 * @brief Delete tag from Webdatovka for username.
	 *
	 * @param[in] userName - account username.
	 * @param[in] tagId    - webdatovka tag id.
	 * @param[out] errStr  - contains an error string if unssucces.
	 * @return true if success.
	 */
	bool deleteTag(const QString &userName,
	    int tagId, QString &errStr);

	/*!
	 * @brief Assign tag to message in Webdatovka for username.
	 *
	 * @param[in] userName - account username.
	 * @param[in] tagId    - webdatovka tag id.
	 * @param[in] msgId    - webdatovka message id.
	 * @param[out] errStr  - contains an error string if unssucces.
	 * @return true if success.
	 */
	bool assignTag(const QString &userName,
	    int tagId, int msgId, QString &errStr);

	/*!
	 * @brief Remove tag assigment from message in Webdatovka for username.
	 *
	 * @param[in] userName - account username.
	 * @param[in] tagId    - webdatovka tag id.
	 * @param[in] msgId    - webdatovka message id.
	 * @param[out] errStr  - contains an error string if unssucces.
	 * @return true if success.
	 */
	bool removeTag(const QString &userName,
	    int tagId, int msgId, QString &errStr);

	/*!
	 * @brief Remove all tags from message in Webdatovka for username.
	 *
	 * @param[in] userName - account username.
	 * @param[in] msgId    - webdatovka message id.
	 * @param[out] errStr  - contains an error string if unssucces.
	 * @return true if success.
	 */
	bool removeAllTags(const QString &userName,
	    int msgId, QString &errStr);

	/*!
	 * @brief Search recipients via Webdatovka.
	 *
	 * @param[in] userName      - account username.
	 * @param[in] accountID     - webdatovka id of account.
	 * @param[in] word          - word for search.
	 * @param[in] position      - result list part, firstly is 0.
	 * @param[out] resultList   - list of recipients, where message was sent.
	 * @param[out] hasMore      - another list part of recipents is available.
	 * @param[out] errStr       - contains an error string if unssucces.
	 * @return true if success.
	 */
	bool searchRecipient(const QString &userName,
	    int accountID, const QString &word, int position,
	    QList<JsonLayer::Recipient> &resultList, bool &hasMore,
	    QString &errStr);

	/*!
	 * @brief Send message to recipients via Webdatovka.
	 *
	 * @param[in] userName      - account username.
	 * @param[in] accountID     - webdatovka id of account.
	 * @param[in] recipientList - another recipents are available.
	 * @param[in] envelope      - message envelope data.
	 * @param[in] fileList      - list of files (attachments).
	 * @param[out] resultList   - list of recipients, where message was sent.
	 * @param[out] errStr       - contains an error string if unssucces.
	 * @return true if success.
	 */
	bool sendMessage(const QString &userName, int accountID,
	    const QList<JsonLayer::Recipient> &recipientList,
	    const JsonLayer::Envelope &envelope,
	    const QList<JsonLayer::File> &fileList,
	    QStringList &resultList, QString &errStr);

	/*!
	 * @brief Delete message from Webdatovka.
	 *
	 * @param[in] userName - account username.
	 * @param[in] msgId    - webdatovka message id.
	 * @param[out] errStr  - contains an error string if unssucces.
	 * @return true if success.
	 */
	bool deleteMessage(const QString &userName, int msgI, QString &errStr);

	/*!
	 * @brief Mark messages as read in Webdatovka.
	 *
	 * @param[in] userName - account username.
	 * @param[in] msgId    - webdatovka message id.
	 * @param[out] errStr  - contains an error string if unssucces.
	 * @return true if success.
	 */
	bool markMessageAsRead(const QString &userName, int msgId,
	    QString &errStr);

private:

	/*!
	 * @brief Test if user is logged to Webdatovka.
	 *
	 * @param[in] userName   - account username.
	 * @param[out] sessionid - sessionid cookie for GET/POST reguests.
	 * @return true if user is logged.
	 */
	bool isLoggedToWebDatovka(const QString &userName,
	    QNetworkCookie &sessionid);

	/*!
	 * @brief Parse account list.
	 *
	 * @param[in] content      - reply content.
	 * @param[out] userId      - mojeID user id.
	 * @param[out] accountList - list of accounts.
	 * @param[out] errStr      - contains an error string if unssucces.
	 * @return true if success.
	 */
	bool parseAccountList(const QByteArray &content, int &userId,
	    QList<JsonLayer::AccountData> &accountList,
	    QString &errStr);

	/*!
	 * @brief Parse message list.
	 *
	 * @param[in] content      - reply content.
	 * @param[out] messageList - list of message envelopes.
	 * @param[out] errStr      - contains an error string if unssucces.
	 * @return true if success.
	 */
	bool parseMessageList(const QByteArray &content,
	    QList<JsonLayer::Envelope> &messageList, QString &errStr);

	/*!
	 * @brief Parse tag list.
	 *
	 * @param[in] content  - reply content.
	 * @param[out] tagList - list of tag attributes.
	 * @param[out] errStr  - contains an error string if unssucces.
	 * @return true if success.
	 */
	bool parseTagList(const QByteArray &content,
	    QList<JsonLayer::Tag> &tagList, QString &errStr);

	/*!
	 * @brief Parse recipient records.
	 *
	 * @param[in] content     - reply content.
	 * @param[out] resultList - list of recipients.
	 * @param[out] hasMore    - another recipents are available.
	 * @param[out] errStr     - contains an error string if unssucces.
	 * @return true if success.
	 */
	bool parseSearchRecipient(const QByteArray &content,
	    QList<JsonLayer::Recipient> &resultList, bool &hasMore,
	    QString &errStr);
};

/* global jsonlazyer object */
extern JsonLayer jsonlayer;

#endif /* _JSON_H_ */
