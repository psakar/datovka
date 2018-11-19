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

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

#include "src/datovka_shared/records_management/json/entry_error.h"

namespace RecMgmt {

	/*!
	 * @brief Encapsulates the upload_file request.
	 */
	class UploadFileReq {
	private:
		/*!
		 * @brief Constructor. Creates an invalid structure.
		 */
		UploadFileReq(void);

	public:
		/*!
		 * @brief Constructor.
		 *
		 * @param[in] ids Location identifiers as obtained from upload_hierarchy.
		 * @param[in] fileName File name.
		 * @param[in] fileContent Raw file content.
		 */
		UploadFileReq(const QStringList &ids, const QString &fileName,
		    const QByteArray &fileContent);

		/*!
		 * @brief Copy constructor.
		 *
		 * @param[in] other Upload file request.
		 */
		UploadFileReq(const UploadFileReq &other);

		/*!
		 * @brief Return location identifier.
		 *
		 * @return Location identifier.
		 */
		const QStringList &ids(void) const;

		/*!
		 * @brief Return file name.
		 *
		 * @return File name.
		 */
		const QString &fileName(void) const;

		/*!
		 * @brief Return file content.
		 *
		 * @return Raw file content.
		 */
		const QByteArray &fileContent(void) const;

		/*!
		 * @brief Check whether content is valid.
		 *
		 * @return True if content is valid.
		 */
		bool isValid(void) const;

		/*!
		 * @brief Creates a upload file request structure from supplied JSON
		 *     document.
		 *
		 * @param[in]  json JSON document.
		 * @param[out] ok Set to true on success.
		 * @return Invalid structure on error a valid structure else.
		 */
		static
		UploadFileReq fromJson(const QByteArray &json, bool *ok = Q_NULLPTR);

		/*!
		 * @brief Converts upload file structure into a JSON document.
		 *
		 * @note Unspecified values are stores as null into the JSON document.
		 *
		 * @return JSON document containing stored data.
		 */
		QByteArray toJson(void) const;

	private:
		QStringList m_ids; /*!< Location identifiers as obtained from upload_hierarchy. */
		QString m_fileName; /*!< Uploaded file name. */
		QByteArray m_fileContent; /*!< Raw content of uploaded file. */
	};

	/*!
	 * @brief Encapsulates the upload_file response.
	 */
	class UploadFileResp {
	private:
		/*!
		 * @brief Constructor. Creates an invalid structure.
		 */
		UploadFileResp(void);

		/*!
		 * @brief Constructor.
		 *
		 * @param[in] id File identifier.
		 * @param[in] error Error entry.
		 * @param[in] locations List of locations.
		 */
		UploadFileResp(const QString &id, const ErrorEntry &error,
		    const QStringList &locations);

	public:
		/*!
		 * @brief Copy constructor.
		 *
		 * @param[in] other Upload file response.
		 */
		UploadFileResp(const UploadFileResp &other);

		/*!
		 * @brief Return file identifier.
		 *
		 * @return File identifier.
		 */
		const QString &id(void) const;

		/*!
		 * @brief Return error entry.
		 *
		 * @return Error entry.
		 */
		const ErrorEntry &error(void) const;

		/*!
		 * @brief Return location list.
		 *
		 * @return List of places where the file is stored in the service.
		 */
		const QStringList &locations(void) const;

		/*!
		 * @brief Check whether content is valid.
		 *
		 * @return True if content is valid.
		 */
		bool isValid(void) const;

		/*!
		 * @brief Creates a upload file response structure from supplied JSON
		 *     document.
		 *
		 * @param[in]  json JSON document.
		 * @param[out] ok Set to true on success.
		 * @return Invalid structure on error a valid structure else.
		 */
		static
		UploadFileResp fromJson(const QByteArray &json,
		    bool *ok = Q_NULLPTR);

		/*!
		 * @brief Converts upload file response structure into a JSON document.
		 *
		 * @note Unspecified values are stores as null into the JSON document.
		 *
		 * @return JSON document containing stored data.
		 */
		QByteArray toJson(void) const;

	private:
		QString m_id; /*!< Uploaded file identifier (not necessary from ISDS). */
		ErrorEntry m_error; /*!< Brief error entry. */
		QStringList m_locations; /*!< Where the uploaded file is located in the service. */
	};

}
