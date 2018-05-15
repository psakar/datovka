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

qint64 Isds::variant2nonNegativeLong(const QVariant &v)
{
	if (v.isNull()) {
		return -1;
	}

	bool ok = false;
	qint64 num = v.toLongLong(&ok);
	if ((!ok) || (num < 0)) {
		return -1;
	}

	return num;
}

QVariant Isds::nonNegativeLong2Variant(qint64 i)
{
	return (i >= 0) ? QVariant(i) : QVariant();
}

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

enum Isds::Type::DbType Isds::long2DbType(long int bt, bool *ok)
{
	bool iOk = true;
	enum Type::DbType type = Type::BT_SYSTEM;

	switch (bt) {
	case Type::BT_SYSTEM: type = Type::BT_SYSTEM; break;
	case Type::BT_OVM: type = Type::BT_OVM; break;
	case Type::BT_OVM_NOTAR: type = Type::BT_OVM_NOTAR; break;
	case Type::BT_OVM_EXEKUT: type = Type::BT_OVM_EXEKUT; break;
	case Type::BT_OVM_REQ: type = Type::BT_OVM_REQ; break;
	case Type::BT_OVM_FO: type = Type::BT_OVM_FO; break;
	case Type::BT_OVM_PFO: type = Type::BT_OVM_PFO; break;
	case Type::BT_OVM_PO: type = Type::BT_OVM_PO; break;
	case Type::BT_PO: type = Type::BT_PO; break;
	case Type::BT_PO_ZAK: type = Type::BT_PO_ZAK; break;
	case Type::BT_PO_REQ: type = Type::BT_PO_REQ; break;
	case Type::BT_PFO: type = Type::BT_PFO; break;
	case Type::BT_PFO_ADVOK: type = Type::BT_PFO_ADVOK; break;
	case Type::BT_PFO_DANPOR: type = Type::BT_PFO_DANPOR; break;
	case Type::BT_PFO_INSSPR: type = Type::BT_PFO_INSSPR; break;
	case Type::BT_PFO_AUDITOR: type = Type::BT_PFO_AUDITOR; break;
	case Type::BT_FO: type = Type::BT_FO; break;
	default:
		Q_ASSERT(0);
		iOk = false;
		break;
	}

	if (ok != Q_NULLPTR) {
		*ok = iOk;
	}
	return type;
}

enum Isds::Type::DbType Isds::variant2DbType(const QVariant &v)
{
	if (v.isNull()) {
		return Type::BT_NULL;
	}

	bool ok = false;
	qint64 num = v.toLongLong(&ok);
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

enum Isds::Type::DbState Isds::long2DbState(long int bs, bool *ok)
{
	bool iOk = true;
	enum Type::DbState state = Type::BS_ERROR;

	switch (bs) {
	case Type::BS_ERROR: state = Type::BS_ERROR; break;
	case Type::BS_ACCESSIBLE: state = Type::BS_ACCESSIBLE; break;
	case Type::BS_TEMP_INACCESSIBLE: state = Type::BS_TEMP_INACCESSIBLE; break;
	case Type::BS_NOT_YET_ACCESSIBLE: state = Type::BS_NOT_YET_ACCESSIBLE; break;
	case Type::BS_PERM_INACCESSIBLE: state = Type::BS_PERM_INACCESSIBLE; break;
	case Type::BS_REMOVED: state = Type::BS_REMOVED; break;
	case Type::BS_TEMP_UNACCESSIBLE_LAW: state = Type::BS_TEMP_UNACCESSIBLE_LAW; break;
	default:
		Q_ASSERT(0);
		iOk = false;
		break;
	}

	if (ok != Q_NULLPTR) {
		*ok = iOk;
	}
	return state;
}

enum Isds::Type::DbState Isds::variant2DbState(const QVariant &v)
{
	if (v.isNull()) {
		return Type::BS_ERROR;
	}

	bool ok = false;
	qint64 num = v.toLongLong(&ok);
	if (Q_UNLIKELY(!ok)) {
		Q_ASSERT(0);
		return Type::BS_ERROR;
	}

	return long2DbState(num);
}

QVariant Isds::dbState2Variant(enum Type::DbState bs)
{
	if (bs != Type::BS_ERROR) {
		return QVariant((int)bs);
	} else {
		return QVariant();
	}
}

Isds::Type::Privileges Isds::long2Privileges(long int p)
{
	Type::Privileges privileges = Type::PRIVIL_NONE;
	if (p & Type::PRIVIL_READ_NON_PERSONAL) {
		privileges |= Type::PRIVIL_READ_NON_PERSONAL;
	}
	if (p & Type::PRIVIL_READ_ALL) {
		privileges |= Type::PRIVIL_READ_ALL;
	}
	if (p & Type::PRIVIL_CREATE_DM) {
		privileges |= Type::PRIVIL_CREATE_DM;
	}
	if (p & Type::PRIVIL_VIEW_INFO) {
		privileges |= Type::PRIVIL_VIEW_INFO;
	}
	if (p & Type::PRIVIL_SEARCH_DB) {
		privileges |= Type::PRIVIL_SEARCH_DB;
	}
	if (p & Type::PRIVIL_OWNER_ADM) {
		privileges |= Type::PRIVIL_OWNER_ADM;
	}
	if (p & Type::PRIVIL_READ_VAULT) {
		privileges |= Type::PRIVIL_READ_VAULT;
	}
	if (p & Type::PRIVIL_ERASE_VAULT) {
		privileges |= Type::PRIVIL_ERASE_VAULT;
	}
	return privileges;
}

Isds::Type::Privileges Isds::variant2Privileges(const QVariant &v)
{
	if (v.isNull()) {
		return Type::PRIVIL_NONE;
	}

	bool ok = false;
	qint64 num = v.toLongLong(&ok);
	if (Q_UNLIKELY(!ok)) {
		Q_ASSERT(0);
		return Type::PRIVIL_NONE;
	}

	return long2Privileges(num);
}

QVariant Isds::privileges2Variant(Type::Privileges p)
{
	return QVariant((int)p);
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
	qint64 num = v.toLongLong(&ok);
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

static const QString strPu("PRIMARY_USER"), strEu("ENTRUSTED_USER"),
    strA("ADMINISTRATOR"), strOu("OFFICIAL_USER"), strOcu("OFFICIAL_CERT_USER"),
    strL("LIQUIDATOR"), strR("RECEIVER"), strG("GUARDIAN");

enum Isds::Type::UserType Isds::str2UserType(const QString &s)
{
	if (s.isNull()) {
		return Type::UT_NULL;
	} else if (s == strPu) {
		return Type::UT_PRIMARY;
	} else if (s == strEu) {
		return Type::UT_ENTRUSTED;
	} else if (s == strA) {
		return Type::UT_ADMINISTRATOR;
	} else if (s == strOu) {
		return Type::UT_OFFICIAL;
	} else if (s == strOcu) {
		return Type::UT_OFFICIAL_CERT;
	} else if (s == strL) {
		return Type::UT_LIQUIDATOR;
	} else if (s == strR) {
		return Type::UT_RECEIVER;
	} else if (s == strG) {
		return Type::UT_GUARDIAN;
	} else {
		Q_ASSERT(0);
		return Type::UT_NULL;
	}
}

const QString &Isds::userType2Str(enum Type::UserType ut)
{
	switch (ut) {
	case Type::UT_NULL: return strNull; break;
	case Type::UT_PRIMARY: return strPu; break;
	case Type::UT_ENTRUSTED: return strEu; break;
	case Type::UT_ADMINISTRATOR: return strA; break;
	case Type::UT_OFFICIAL: return strOu; break;
	case Type::UT_OFFICIAL_CERT: return strOcu; break;
	case Type::UT_LIQUIDATOR: return strL; break;
	case Type::UT_RECEIVER: return strR; break;
	case Type::UT_GUARDIAN: return strG; break;
	default:
		Q_ASSERT(0);
		return strNull;
		break;
	}
}

enum Isds::Type::UserType Isds::variant2UserType(const QVariant &v)
{
	if (v.isNull()) {
		return Type::UT_NULL;
	}

	return str2UserType(v.toString());
}

QVariant Isds::userType2Variant(enum Type::UserType ut)
{
	if (ut == Type::UT_NULL) {
		return QVariant();
	}

	return QVariant(userType2Str(ut));
}

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
