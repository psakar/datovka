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

#ifndef _TASK_SEND_MESSAGE_H_
#define _TASK_SEND_MESSAGE_H_

#include <QByteArray>
#include <QList>
#include <QString>

#include "src/io/message_db_set.h"
#include "src/worker/task.h"

/*!
 * @brief Ersatz structure for isds_envelope.
 */
class IsdsEnvelope {
public:
	/*!
	 * @brief Constructor.
	 */
	IsdsEnvelope(void)
	    : dmID(),
	    dbIDRecipient(),
	    dmToHands(),
	    dmAnnotation(),
	    dmRecipientRefNumber(),
	    dmSenderRefNumber(),
	    dmRecipientIdent(),
	    dmSenderIdent(),
	    _using_dmLegalTitleLaw(false),
	    dmLegalTitleLaw(0),
	    _using_dmLegalTitleYear(false),
	    dmLegalTitleYear(0),
	    dmLegalTitleSect(),
	    dmLegalTitlePar(),
	    dmLegalTitlePoint(),
	    dmPersonalDelivery(false),
	    dmAllowSubstDelivery(false),
	    dmType(),
	    dmOVM(false),
	    dmPublishOwnID(false)
	{
	}

	/* The commented members are not used when sending a message. */

	QString dmID;
//    char *dbIDSender;
//    char *dmSender;
//    char *dmSenderAddress;
//    long int *dmSenderType;
//    char *dmRecipient;
//    char *dmRecipientAddress;
//    _Bool *dmAmbiguousRecipient;
//    unsigned long int *dmOrdinal;
//    isds_message_status *dmMessageStatus;
//    long int *dmAttachmentSize;
//    struct timeval *dmDeliveryTime;
//    struct timeval *dmAcceptanceTime;
//    struct isds_hash *hash;
//    void *timestamp;
//    size_t timestamp_length;
//    struct isds_list *events;
//    char *dmSenderOrgUnit;
//    long int *dmSenderOrgUnitNum;
	QString dbIDRecipient;
//    char *dmRecipientOrgUnit;
//    long int *dmRecipientOrgUnitNum;
	QString dmToHands;
	QString dmAnnotation;
	QString dmRecipientRefNumber;
	QString dmSenderRefNumber;
	QString dmRecipientIdent;
	QString dmSenderIdent;

	bool _using_dmLegalTitleLaw;
	long int dmLegalTitleLaw;
	bool _using_dmLegalTitleYear;
	long int dmLegalTitleYear;
	QString dmLegalTitleSect;
	QString dmLegalTitlePar;
	QString dmLegalTitlePoint;
	bool dmPersonalDelivery;
	bool dmAllowSubstDelivery;
	QString dmType;
	bool dmOVM;
	bool dmPublishOwnID;
};

/*!
 * @brief Ersatz structure for isds_document.
 */
class IsdsDocument {
public:
	/*!
	 * @brief Constructor.
	 */
	IsdsDocument(void)
	    : isXml(false),
	    data(),
	    dmMimeType(),
	    dmFileDescr()
	{
	}

	bool isXml;
	QByteArray data;
	QString dmMimeType;
	QString dmFileDescr;
};

/*!
 * @brief Ersatz structure for isds_message.
 */
class IsdsMessage {
	/*
	 * Most of the content of the original structure is ignored as it
	 * is not needed when composing and sending a message.
	 */
public:
	IsdsEnvelope envelope; /*!< Message envelope. */
	QList<IsdsDocument> documents; /*!< List of documents. */
};

/*!
 * @brief Task describing sending message.
 */
class TaskSendMessage : public Task {
public:
	/*!
	 * @brief Gives more detailed information about sending outcome.
	 */
	class Result {
	public:
		/*!
		 * @brief Constructor.
		 */
		Result(void);

		int isdsRetError; /*!< Status as returned by libisds. */
		QString dbIDRecipient; /*!< Recipient identifier. */
		QString recipientName; /*!< Recipient name. */
		qint64 dmId; /*!< Sent message identifier. */
		bool isPDZ; /*!< True if message was sent as PDZ. */
		QString errInfo; /*!< Error description. */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in]     userName         Account identifier (user login name).
	 * @param[in,out] dbSet            Non-null pointer to database
	 *                                 container.
	 * @param[in]     message          Message to be sent.
	 * @param[in]     recipientName    Message recipient name.
	 * @param[in]     recipientAddress Message recipient address.
	 * @param[in]     isPDZ            True if message is a PDZ.
	 */
	explicit TaskSendMessage(const QString &userName,
	    MessageDbSet *dbSet, const IsdsMessage &message,
	    const QString &recipientName, const QString &recipientAddress,
	    bool isPDZ);

	/*!
	 * @brief Performs actual message sending.
	 */
	virtual
	void run(void);

	Result m_sendingResult; /*!< Sending outcome. */

private:
	/*!
	 * Disable copy and assignment.
	 */
	TaskSendMessage(const TaskSendMessage &);
	TaskSendMessage &operator=(const TaskSendMessage &);

	/*!
	 * @brief Sends a single message to ISDS fro given account.
	 *
	 * @param[in]     userName         Account identifier (user login name).
	 * @param[in,out] dbSet            Database container.
	 * @param[in,out] message          Message being sent.
	 * @param[in]     recipientName    Message recipient name.
	 * @param[in]     recipientAddress Message recipient address.
	 * @param[in]     isPDZ            True if message is a PDZ.
	 * @param[in]     progressLabel    Progress-bar label.
	 * @param[out]    result           Results, pass NULL if not desired.
	 * @return Error state.
	 */
	static
	qdatovka_error sendMessage(const QString &userName,
	    MessageDbSet &dbSet, struct isds_message *message,
	    const QString &recipientName, const QString &recipientAddress,
	    bool isPDZ, const QString &progressLabel, Result *result);

	const QString m_userName; /*!< Account identifier (user login name). */
	MessageDbSet *m_dbSet; /*!< Pointer to database container. */
	const IsdsMessage m_message; /*!< Message to be sent. */
	const QString m_recipientName; /*!< Message recipient name. */
	const QString m_recipientAddress; /*!< Message recipient address. */
	const bool m_isPDZ; /*!< True if message is a PDZ. */
};

#endif /* _TASK_SEND_MESSAGE_H_ */
