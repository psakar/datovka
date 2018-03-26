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

#if defined(__APPLE__) || defined(__clang__)
#  define __USE_C99_MATH
#  define _Bool bool
#else /* !__APPLE__ */
#  include <cstdbool>
#endif /* __APPLE__ */

#include <isds.h>
#include <utility> /* std::move */

#include "src/isds/box_management.h"

Isds::Address::Address(const Address &other)
    : m_adCity(other.m_adCity),
    m_adStreet(other.m_adStreet),
    m_adNumberInStreet(other.m_adNumberInStreet),
    m_adNumberInMunicipality(other.m_adNumberInMunicipality),
    m_adZipCode(other.m_adZipCode),
    m_adState(other.m_adState)
{
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::Address::Address(Address &&other) Q_DECL_NOEXCEPT
    : m_adCity(std::move(other.m_adCity)),
    m_adStreet(std::move(other.m_adStreet)),
    m_adNumberInStreet(std::move(other.m_adNumberInStreet)),
    m_adNumberInMunicipality(std::move(other.m_adNumberInMunicipality)),
    m_adZipCode(std::move(other.m_adZipCode)),
    m_adState(std::move(other.m_adState))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::Address &Isds::Address::operator=(const Address &other) Q_DECL_NOTHROW
{
	m_adCity = other.m_adCity;
	m_adStreet = other.m_adStreet;
	m_adNumberInStreet = other.m_adNumberInStreet;
	m_adNumberInMunicipality = other.m_adNumberInMunicipality;
	m_adZipCode = other.m_adZipCode;
	m_adState = other.m_adState;
	return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::Address &Isds::Address::operator=(Address &&other) Q_DECL_NOTHROW
{
	std::swap(m_adCity, other.m_adCity);
	std::swap(m_adStreet, other.m_adStreet);
	std::swap(m_adNumberInStreet, other.m_adNumberInStreet);
	std::swap(m_adNumberInMunicipality, other.m_adNumberInMunicipality);
	std::swap(m_adZipCode, other.m_adZipCode);
	std::swap(m_adState, other.m_adState);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::BirthInfo::BirthInfo(const BirthInfo &other)
    : m_biDate(other.m_biDate),
    m_biCity(other.m_biCity),
    m_biCounty(other.m_biCounty),
    m_biState(other.m_biState)
{
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::BirthInfo::BirthInfo(BirthInfo &&other) Q_DECL_NOEXCEPT
    : m_biDate(std::move(other.m_biDate)),
    m_biCity(std::move(other.m_biCity)),
    m_biCounty(std::move(other.m_biCounty)),
    m_biState(std::move(other.m_biState))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::BirthInfo &Isds::BirthInfo::operator=(const BirthInfo &other) Q_DECL_NOTHROW
{
	m_biDate = other.m_biDate;
	m_biCity = other.m_biCity;
	m_biCounty = other.m_biCounty;
	m_biState = other.m_biState;
	return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::BirthInfo &Isds::BirthInfo::operator=(BirthInfo &&other) Q_DECL_NOTHROW
{
	std::swap(m_biDate, other.m_biDate);
	std::swap(m_biCity, other.m_biCity);
	std::swap(m_biCounty, other.m_biCounty);
	std::swap(m_biState, other.m_biState);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::PersonName::PersonName(const PersonName &other)
    : m_pnFirstName(other.m_pnFirstName),
    m_pnMiddleName(other.m_pnMiddleName),
    m_pnLastName(other.m_pnLastName),
    m_pnLastNameAtBirth(other.m_pnLastNameAtBirth)
{
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::PersonName::PersonName(PersonName &&other) Q_DECL_NOEXCEPT
    : m_pnFirstName(std::move(other.m_pnFirstName)),
    m_pnMiddleName(std::move(other.m_pnMiddleName)),
    m_pnLastName(std::move(other.m_pnLastName)),
    m_pnLastNameAtBirth(std::move(other.m_pnLastNameAtBirth))
{
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::PersonName &Isds::PersonName::operator=(const PersonName &other) Q_DECL_NOTHROW
{
	m_pnFirstName = other.m_pnFirstName;
	m_pnMiddleName = other.m_pnMiddleName;
	m_pnLastName = other.m_pnLastName;
	m_pnLastNameAtBirth = other.m_pnLastNameAtBirth;
	return *this;
}

#ifdef Q_COMPILER_RVALUE_REFS
Isds::PersonName &Isds::PersonName::operator=(PersonName &&other) Q_DECL_NOTHROW
{
	std::swap(m_pnFirstName, other.m_pnFirstName);
	std::swap(m_pnMiddleName, other.m_pnMiddleName);
	std::swap(m_pnLastName, other.m_pnLastName);
	std::swap(m_pnLastNameAtBirth, other.m_pnLastNameAtBirth);
	return *this;
}
#endif /* Q_COMPILER_RVALUE_REFS */

Isds::DbOwnerInfo::DbOwnerInfo(void)
    : m_dataPtr(NULL)
{
}

Isds::DbOwnerInfo::~DbOwnerInfo(void)
{
	if (m_dataPtr != NULL) {
		isds_DbOwnerInfo_free((struct isds_DbOwnerInfo**) &m_dataPtr);
	}
}
