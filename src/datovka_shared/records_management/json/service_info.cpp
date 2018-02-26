/*
 * Copyright (C) 2014-2018 CZ.NIC
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

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include "src/datovka_shared/records_management/json/helper.h"
#include "src/datovka_shared/records_management/json/service_info.h"

static
const QString keyLogoSvg("logo_svg");
static
const QString keyName("name");
static
const QString keyTokenName("token_name");

ServiceInfoResp::ServiceInfoResp(void)
    : m_logoSvg(),
    m_name(),
    m_tokenName()
{
}

ServiceInfoResp::ServiceInfoResp(const QByteArray &logoSvg, const QString &name,
    const QString &tokenName)
    : m_logoSvg(logoSvg),
    m_name(name),
    m_tokenName(tokenName)
{
}

ServiceInfoResp::ServiceInfoResp(const ServiceInfoResp &sir)
    : m_logoSvg(sir.m_logoSvg),
    m_name(sir.m_name),
    m_tokenName(sir.m_tokenName)
{
}

const QByteArray &ServiceInfoResp::logoSvg(void) const
{
	return m_logoSvg;
}

const QString &ServiceInfoResp::name(void) const
{
	return m_name;
}

const QString &ServiceInfoResp::tokenName(void) const
{
	return m_tokenName;
}

bool ServiceInfoResp::isValid(void) const
{
	return !m_logoSvg.isNull() && !m_name.isNull() && !m_tokenName.isNull();
}

ServiceInfoResp ServiceInfoResp::fromJson(const QByteArray &json, bool *ok)
{
	QJsonObject jsonObj;
	if (!JsonHelper::readRootObject(json, jsonObj)) {
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return ServiceInfoResp();
	}

	ServiceInfoResp sir;

	{
		QString valStr;
		if (!JsonHelper::readString(jsonObj, keyLogoSvg, valStr,
		        false)) {
			if (ok != Q_NULLPTR) {
				*ok = false;
			}
			return ServiceInfoResp();
		}

		sir.m_logoSvg = QByteArray::fromBase64(valStr.toUtf8());
	}

	if (!JsonHelper::readString(jsonObj, keyName, sir.m_name, false)) {
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return ServiceInfoResp();
	}
	if (!JsonHelper::readString(jsonObj, keyTokenName, sir.m_tokenName,
	        false)) {
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return ServiceInfoResp();
	}

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return sir;
}

QByteArray ServiceInfoResp::toJson(void) const
{
	QJsonObject jsonObj;
	jsonObj.insert(keyLogoSvg, !m_logoSvg.isNull() ?
	    QString::fromUtf8(m_logoSvg.toBase64()) : QJsonValue());
	jsonObj.insert(keyName, !m_name.isNull() ? m_name : QJsonValue());
	jsonObj.insert(keyTokenName, !m_tokenName.isNull() ?
	    m_tokenName : QJsonValue());

	return QJsonDocument(jsonObj).toJson(QJsonDocument::Indented);
}
