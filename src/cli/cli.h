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


#ifndef CLI_H
#define CLI_H

#include <QApplication>
#include <QDebug>

#define SER_CONNECT "connect"
#define SER_GET_MSG_LIST "get-msg-list"
#define SER_SEND_MSG "send-msg"


struct sendMsgStruct {
	QString username;	//Username. Length is 6 characters. [Mandatory]
	QString dbIDSender;	//Box ID of sender. Length is 7 characters. [Mandatory]
	QString dbIDRecipient;	//Box ID of recipient. Length is 7 characters. [Mandatory]
	QString dmAnnotation;	//Subject (title) of the message. Maximal length is 255 characters. [Mandatory]
	QString dmToHands;	//Person in recipient organisation. Maximal length is 50 characters. [Optional]
	QString dmRecipientRefNumber; //Czech: číslo jednací příjemce. Maximal length is 50 characters. [Optional]
	QString dmSenderRefNumber; //Czech: číslo jednací odesílatele. Maximal length is 50 characters. [Optional]
	QString dmRecipientIdent; //Czech: spisová značka příjemce. Maximal length is 50 characters. [Optional]
	QString dmSenderIdent; //Czech: spisová značka odesílatele. Maximal length is 50 characters. [Optional]
	QString dmLegalTitleLaw; //Number of act mandating authority. Maximal length is 4 characters. [Optional]
	QString dmLegalTitleYear; //Year of act issue mandating authority. Maximal length is 4 characters. [Optional]
	QString dmLegalTitleSect; //Section of act mandating authority (paragraf). Maximal length is 10 characters. [Optional]
	QString dmLegalTitlePar; //Paragraph of act mandating authority (odstavec). Maximal length is 10 characters. [Optional]
	QString dmLegalTitlePoint; //Point of act mandating authority (písmeno). Maximal length is 1 characters. [Optional]
	bool dmPersonalDelivery;//If true, only person with higher privileges can read this message. Values: {0,1}, default is 1.
	bool dmAllowSubstDelivery; //Allow delivery through fiction. Values: {0,1}, default is 1.
	QChar dmType; //Message type (commercial subtypes or government message). Maximal length is 1 character. Values: {I,K,O,V} [Optional]
	bool dmOVM; //OVM sending mode. Values: {0,1}, default is 1.
	bool dmPublishOwnID; //Allow sender to express his name shall be available to recipient. Values: {0,1}, default is 0.
	QStringList dmAttachment;
};


int parseService2(const QString &service, const QString &paramString);

int checkAndSetValues(const QString &service, const QString &label,
    const QString &value);

int parseService(const QString &service, const QString &paramString);

#endif // CLI_H
