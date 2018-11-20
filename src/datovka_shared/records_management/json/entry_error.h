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

#pragma once

#include <QCoreApplication> /* Q_DECLARE_TR_FUNCTIONS() */
#include <QString>

class QJsonValue; /* Forward declaration. */

namespace RecMgmt {

	/*!
	 * @brief Encapsulates any error entry.
	 */
	class ErrorEntry {
		Q_DECLARE_TR_FUNCTIONS(ErrorEntry)

	public:
		/*!
		 * @brief Error codes.
		 */
		enum Code {
			ERR_NO_ERROR, /*!< Just for convenience. */
			ERR_MALFORMED_REQUEST, /*!< JSON request was corrupt. */
			ERR_MISSING_IDENTIFIER, /*!< JSON request provided no identifier. */
			ERR_WRONG_IDENTIFIER, /*!< Wrong identifier provided in JSON request. */
			ERR_UNSUPPORTED_FILE_FORMAT, /*!< Uploaded file format not supported. */
			ERR_ALREADY_PRESENT, /*!< File is already present. */
			ERR_LIMIT_EXCEEDED, /*!< Too many identifiers in a single request. */
			ERR_UNSPECIFIED /*!< Unspecified error. */
		};

		/*!
		 * @brief Constructor. Constructs a no-error entry.
		 */
		ErrorEntry(void);

		/*!
		 * @brief Constructor.
		 *
		 * @param[in] code Error code.
		 * @param[in] description Additional error description.
		 */
		ErrorEntry(enum Code code, const QString &description);

		/*!
		 * @brief Copy constructor.
		 *
		 * @param[in] other Error entry.
		 */
		ErrorEntry(const ErrorEntry &other);

		/*!
		 * @brief Returns error code.
		 *
		 * @return Error code.
		 */
		enum Code code(void) const;

		/*!
		 * @brief Returns error description.
		 *
		 * @return Error description.
		 */
		const QString &description(void) const;

		/*!
		 * @brief Set content according to JSON value.
		 *
		 * @note Method does not modify value of error entry if false returned.
		 *     JSON null values are converted into no-error entries.
		 *
		 * @param[in] jsonVal Value to read from.
		 * @return True on success, false else.
		 */
		bool fromJsonVal(const QJsonValue *jsonVal);

		/*!
		 * @brief Set content of supplied JSON value.
		 *
		 * @note No-error entries are converted into JSON null values.
		 *
		 * @param[out] jsonVal Value to store to.
		 * @return True on success, false else.
		 */
		bool toJsonVal(QJsonValue *jsonVal) const;

		/*!
		 * @brief Returns translated error description.
		 *
		 * @return String containing localised description.
		 */
		QString trVerbose(void) const;

	private:
		/*!
		 * @brief Converts error code into string as used in JSON.
		 *
		 * @param[in] code Code value to be converted into a string.
		 * @return JSON string code representation.
		 */
		static
		const QString &codeToString(enum Code code);

		/*!
		 * @brief Converts error string as used in JSON into error code.
		 *
		 * @param[in]  str JSON string code representation.
		 * @param[out] ok Set to true if conversion was ok.
		 * @return Code value.
		 */
		static
		enum Code stringToCode(const QString &str, bool *ok = Q_NULLPTR);

		enum Code m_code; /*!< Error code. */
		QString m_description; /*!< Error description as obtained from JSON. */
	};

}
