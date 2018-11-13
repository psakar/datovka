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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
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

#include "src/datovka_shared/gov_services/service/gov_service_form_field.h"

/* Null objects - for convenience. */
static const QString nullString;

/*!
 * @brief PIMPL FormField class.
 */
class Gov::FormFieldPrivate {
	//Q_DISABLE_COPY(FormFieldPrivate)
public:
	FormFieldPrivate(void)
	    : m_key(), m_val(), m_descr(), m_placeholder(),
	    m_properties(FormFieldType::PROP_NONE)
	{ }

	FormFieldPrivate &operator=(const FormFieldPrivate &other) Q_DECL_NOTHROW
	{
		m_key = other.m_key;
		m_val = other.m_val;
		m_descr = other.m_descr;
		m_placeholder = other.m_placeholder;
		m_properties = other.m_properties;

		return *this;
	}

	bool operator==(const FormFieldPrivate &other) const
	{
		return (m_key == other.m_key) &&
		    (m_val == other.m_val) &&
		    (m_descr == other.m_descr) &&
		    (m_placeholder == other.m_placeholder) &&
		    (m_properties == other.m_properties);
	}

	QString m_key; /*!< Internal value identifier. */
	QString m_val; /*!< Supplied value. */
	QString m_descr; /*!< Value description. */
	QString m_placeholder; /*!< Query placeholder text. */
	FormFieldType::Properties m_properties; /*!< Field properties. */
};

Gov::FormField::FormField(void)
    : d_ptr(Q_NULLPTR)
{
}

Gov::FormField::FormField(const FormField &other)
    : d_ptr((other.d_func() != Q_NULLPTR) ? (new (std::nothrow) FormFieldPrivate) : Q_NULLPTR)
{
	Q_D(FormField);
	if (d == Q_NULLPTR) {
		return;
	}

	*d = *other.d_func();
}

#ifdef Q_COMPILER_RVALUE_REFS
Gov::FormField::FormField(FormField &&other) Q_DECL_NOEXCEPT
    : d_ptr(other.d_ptr.take()) //d_ptr(std::move(other.d_ptr))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

Gov::FormField::~FormField(void)
{
}

/*!
 * @brief Ensures private form field presence.
 *
 * @note Returns if private form field could not be allocated.
 */
#define ensureFormFieldPrivate(_x_) \
	do { \
		if (Q_UNLIKELY(d_ptr == Q_NULLPTR)) { \
			FormFieldPrivate *p = new (std::nothrow) FormFieldPrivate; \
			if (Q_UNLIKELY(p == Q_NULLPTR)) { \
				Q_ASSERT(0); \
				return _x_; \
			} \
			d_ptr.reset(p); \
		} \
	} while (0)

Gov::FormField &Gov::FormField::operator=(const FormField &other) Q_DECL_NOTHROW
{
	if (other.d_func() == Q_NULLPTR) {
		d_ptr.reset(Q_NULLPTR);
		return *this;
	}
	ensureFormFieldPrivate(*this);
	Q_D(FormField);

	*d = *other.d_func();

	return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
Gov::FormField &Gov::FormField::operator=(FormField &&other) Q_DECL_NOTHROW
{
	swap(*this, other);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */

bool Gov::FormField::operator==(const FormField &other) const
{
	Q_D(const FormField);
	if ((d == Q_NULLPTR) && ((other.d_func() == Q_NULLPTR))) {
		return true;
	} else if ((d == Q_NULLPTR) || ((other.d_func() == Q_NULLPTR))) {
		return false;
	}

	return *d == *other.d_func();
}

bool Gov::FormField::operator!=(const FormField &other) const
{
	return !operator==(other);
}

bool Gov::FormField::isNull(void) const
{
	Q_D(const FormField);
	return d == Q_NULLPTR;
}

const QString &Gov::FormField::key(void) const
{
	Q_D(const FormField);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_key;
}

void Gov::FormField::setKey(const QString &k)
{
	ensureFormFieldPrivate();
	Q_D(FormField);
	d->m_key = k;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Gov::FormField::setKey(QString &&k)
{
	ensureFormFieldPrivate();
	Q_D(FormField);
	d->m_key = k;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Gov::FormField::val(void) const
{
	Q_D(const FormField);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_val;
}

void Gov::FormField::setVal(const QString &v)
{
	ensureFormFieldPrivate();
	Q_D(FormField);
	d->m_val = v;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Gov::FormField::setVal(QString &&v)
{
	ensureFormFieldPrivate();
	Q_D(FormField);
	d->m_val = v;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Gov::FormField::descr(void) const
{
	Q_D(const FormField);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_descr;
}

void Gov::FormField::setDescr(const QString &de)
{
	ensureFormFieldPrivate();
	Q_D(FormField);
	d->m_descr = de;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Gov::FormField::setDescr(QString &&de)
{
	ensureFormFieldPrivate();
	Q_D(FormField);
	d->m_descr = de;
}
#endif /* Q_COMPILER_RVALUE_REFS */

const QString &Gov::FormField::placeholder(void) const
{
	Q_D(const FormField);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return nullString;
	}

	return d->m_placeholder;
}

void Gov::FormField::setPlaceholder(const QString &p)
{
	ensureFormFieldPrivate();
	Q_D(FormField);
	d->m_placeholder = p;
}

#ifdef Q_COMPILER_RVALUE_REFS
void Gov::FormField::setPlaceholder(QString &&p)
{
	ensureFormFieldPrivate();
	Q_D(FormField);
	d->m_placeholder = p;
}
#endif /* Q_COMPILER_RVALUE_REFS */

Gov::FormFieldType::Properties Gov::FormField::properties(void) const
{
	Q_D(const FormField);
	if (Q_UNLIKELY(d == Q_NULLPTR)) {
		return FormFieldType::PROP_NONE;
	}

	return d->m_properties;
}

void Gov::FormField::setProperties(FormFieldType::Properties p)
{
	ensureFormFieldPrivate();
	Q_D(FormField);
	d->m_properties = p;
}

void Gov::swap(FormField &first, FormField &second) Q_DECL_NOTHROW
{
	using std::swap;
	swap(first.d_ptr, second.d_ptr);
}
