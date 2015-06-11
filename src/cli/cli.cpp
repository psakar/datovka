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
const QString createErrorMsg(const QString &msg)
/* ========================================================================= */
{
	return QString(CLI_PREFIX) + QString(PARSER_PREFIX) + msg;
}


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
const QStringList parseAttachment(const QString &files)
/* ========================================================================= */
{
	if (files.isEmpty()) {
		return QStringList();
	}
	return files.split(";");
}


/* ========================================================================= */
int checkConnectMandatoryAttributes(const QMap <QString, QVariant> &map)
/* ========================================================================= */
{
	QString errmsg;
	int ret = CLI_RET_ERROR_CODE;

	qDebug() << CLI_PREFIX << "checking of mandatory connect parameters...";

	if (!map.contains("method") ||
	    map.value("method").toString().isEmpty()) {
		errmsg = createErrorMsg("method attribute missing or "
		    "contains empty string.");
		qDebug() << errmsg;
		return ret;
	}
	if (!map.contains("username") ||
	    map.value("username").toString().isEmpty() ||
	    map.value("username").toString().length() != 6) {
		errmsg = createErrorMsg("username attribute missing or "
		    "contains wrong value.");
		qDebug() << errmsg;
		return ret;
	}
	if (!map.contains("password") ||
	    map.value("password").toString().isEmpty()) {
		errmsg = createErrorMsg("password attribute missing or "
		    "contains empty string.");
		qDebug() << errmsg;
		return ret;
	}

	QString method = map.value("method").toString();

	if (method == L_HOTP || method == L_HOTP) {
		if (!map.contains("otpcode") ||
		    map.value("otpcode").toString().isEmpty()) {
			errmsg = createErrorMsg("otpcode attribute missing or "
			    "contains empty security code.");
			qDebug() << errmsg;
			return ret;
		}
	} else if (method == L_CERT) {
		if (!map.contains("certificate") ||
		    map.value("certificate").toString().isEmpty()) {
			errmsg = createErrorMsg("certificate attribute missing"
			    " or contains empty certificate path.");
			qDebug() << errmsg;
			return ret;
		}
	} else if (method == L_USER) {

	} else {
		return ret;
	}

	/* CALL LIBISDS LOGIN METHOD */
	ret = 0;

	return ret;

}


/* ========================================================================= */
int checkSendMsgMandatoryAttributes(const QMap <QString, QVariant> &map)
/* ========================================================================= */
{
	QString errmsg;
	int ret = CLI_RET_ERROR_CODE;

	qDebug() << CLI_PREFIX << "checking of mandatory send msg parameters...";

	if (!map.contains("username") ||
	    map.value("username").toString().isEmpty() ||
	    map.value("username").toString().length() != 6) {
		errmsg = createErrorMsg("username attribute missing or "
		    "contains wrong value.");
		qDebug() << errmsg;
		return ret;
	}
	if (!map.contains("dbIDRecipient") ||
	    map.value("dbIDRecipient").toString().isEmpty() ||
	    map.value("dbIDRecipient").toString().length() != 7) {
		errmsg = createErrorMsg("databox ID attribute of recipient "
		    "missing or contains wrong value.");
		qDebug() << errmsg;
		return ret;
	}
	if (!map.contains("dmAnnotation") ||
	    map.value("dmAnnotation").toString().isEmpty()) {
		errmsg = createErrorMsg("subject attribute missing or "
		    "contains empty string.");
		qDebug() << errmsg;
		return ret;
	}
	if (!map.contains("dmAttachment") ||
	    map.value("dmAttachment").toStringList().isEmpty()) {
		errmsg = createErrorMsg("attachment attribute missing or "
		    "contains empty file path list.");
		qDebug() << errmsg;
		return ret;
	}

	/* CALL LIBISDS LOGIN METHOD */
	ret = 0;

	return ret;

}


/* ========================================================================= */
int runService(const QString &service, const QString &paramString)
/* ========================================================================= */
{
	qDebug() <<  CLI_PREFIX << "input:" << service << ":" << paramString;

	QMap <QString, QVariant> map;

	QString attribute = "";
	QString value = "";
	QString errmsg;
	bool newAttribute = true;
	bool newValue = false;
	bool special = false;
	int attrPosition = 0;
	int ret = CLI_RET_ERROR_CODE;

	qDebug() << CLI_PREFIX << "parsing input string with parameters...";

	for (int i = 0; i < paramString.length(); ++i) {
		if (paramString.at(i) == ',') {
			if (newValue) {
				value = value + paramString.at(i);
			} else {
				attrPosition++;
				//qDebug() << attribute << value;
				if (attribute.isEmpty()) {
					errmsg = createErrorMsg(
					    QString("empty attribute "
					    "name on position '%1'").
					    arg(attrPosition));
					qDebug() << errmsg;
					return ret;
				}
				if (value.isEmpty()) {
					errmsg = createErrorMsg(
					    QString("empty attribute "
					    "value on position '%1'").
					    arg(attrPosition));
					qDebug() << errmsg;
					return ret;
				}

				if (checkAttributeIfExists(service,attribute)) {
					if (attribute == ATTACH_LABEL) {
						map[attribute] =
						    parseAttachment(value);
					} else {
						map[attribute] = value;
					}
				} else {
					errmsg = createErrorMsg(
					    QString("unknown attribute "
					    "name '%1'").arg(attribute));
					qDebug() << errmsg;
					return ret;
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

	// parse last token
	attrPosition++;
	if (attribute.isEmpty()) {
		errmsg = createErrorMsg(QString("empty attribute "
		    "name on position '%1'").arg(attrPosition));
		qDebug() << errmsg;
		return ret;
	}
	if (value.isEmpty()) {
		errmsg = createErrorMsg(QString("empty attribute "
		    "value on position '%1'").arg(attrPosition));
		qDebug() << errmsg;
		return ret;
	}
	if (checkAttributeIfExists(service, attribute)) {
		if (attribute == ATTACH_LABEL) {
			map[attribute] = parseAttachment(value);
		} else {
			map[attribute] = value;
		}
	} else {
		errmsg = createErrorMsg(QString("unknown attribute "
		    "name '%1'").arg(attribute));
		qDebug() << errmsg;
		return ret;
	}

	// add service name to map
	map[SERVICE_LABEL] = service;

	if (service == SER_CONNECT) {
		ret = checkConnectMandatoryAttributes(map);
	} else if (service == SER_GET_MSG_LIST) {
		ret = checkConnectMandatoryAttributes(map);
	} else if (service == SER_SEND_MSG) {
		ret = checkSendMsgMandatoryAttributes(map);
	} else if (service == SER_DWNLD_MSG) {
		ret = checkConnectMandatoryAttributes(map);
	} else if (service == SER_DWNLD_DEL_INFO) {
		ret = checkConnectMandatoryAttributes(map);
	} else if (service == SER_GET_USER_INFO) {
		ret = checkConnectMandatoryAttributes(map);
	} else if (service == SER_GET_OWNER_INFO) {
		ret = checkConnectMandatoryAttributes(map);
	} else {
		return ret;
	}

	return ret;
}
