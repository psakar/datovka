/*
 * Copyright (C) 2014-2017 CZ.NIC
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

#include <QJsonObject>
#include <QJsonValue>

#include "src/document_service/json/entry_error.h"
#include "src/document_service/json/helper.h"

static
const QString keyCode("code");
static
const QString keyDescription("description");

static const QString strNoError("NO_ERROR"); /* This one should not be used. */
static const QString strMalformedRequest("MALFORMED_REQUEST");
static const QString strMissingIdentifier("MISSING_IDENTIFIER");
static const QString strWrongIdentifier("WRONG_IDENTIFIER");
static const QString strUnsupportedFileFormat("UNSUPPORTED_FILE_FORMAT");
static const QString strAlreadyPresent("ALREADY_PRESENT");
static const QString strLimitExceeded("LIMIT_EXCEEDED");
static const QString strUnspecified("UNSPECIFIED");

ErrorEntry::ErrorEntry(void)
    : m_code(ERR_NO_ERROR),
    m_description()
{
}

ErrorEntry::ErrorEntry(enum Code code, const QString &description)
    : m_code(code),
    m_description(description)
{
}

ErrorEntry::ErrorEntry(const ErrorEntry &ee)
    : m_code(ee.m_code),
    m_description(ee.m_description)
{
}

enum ErrorEntry::Code ErrorEntry::code(void) const
{
	return m_code;
}

const QString &ErrorEntry::description(void) const
{
	return m_description;
}

bool ErrorEntry::fromJsonVal(const QJsonValue *jsonVal)
{
	if (jsonVal == Q_NULLPTR) {
		Q_ASSERT(0);
		return false;
	}

	if (jsonVal->isNull()) {
		m_code = ERR_NO_ERROR;
		m_description.clear();
		return true;
	}

	if (!jsonVal->isObject()) {
		return false;
	}

	QJsonObject jsonObj(jsonVal->toObject());

	QString codeStr, descrStr;
	if (!JsonHelper::readString(jsonObj, keyCode, codeStr, false)) {
		return false;
	}
	if (!JsonHelper::readString(jsonObj, keyDescription, descrStr, false)) {
		return false;
	}

	bool ok = false;
	enum Code code = stringToCode(codeStr, &ok);
	if (!ok) {
		return false;
	}

	m_code = code;
	m_description = descrStr;
	return true;
}

bool ErrorEntry::toJsonVal(QJsonValue *jsonVal) const
{
	if (jsonVal == Q_NULLPTR) {
		Q_ASSERT(0);
		return false;
	}

	if (m_code == ERR_NO_ERROR) {
		*jsonVal = QJsonValue();
		return true;
	}

	QJsonObject jsonObj;
	jsonObj.insert(keyCode, codeToString(m_code));
	jsonObj.insert(keyDescription, m_description);
	*jsonVal = jsonObj;
	return true;
}

QString ErrorEntry::trVerbose(void) const
{
	QString retStr(codeToString(m_code) + QLatin1String(" ("));
	QString explanation;

	switch (m_code) {
	case ERR_NO_ERROR:
		explanation = tr("No error occurred");
		break;
	case ERR_MALFORMED_REQUEST:
		explanation = tr("Request was malformed");
		break;
	case ERR_MISSING_IDENTIFIER:
		explanation = tr("Identifier is missing");
		break;
	case ERR_WRONG_IDENTIFIER:
		explanation = tr("Supplied identifier is wrong");
		break;
	case ERR_UNSUPPORTED_FILE_FORMAT:
		explanation = tr("File format is not supported");
		break;
	case ERR_ALREADY_PRESENT:
		explanation = tr("Data are already present");
		break;
	case ERR_LIMIT_EXCEEDED:
		explanation = tr("Service limit was exceeded");
		break;
	case ERR_UNSPECIFIED:
		explanation = tr("Unspecified error");
		break;
	default:
		Q_ASSERT(0);
		explanation = tr("Unknown error");
		break;
	}

	retStr += QLatin1String(") ") + explanation;
	return retStr;
}

const QString &ErrorEntry::codeToString(enum Code code)
{
	switch (code) {
	case ERR_NO_ERROR:
		Q_ASSERT(0); /* This one should never occur. */
		return strNoError;
		break;
	case ERR_MALFORMED_REQUEST:
		return strMalformedRequest;
		break;
	case ERR_MISSING_IDENTIFIER:
		return strMissingIdentifier;
		break;
	case ERR_WRONG_IDENTIFIER:
		return strWrongIdentifier;
		break;
	case ERR_UNSUPPORTED_FILE_FORMAT:
		return strUnsupportedFileFormat;
		break;
	case ERR_ALREADY_PRESENT:
		return strAlreadyPresent;
		break;
	case ERR_LIMIT_EXCEEDED:
		return strLimitExceeded;
		break;
	case ERR_UNSPECIFIED:
		return strUnspecified;
		break;
	default:
		Q_ASSERT(0);
		return strUnspecified;
		break;
	}
}

enum ErrorEntry::Code ErrorEntry::stringToCode(const QString &str, bool *ok)
{
	if (str == strNoError) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return ErrorEntry::ERR_NO_ERROR;
	} else if (str == strMalformedRequest) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return ErrorEntry::ERR_MALFORMED_REQUEST;
	} else if (str == strMissingIdentifier) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return ERR_MISSING_IDENTIFIER;
	} else if (str == strWrongIdentifier) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return ERR_WRONG_IDENTIFIER;
	} else if (str == strUnsupportedFileFormat) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return ErrorEntry::ERR_UNSUPPORTED_FILE_FORMAT;
	} else if (str == strAlreadyPresent) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return ErrorEntry::ERR_ALREADY_PRESENT;
	} else if (str == strLimitExceeded) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return ErrorEntry::ERR_LIMIT_EXCEEDED;
	} else if (str == strUnspecified) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return ErrorEntry::ERR_UNSPECIFIED;
	} else {
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return ErrorEntry::ERR_UNSPECIFIED;
	}
}
