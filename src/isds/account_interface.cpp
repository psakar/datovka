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

#include "src/isds/account_interface.h"

/* Null objects - for convenience. */
static const QString nullString;

/*!
 * @brief PIMPL Otp class.
 */
class Isds::OtpPrivate {
	//Q_DISABLE_COPY(OtpPrivate)
public:
	OtpPrivate(void)
	    : m_method(Type::OM_UNKNOWN), m_otpCode(),
	    m_resolution(Type::OR_UNKNOWN)
	{ }

	OtpPrivate &operator=(const OtpPrivate &other) Q_DECL_NOTHROW
	{
		m_method = other.m_method;
		m_otpCode = other.m_otpCode;
		m_resolution = other.m_resolution;

		return *this;
	}

	bool operator==(const OtpPrivate &other) const
	{
		return (m_method == other.m_method) &&
		    (m_otpCode == other.m_otpCode) &&
		    (m_resolution == other.m_resolution);
	}

	/* Input members. */
	enum Type::OtpMethod m_method; /*!< OTP authentication method to be used. */
	QString m_otpCode; /*!<
	                    * One-time password to use. Pass null value, if you
	                    * don't know it yet (e.g. in case of first phase
	                    * of the time-based OTP to request new code from
	                    * ISDS.)
	                    */
	/* Output members. */
	enum Type::OtpResolution m_resolution; /*!<
	                                        * Fine-grade resolution of OTP
	                                        * authentication state description.
	                                        */
};

Isds::Otp::Otp(void)
    : d_ptr(Q_NULLPTR)
{
}

Isds::Otp::Otp(const Otp &other)
    : d_ptr((other.d_func() != Q_NULLPTR) ? (new (std::nothrow) OtpPrivate) : Q_NULLPTR)
{
	Q_D(Otp);
	if (d == Q_NULLPTR) {
		return;
	}

	*d = *other.d_func();
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::Otp::Otp(Otp &&other) Q_DECL_NOEXCEPT
    : d_ptr(other.d_ptr.take()) //d_ptr(std::move(other.d_ptr))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::Otp::~Otp(void)
{
}

/*!
 * @brief Ensures private OTP presence.
 *
 * @note Returns if private OTP could not be allocated.
 */
#define ensureOtpPrivate(_x_) \
	do { \
		if (Q_UNLIKELY(d_ptr == Q_NULLPTR)) { \
			OtpPrivate *p = new (std::nothrow) OtpPrivate; \
			if (Q_UNLIKELY(p == Q_NULLPTR)) { \
				Q_ASSERT(0); \
				return _x_; \
			} \
			d_ptr.reset(p); \
		} \
	} while (0)

Isds::Otp &Isds::Otp::operator=(const Otp &other) Q_DECL_NOTHROW
{
	if (other.d_func() == Q_NULLPTR) {
		d_ptr.reset(Q_NULLPTR);
		return *this;
	}
	ensureOtpPrivate(*this);
	Q_D(Otp);

	*d = *other.d_func();

	return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::Otp &Isds::Otp::operator=(Otp &&other) Q_DECL_NOTHROW
{
	swap(*this, other);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */

bool Isds::Otp::operator==(const Otp &other) const
{
	Q_D(const Otp);
	if ((d == Q_NULLPTR) && ((other.d_func() == Q_NULLPTR))) {
		return true;
	} else if ((d == Q_NULLPTR) || ((other.d_func() == Q_NULLPTR))) {
		return false;
	}

	return *d == *other.d_func();
}

bool Isds::Otp::operator!=(const Otp &other) const
{
	return !operator==(other);
}

bool Isds::Otp::isNull(void) const
{
	Q_D(const Otp);
	return d == Q_NULLPTR;
}

enum Isds::Type::OtpMethod Isds::Otp::method(void) const
{
	Q_D(const Otp);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return Type::OM_UNKNOWN;
	}

	return d->m_method;
}

void Isds::Otp::setMethod(enum Type::OtpMethod m)
{
	ensureOtpPrivate();
	Q_D(Otp);
	d->m_method = m;
}

const QString &Isds::Otp::otpCode(void) const
{
	Q_D(const Otp);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_otpCode;
}

void Isds::Otp::setOtpCode(const QString &oc)
{
	ensureOtpPrivate();
	Q_D(Otp);
	d->m_otpCode = oc;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Otp::setOtpCode(QString &&oc)
{
	ensureOtpPrivate();
	Q_D(Otp);
	d->m_otpCode = oc;
}
#endif /* Q_COMPILER_RVALUE_REFS */

enum Isds::Type::OtpResolution Isds::Otp::resolution(void) const
{
	Q_D(const Otp);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return Type::OR_UNKNOWN;
	}

	return d->m_resolution;
}

void Isds::Otp::setResolution(enum Type::OtpResolution r)
{
	ensureOtpPrivate();
	Q_D(Otp);
	d->m_resolution = r;
}

void Isds::swap(Otp &first, Otp &second) Q_DECL_NOTHROW
{
	using std::swap;
	swap(first.d_ptr, second.d_ptr);
}
