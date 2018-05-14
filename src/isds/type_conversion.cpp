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
	case Type::BT_SYSTEM: return Type::BT_SYSTEM; break;
	case Type::BT_OVM: return Type::BT_OVM; break;
	case Type::BT_OVM_NOTAR: return Type::BT_OVM_NOTAR; break;
	case Type::BT_OVM_EXEKUT: return Type::BT_OVM_EXEKUT; break;
	case Type::BT_OVM_REQ: return Type::BT_OVM_REQ; break;
	case Type::BT_OVM_FO: return Type::BT_OVM_FO; break;
	case Type::BT_OVM_PFO: return Type::BT_OVM_PFO; break;
	case Type::BT_OVM_PO: return Type::BT_OVM_PO; break;
	case Type::BT_PO: return Type::BT_PO; break;
	case Type::BT_PO_ZAK: return Type::BT_PO_ZAK; break;
	case Type::BT_PO_REQ: return Type::BT_PO_REQ; break;
	case Type::BT_PFO: return Type::BT_PFO; break;
	case Type::BT_PFO_ADVOK: return Type::BT_PFO_ADVOK; break;
	case Type::BT_PFO_DANPOR: return Type::BT_PFO_DANPOR; break;
	case Type::BT_PFO_INSSPR: return Type::BT_PFO_INSSPR; break;
	case Type::BT_PFO_AUDITOR: return Type::BT_PFO_AUDITOR; break;
	case Type::BT_FO: return Type::BT_FO; break;
	default:
		Q_ASSERT(0);
		return Type::BT_SYSTEM; /* FIXME */
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

enum Isds::Type::DmState Isds::long2DmState(long int ms)
{
	switch (ms) {
	case Type::MS_NULL: return Type::MS_NULL; break;
	case Type::MS_POSTED: return Type::MS_POSTED; break;
	case Type::MS_STAMPED: return Type::MS_STAMPED; break;
	case Type::MS_INFECTED: return Type::MS_INFECTED; break;
	case Type::MS_DELIVERED: return Type::MS_DELIVERED; break;
	case Type::MS_ACCEPTED_FICT: return Type::MS_ACCEPTED_FICT; break;
	case Type::MS_ACCEPTED: return Type::MS_ACCEPTED; break;
	case Type::MS_READ: return Type::MS_READ; break;
	case Type::MS_UNDELIVERABLE: return Type::MS_UNDELIVERABLE; break;
	case Type::MS_REMOVED: return Type::MS_REMOVED; break;
	case Type::MS_IN_VAULT: return Type::MS_IN_VAULT; break;
	default:
		Q_ASSERT(0);
		return Type::MS_NULL;
		break;
	}
}

enum Isds::Type::DmState Isds::variant2DmState(const QVariant &v)
{
	if (v.isNull()) {
		return Type::MS_NULL;
	}

	bool ok = false;
	long int num = v.toLongLong(&ok);
	if (Q_UNLIKELY(!ok)) {
		Q_ASSERT(0);
		return Type::MS_NULL;
	}

	return long2DmState(num);
}

QVariant Isds::dmState2Variant(enum Type::DmState ms)
{
	if (ms == Type::MS_NULL) {
		return QVariant();
	} else {
		return QVariant((int)ms);
	}
}

static const QString strNull;

static const QString strMd5("MD5"), strSha1("SHA-1"), strSha224("SHA-224"),
    strSha256("SHA-256"), strSha384("SHA-384"), strSha512("SHA-512");

enum Isds::Type::HashAlg Isds::str2HashAlg(const QString &s)
{
	if (s.isNull()) {
		return Type::HA_UNKNOWN;
	} else if (s == strMd5) {
		return Type::HA_MD5;
	} else if (s == strSha1) {
		return Type::HA_SHA_1;
	} else if (s == strSha224) {
		return Type::HA_SHA_224;
	} else if (s == strSha256) {
		return Type::HA_SHA_256;
	} else if (s == strSha384) {
		return Type::HA_SHA_384;
	} else if (s == strSha512) {
		return Type::HA_SHA_512;
	} else {
		Q_ASSERT(0);
		return Type::HA_UNKNOWN;
	}
}

const QString &Isds::hashAlg2Str(enum Type::HashAlg ha)
{
	switch (ha) {
	case Type::HA_UNKNOWN: return strNull; break;
	case Type::HA_MD5: return strMd5; break;
	case Type::HA_SHA_1: return strSha1; break;
	case Type::HA_SHA_224: return strSha224; break;
	case Type::HA_SHA_256: return strSha256; break;
	case Type::HA_SHA_384: return strSha384; break;
	case Type::HA_SHA_512: return strSha512; break;
	default:
		Q_ASSERT(0);
		return strNull;
		break;
	}
}

enum Isds::Type::HashAlg Isds::variant2HashAlg(const QVariant &v)
{
	if (v.isNull()) {
		return Type::HA_UNKNOWN;
	}

	return str2HashAlg(v.toString());
}

QVariant Isds::hashAlg2Variant(enum Type::HashAlg ha)
{
	if (ha == Type::HA_UNKNOWN) {
		return QVariant();
	}

	return QVariant(hashAlg2Str(ha));
}

static const QString strMain("main"), strEncl("encl"), strSign("sign"), strMeta("meta");

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

const QString &Isds::fileMetaType2Str(enum Type::FileMetaType fmt)
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

	return str2FileMetaType(v.toString());
}

QVariant Isds::fileMetaType2Variant(enum Type::FileMetaType fmt)
{
	if (fmt == Type::FMT_UNKNOWN) {
		return QVariant();
	}

	return QVariant(fileMetaType2Str(fmt));
}
