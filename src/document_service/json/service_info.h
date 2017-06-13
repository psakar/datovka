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

#ifndef _SERVICE_INFO_H_
#define _SERVICE_INFO_H_

#include <QByteArray>
#include <QString>

/*!
 * @brief Encapsulates the service_info response.
 */
class ServiceInfoResp {
private:
	/*!
	 * @brief Constructor. Creates an invalid structure.
	 */
	ServiceInfoResp(void);

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] logoSvg SVG logo stored as raw data.
	 * @param[in] name Service provider name.
	 * @param[in] tokenMame Security token name.
	 */
	ServiceInfoResp(const QByteArray &logoSvg, const QString &name,
	    const QString &tokenName);

public:
	/*!
	 * @brief Copy constructor.
	 *
	 * @param[in] sir Service info response.
	 */
	ServiceInfoResp(const ServiceInfoResp &sir);

	/*!
	 * @brief Return raw SVG data.
	 *
	 * @return Stored raw SVG data.
	 */
	const QByteArray &logoSvg(void) const;

	/*!
	 * @brief Return service provider name.
	 *
	 * @return Stored name.
	 */
	const QString &name(void) const;

	/*!
	 * @brief Return security token name.
	 *
	 * @return Stored token name.
	 */
	const QString &tokenName(void) const;

	/*!
	 * @brief Check whether content is valid.
	 *
	 * @return True if content is valid.
	 */
	bool isValid(void) const;

	/*!
	 * @brief Creates a service info structure from supplied JSON document.
	 *
	 * @param[in]  json JSON document.
	 * @param[out] ok Set to true on success.
	 * @return Invalid structure on error a valid structure else.
	 */
	static
	ServiceInfoResp fromJson(const QByteArray &json, bool *ok = Q_NULLPTR);

	/*!
	 * @brief Converts service info structure into a JSON document.
	 *
	 * @note Unspecified values are stores as null into the JSON document.
	 *
	 * @return JSON document containing stored data.
	 */
	QByteArray toJson(void) const;

private:
	QByteArray m_logoSvg; /*!< Raw SVG data. */
	QString m_name; /*!< Service provider name. */
	QString m_tokenName; /*!< Obtained token identifier. */
};

/*!
 * @brief Returns JSON data.
 */
QByteArray jsonServiceInfo(const QString &svgPath, const QString &name,
    const QString &tokenName);

#endif /* _SERVICE_INFO_H_ */
