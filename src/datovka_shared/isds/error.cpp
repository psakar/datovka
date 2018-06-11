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

#include "src/datovka_shared/isds/error.h"

static const QString nullString;

/*!
 * @brief PIMPL Error class.
 */
class Isds::ErrorPrivate {
	//Q_DISABLE_COPY(ErrorPrivate)
public:
	ErrorPrivate(void)
	    : m_code(Type::ERR_SUCCESS), m_longDescr()
	{ }

	ErrorPrivate &operator=(const ErrorPrivate &other) Q_DECL_NOTHROW
	{
		m_code = other.m_code;
		m_longDescr = other.m_longDescr;

		return *this;
	}

	bool operator==(const ErrorPrivate &other) const
	{
		return (m_code == other.m_code) &&
		    (m_longDescr == other.m_longDescr);
	}

	enum Type::Error m_code;
	QString m_longDescr;
};

Isds::Error::Error(void)
    : d_ptr(Q_NULLPTR)
{
}

Isds::Error::Error(const Error &other)
    : d_ptr((other.d_func() != Q_NULLPTR) ? (new (std::nothrow) ErrorPrivate) : Q_NULLPTR)
{
	Q_D(Error);
	if (d == Q_NULLPTR) {
		return;
	}

	*d = *other.d_func();
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::Error::Error(Error &&other) Q_DECL_NOEXCEPT
    : d_ptr(other.d_ptr.take()) //d_ptr(std::move(other.d_ptr))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::Error::~Error(void)
{
}

/*!
 * @brief Ensures private error presence.
 *
 * @note Returns if private error could not be allocated.
 */
#define ensureErrorPrivate(_x_) \
	do { \
		if (Q_UNLIKELY(d_ptr == Q_NULLPTR)) { \
			ErrorPrivate *p = new (std::nothrow) ErrorPrivate; \
			if (Q_UNLIKELY(p == Q_NULLPTR)) { \
				Q_ASSERT(0); \
				return _x_; \
			} \
			d_ptr.reset(p); \
		} \
	} while (0)

Isds::Error &Isds::Error::operator=(const Error &other) Q_DECL_NOTHROW
{
	if (other.d_func() == Q_NULLPTR) {
		d_ptr.reset(Q_NULLPTR);
		return *this;
	}
	ensureErrorPrivate(*this);
	Q_D(Error);

	*d = *other.d_func();

	return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::Error &Isds::Error::operator=(Error &&other) Q_DECL_NOTHROW
{
	swap(*this, other);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */

bool Isds::Error::operator==(const Error &other) const
{
	Q_D(const Error);
	if ((d == Q_NULLPTR) && ((other.d_func() == Q_NULLPTR))) {
		return true;
	} else if ((d == Q_NULLPTR) || ((other.d_func() == Q_NULLPTR))) {
		return false;
	}

	return *d == *other.d_func();
}

bool Isds::Error::operator!=(const Error &other) const
{
	return !operator==(other);
}

bool Isds::Error::isNull(void) const
{
	Q_D(const Error);
	return d == Q_NULLPTR;
}

enum Isds::Type::Error Isds::Error::code(void) const
{
	Q_D(const Error);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return Type::ERR_SUCCESS;
	}

	return d->m_code;
}

void Isds::Error::setCode(enum Type::Error c)
{
	ensureErrorPrivate();
	Q_D(Error);
	d->m_code = c;
}

const QString &Isds::Error::longDescr(void) const
{
	Q_D(const Error);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_longDescr;
}

void Isds::Error::setLongDescr(const QString &ld)
{
	ensureErrorPrivate();
	Q_D(Error);
	d->m_longDescr = ld;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Isds::Error::setLongDescr(QString &&ld)
{
	ensureErrorPrivate();
	Q_D(Error);
	d->m_longDescr = ld;
}
#endif /* Q_COMPILER_RVALUE_REFS */

void Isds::swap(Error &first, Error &second) Q_DECL_NOTHROW
{
	using std::swap;
	swap(first.d_ptr, second.d_ptr);
}
