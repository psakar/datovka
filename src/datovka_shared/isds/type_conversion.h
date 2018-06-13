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

#include <QChar>
#include <QString>
#include <QVariant>

#include "src/datovka_shared/isds/types.h"

namespace Isds {

	/* Negative values are converted to null variant. */
	qint64 variant2nonNegativeLong(const QVariant &v);
	/* Null variant is converted to -1. */
	QVariant nonNegativeLong2Variant(qint64 i);

	/* Empty string is converted to null char. First character of string is returned. */
	QChar str2Char(const QString &s);
	/* Null variant is converted to null char. */
	QChar variant2Char(const QVariant &v);

	enum Type::NilBool variant2NilBool(const QVariant &v);
	QVariant nilBool2Variant(enum Type::NilBool b);

	enum Type::DbType long2DbType(long int bt, bool *ok = Q_NULLPTR);
	enum Type::DbType intVariant2DbType(const QVariant &v);
	QVariant dbType2IntVariant(enum Type::DbType bt);
	enum Type::DbType str2DbType(const QString &s);
	const QString &dbType2Str(enum Type::DbType bt);
	enum Type::DbType strVariant2DbType(const QVariant &v);
	QVariant dbType2StrVariant(enum Type::DbType bt);

	enum Type::DbState long2DbState(long int bs, bool *ok = Q_NULLPTR);
	enum Type::DbState variant2DbState(const QVariant &v);
	QVariant dbState2Variant(enum Type::DbState bs);

	Type::Privileges long2Privileges(long int p);
	Type::Privileges variant2Privileges(const QVariant &v);
	QVariant privileges2Variant(Type::Privileges p);

	enum Type::DmState long2DmState(long int ms);
	enum Type::DmState variant2DmState(const QVariant &v);
	QVariant dmState2Variant(enum Type::DmState ms);

	enum Type::UserType str2UserType(const QString &s);
	const QString &userType2Str(enum Type::UserType ut);
	enum Type::UserType variant2UserType(const QVariant &v);
	QVariant userType2Variant(enum Type::UserType ut);

	enum Type::SenderType str2SenderType(const QString &s);
	const QString &senderType2Str(enum Type::SenderType st);
	enum Type::SenderType variant2SenderType(const QVariant &v);
	QVariant senderType2Variant(enum Type::SenderType st);

	enum Type::HashAlg str2HashAlg(const QString &s);
	const QString &hashAlg2Str(enum Type::HashAlg ha);
	enum Type::HashAlg variant2HashAlg(const QVariant &v);
	QVariant hashAlg2Variant(enum Type::HashAlg ha);

	enum Type::FileMetaType str2FileMetaType(const QString &s);
	const QString &fileMetaType2Str(enum Type::FileMetaType fmt);
	enum Type::FileMetaType variant2FileMetaType(const QVariant &v);
	QVariant fileMetaType2Variant(enum Type::FileMetaType fmt);

}
