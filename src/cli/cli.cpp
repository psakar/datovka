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


#include "src/cli/cli.h"

// Known attributes definition
const QStringList connectAttrs = QStringList() << "method" << "username"
    << "password" << "certificate" << "otpcode";
const QStringList getMsgListAttrs = QStringList() << "username" << "dmType"
   << "dmStatusFilter" << "dmOffset" << "dmLimit" << "dmFromTime" << "dmToTime";
const QStringList sendMsgAttrs = QStringList() << "username" << "dbIDSender"
    << "dbIDRecipient" << "dmAnnotation" << "dmToHands"
    << "dmRecipientRefNumber" << "dmSenderRefNumber" << "dmRecipientIdent"
    << "dmSenderIdent" << "dmLegalTitleLaw" << "dmLegalTitleYear"
    << "dmLegalTitleSect" << "dmLegalTitlePar" << "dmLegalTitlePoint"
    << "dmPersonalDelivery" << "dmAllowSubstDelivery" << "dmType" << "dmOVM"
    << "dmPublishOwnID" << ATTACH_LABEL;
const QStringList dwnldMsgAttrs = QStringList() << "username" << "dmID";
const QStringList dwnldDelInfoAttrs = QStringList() << "username" << "dmID";
const QString databoxInfoAttrs = "username";


/* ========================================================================= */
bool checkAttributeIfExists(const QString &service, const QString &attribute)
/* ========================================================================= */
{
	if (service == SER_CONNECT) {
		return connectAttrs.contains(attribute);
	} else if (service == SER_GET_MSG_LIST) {
		return getMsgListAttrs.contains(attribute);
	} else if (service == SER_SEND_MSG) {
		return sendMsgAttrs.contains(attribute);
	} else if (service == SER_DWNLD_MSG) {
		return dwnldMsgAttrs.contains(attribute);
	} else if (service == SER_DWNLD_DEL_INFO) {
		return dwnldDelInfoAttrs.contains(attribute);
	} else if (service == SER_GET_USER_INFO ||
	    service == SER_GET_OWNER_INFO) {
		return (attribute == databoxInfoAttrs);
	}
	return false;
}


/* ========================================================================= */
int runServiceTest(const QString &service, const QString &paramString)
/* ========================================================================= */
{
	qDebug() << service << ":" << paramString;

	struct sendMsgStruct sendMsg;
	QString errmsg;
	QStringList itemList;
	QString attribute;
	QString value;

	QStringList paramList = paramString.split(",");
	if (paramList.isEmpty()) {
		errmsg = "Parameter list missing or its format is wrong.";
		return -1;
	}

	for (int i = 0; i < paramList.count(); ++i) {

		itemList = paramList.at(i).split("=");

		if (itemList.count() != 2) {
			errmsg = QString("Couple [%1] of parameter string has "
			    "wrong format or more tokens.").arg(i);
			qDebug() << errmsg;
			return -1;
		}

		attribute = itemList.at(0);
		if (attribute.isEmpty()) {
			errmsg = QString("Parameter name missing in "
			    "the couple [%1].").arg(i);
			qDebug() << errmsg;
			return -1;
		}

		value = itemList.at(1);
		if (value.isEmpty()) {
			errmsg = QString("Value string missing in "
			    "the couple [%1].").arg(i);
			qDebug() << errmsg;
			return -1;
		}

		itemList.clear();
		itemList = value.split("'");

		int cnt = itemList.count();

		if (cnt != 3) {
			errmsg = QString("Value string has a wrong format in "
			    "the couple [%1].").arg(i);
			qDebug() << errmsg;
			return -1;
		}

		if (!itemList.at(0).isEmpty()) {
			errmsg = QString("Wrong symbol(s) before value in the "
			    "couple [%1].").arg(i);
			qDebug() << errmsg;
			return -1;
		}

		if (itemList.at(1).isEmpty()) {
			errmsg = QString("Value of parameter '%1' missing in the "
			    "couple [%2].").arg(attribute).arg(i);
			qDebug() << errmsg;
			return -1;
		}

		if (!itemList.at(2).isEmpty()) {
			errmsg = QString("Wrong symbol(s) after value in the "
			    "couple [%1].").arg(i);
			qDebug() << errmsg;
			return -1;
		}

		value = itemList.at(1);

		qDebug() << "attribute: " << attribute;
		qDebug() << "Value: " << value;


		if (itemList.isEmpty()) {
			errmsg = QString("Value string missing in the "
			    "couple [%1].").arg(i);
			qDebug() << errmsg;
			return -1;
		}

		if ("username" == attribute) {
			sendMsg.username = value;
		} else if ("dbIDSender" == attribute) {
			sendMsg.dbIDSender = value;
		}
	}

	return 0;
}

/* ========================================================================= */
QStringList parseAttachment(const QString &files)
/* ========================================================================= */
{
	if (files.isEmpty()) {
		return QStringList();
	}

	return files.split(";");
}


/* ========================================================================= */
int runService(const QString &service, const QString &paramString)
/* ========================================================================= */
{
	qDebug() << service << ":" << paramString;

	QMap <QString, QVariant> map;

	QString attribute = "";
	QString value = "";
	QString errmsg;
	bool newAttribute = true;
	bool newValue = false;
	bool special = false;
	int attrPosition = 0;

	for (int i = 0; i < paramString.length(); ++i) {
		if (paramString.at(i) == ',') {
			if (newValue) {
				value = value + paramString.at(i);
			} else {
				attrPosition++;
				//qDebug() << attribute << value;
				if (attribute.isEmpty()) {
					errmsg = PARSER_PREFIX +
					    QString("empty attribute "
					    "name on position '%1'").
					    arg(attrPosition);
					qDebug() << errmsg;
					return -1;
				}
				if (value.isEmpty()) {
					errmsg = PARSER_PREFIX +
					    QString("empty attribute "
					    "value on position '%1'").
					    arg(attrPosition);
					qDebug() << errmsg;
					return -1;
				}

				if (checkAttributeIfExists(service,attribute)) {
					if (attribute == ATTACH_LABEL) {
						map[attribute] =
						    parseAttachment(value);
					} else {
						map[attribute] = value;
					}
				} else {
					errmsg = PARSER_PREFIX +
					    QString("unknown attribute "
					    "name '%1'").arg(attribute);
					qDebug() << errmsg;
					return -1;
				}
				attribute.clear();
				value.clear();
				newAttribute = true;
				newValue = false;
			}
		} else if (paramString.at(i) == '=') {
			if (newValue) {
				value = value + paramString.at(i);
			} else {
				newAttribute = false;
			}
		} else if (paramString.at(i) == '\'') {
			if (special) {
				value = value + paramString.at(i);
				special = false;
			} else {
				newValue = !newValue;
			}
		} else if (paramString.at(i) == '\\') {
			if (special) {
				value = value + paramString.at(i);
				special = false;
			} else {
				special = true;
			}
		} else {
			if (newAttribute) {
				attribute = attribute + paramString.at(i);
			}
			if (newValue) {
				value = value + paramString.at(i);
			}
		}
	}

	attrPosition++;
	if (attribute.isEmpty()) {
		errmsg = PARSER_PREFIX +
		    QString("empty attribute "
		    "name on position '%1'").arg(attrPosition);
		qDebug() << errmsg;
		return -1;
	}
	if (value.isEmpty()) {
		errmsg = PARSER_PREFIX +
		    QString("empty attribute "
		    "value on position '%1'").arg(attrPosition);
		qDebug() << errmsg;
		return -1;
	}
	if (checkAttributeIfExists(service,attribute)) {
		if (attribute == ATTACH_LABEL) {
			map[attribute] = parseAttachment(value);
		} else {
			map[attribute] = value;
		}
	} else {
		errmsg = PARSER_PREFIX +
		    QString("unknown attribute "
		    "name '%1'").arg(attribute);
		qDebug() << errmsg;
		return -1;
	}

	map[SERVICE_LABEL] = service;

	qDebug() << map;

	/* TODO call libisds and delivery map */

	return 0;
}
