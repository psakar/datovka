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

#include <QScopedPointer>
#include <QString>

#include "src/datovka_shared/isds/types.h"

/*
 * Encapsulates error structure as it is more or less used by libisds.
 */

namespace Isds {

	class ErrorPrivate;
	/*!
	 * @brief Describes an error situation as it may occur while
	 * communicating with ISDS.
	 */
	class Error {
		Q_DECLARE_PRIVATE(Error)

	public:
		Error(void);
		Error(const Error &other);
#ifdef Q_COMPILER_RVALUE_REFS
		Error(Error &&other) Q_DECL_NOEXCEPT;
#endif /* Q_COMPILER_RVALUE_REFS */
		~Error(void);

		Error &operator=(const Error &other) Q_DECL_NOTHROW;
#ifdef Q_COMPILER_RVALUE_REFS
		Error &operator=(Error &&other) Q_DECL_NOTHROW;
#endif /* Q_COMPILER_RVALUE_REFS */

		bool operator==(const Error &other) const;
		bool operator!=(const Error &other) const;

		friend void swap(Error &first, Error &second) Q_DECL_NOTHROW;

		bool isNull(void) const;

		/* error code */
		enum Type::Error code(void) const;
		void setCode(enum Type::Error c);
		/* long error description - detail */
		const QString &longDescr(void) const;
		void setLongDescr(const QString &ld);
#ifdef Q_COMPILER_RVALUE_REFS
		void setLongDescr(QString &&ld);
#endif /* Q_COMPILER_RVALUE_REFS */

	private:
		QScopedPointer<ErrorPrivate> d_ptr; // std::unique_ptr ?
	};

	void swap(Error &first, Error &second) Q_DECL_NOTHROW;

}
