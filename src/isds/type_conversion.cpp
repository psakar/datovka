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

#include "src/isds/type_conversion.h"

enum Isds::Type::NilBool Isds::variant2NilBool(const QVariant &v)
{
	if (v.isNull()) {
		return Type::BOOL_NULL;
	}

	return v.toBool() ? Type::BOOL_TRUE : Type::BOOL_FALSE;
}

QVariant Isds::nilBool2Variant(enum Type::NilBool b)
{
	if (b == Type::BOOL_NULL) {
		return QVariant();
	} else {
		return QVariant(b == Type::BOOL_TRUE);
	}
}

enum Isds::Type::DbType Isds::long2DbType(long int bt)
{
	switch (bt) {
	case Isds::Type::BT_SYSTEM: return Isds::Type::BT_SYSTEM; break;
	case Isds::Type::BT_OVM: return Isds::Type::BT_OVM; break;
	case Isds::Type::BT_OVM_NOTAR: return Isds::Type::BT_OVM_NOTAR; break;
	case Isds::Type::BT_OVM_EXEKUT: return Isds::Type::BT_OVM_EXEKUT; break;
	case Isds::Type::BT_OVM_REQ: return Isds::Type::BT_OVM_REQ; break;
	case Isds::Type::BT_OVM_FO: return Isds::Type::BT_OVM_FO; break;
	case Isds::Type::BT_OVM_PFO: return Isds::Type::BT_OVM_PFO; break;
	case Isds::Type::BT_OVM_PO: return Isds::Type::BT_OVM_PO; break;
	case Isds::Type::BT_PO: return Isds::Type::BT_PO; break;
	case Isds::Type::BT_PO_ZAK: return Isds::Type::BT_PO_ZAK; break;
	case Isds::Type::BT_PO_REQ: return Isds::Type::BT_PO_REQ; break;
	case Isds::Type::BT_PFO: return Isds::Type::BT_PFO; break;
	case Isds::Type::BT_PFO_ADVOK: return Isds::Type::BT_PFO_ADVOK; break;
	case Isds::Type::BT_PFO_DANPOR: return Isds::Type::BT_PFO_DANPOR; break;
	case Isds::Type::BT_PFO_INSSPR: return Isds::Type::BT_PFO_INSSPR; break;
	case Isds::Type::BT_PFO_AUDITOR: return Isds::Type::BT_PFO_AUDITOR; break;
	case Isds::Type::BT_FO: return Isds::Type::BT_FO; break;
	default:
		Q_ASSERT(0);
		return Isds::Type::BT_SYSTEM; /* FIXME */
		break;
	}
}

enum Isds::Type::DbType Isds::variant2DbType(const QVariant &v)
{
	if (v.isNull()) {
		return Type::BT_NULL;
	}

	bool ok = false;
	long int num = v.toLongLong(&ok);
	if (Q_UNLIKELY(!ok)) {
		Q_ASSERT(0);
		return Type::BT_NULL;
	}

	return long2DbType(num);
}

QVariant Isds::dbType2Variant(enum Type::DbType bt)
{
	if (bt == Type::BT_NULL) {
		return QVariant();
	} else {
		return QVariant((int)bt);
	}
}

static const QString strMain("main"), strEncl("encl"), strSign("sign"), strMeta("meta");
static const QString strNull;

enum Isds::Type::FileMetaType Isds::str2FileMetaType(const QString &s)
{
	if (s.isNull()) {
		return Type::FMT_UNKNOWN;
	} else if (s == strMain) {
		return Type::FMT_MAIN;
	} else if (s == strEncl) {
		return Type::FMT_ENCLOSURE;
	} else if (s == strSign) {
		return Type::FMT_SIGNATURE;
	} else if (s == strMeta) {
		return Type::FMT_META;
	} else {
		Q_ASSERT(0);
		return Type::FMT_UNKNOWN;
	}
}

const QString &Isds::fileMetaType2str(enum Type::FileMetaType fmt)
{
	switch (fmt) {
	case Type::FMT_UNKNOWN: return strNull; break;
	case Type::FMT_MAIN: return strMain; break;
	case Type::FMT_ENCLOSURE: return strEncl; break;
	case Type::FMT_SIGNATURE: return strSign; break;
	case Type::FMT_META: return strMeta; break;
	default:
		Q_ASSERT(0);
		return strNull;
		break;
	}
}

enum Isds::Type::FileMetaType Isds::variant2FileMetaType(const QVariant &v)
{
	if (v.isNull()) {
		return Type::FMT_UNKNOWN;
	}

	return Isds::str2FileMetaType(v.toString());
}

QVariant Isds::fileMetaType2Variant(enum Type::FileMetaType fmt)
{
	if (fmt == Type::FMT_UNKNOWN) {
		return QVariant();
	}

	return QVariant(Isds::fileMetaType2str(fmt));
}
