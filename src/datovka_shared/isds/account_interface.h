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
 * Structures based on from pril_2/ISDS_OTP_autentizace.pdf.
 */

namespace Isds {

	class OtpPrivate;
	/*!
	 * @brief One-time password authentication data.
	 */
	class Otp {
		Q_DECLARE_PRIVATE(Otp)

	public:
		Otp(void);
		Otp(const Otp &other);
#ifdef Q_COMPILER_RVALUE_REFS
		Otp(Otp &&other) Q_DECL_NOEXCEPT;
#endif /* Q_COMPILER_RVALUE_REFS */
		~Otp(void);

		Otp &operator=(const Otp &other) Q_DECL_NOTHROW;
#ifdef Q_COMPILER_RVALUE_REFS
		Otp &operator=(Otp &&other) Q_DECL_NOTHROW;
#endif /* Q_COMPILER_RVALUE_REFS */

		bool operator==(const Otp &other) const;
		bool operator!=(const Otp &other) const;

		friend void swap(Otp &first, Otp &second) Q_DECL_NOTHROW;

		bool isNull(void) const;

		/* Authentication method. */
		enum Type::OtpMethod method(void) const;
		void setMethod(enum Type::OtpMethod m);
		/* One-time password. */
		const QString &otpCode(void) const;
		void setOtpCode(const QString &oc);
#ifdef Q_COMPILER_RVALUE_REFS
		void setOtpCode(QString &&oc);
#endif /* Q_COMPILER_RVALUE_REFS */
		/* Resolution state. */
		enum Type::OtpResolution resolution(void) const;
		void setResolution(enum Type::OtpResolution r);

	private:
		QScopedPointer<OtpPrivate> d_ptr; // std::unique_ptr ?
	};

	void swap(Otp &first, Otp &second) Q_DECL_NOTHROW;

}
