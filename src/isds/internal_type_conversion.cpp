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

#include <QtGlobal> /* Q_ASSERT() */

#include "src/isds/internal_type_conversion.h"

enum Isds::Type::DbType IsdsInternal::libisdsDbType2DbType(const isds_DbType ibt,
    bool *ok)
{
	bool iOk = true;
	enum Isds::Type::DbType type = Isds::Type::BT_NULL;

	switch (ibt) {
	case DBTYPE_SYSTEM: type = Isds::Type::BT_SYSTEM; break;
	case DBTYPE_OVM: type = Isds::Type::BT_OVM; break;
	case DBTYPE_OVM_NOTAR: type = Isds::Type::BT_OVM_NOTAR; break;
	case DBTYPE_OVM_EXEKUT: type = Isds::Type::BT_OVM_EXEKUT; break;
	case DBTYPE_OVM_REQ: type = Isds::Type::BT_OVM_REQ; break;
	case DBTYPE_OVM_FO: type = Isds::Type::BT_OVM_FO; break;
	case DBTYPE_OVM_PFO: type = Isds::Type::BT_OVM_PFO; break;
	case DBTYPE_OVM_PO: type = Isds::Type::BT_OVM_PO; break;
	case DBTYPE_PO: type = Isds::Type::BT_PO; break;
	case DBTYPE_PO_ZAK: type = Isds::Type::BT_PO_ZAK; break;
	case DBTYPE_PO_REQ: type = Isds::Type::BT_PO_REQ; break;
	case DBTYPE_PFO: type = Isds::Type::BT_PFO; break;
	case DBTYPE_PFO_ADVOK: type = Isds::Type::BT_PFO_ADVOK; break;
	case DBTYPE_PFO_DANPOR: type = Isds::Type::BT_PFO_DANPOR; break;
	case DBTYPE_PFO_INSSPR: type = Isds::Type::BT_PFO_INSSPR; break;
	case DBTYPE_PFO_AUDITOR: type = Isds::Type::BT_PFO_AUDITOR; break;
	case DBTYPE_FO: type = Isds::Type::BT_FO; break;
	default:
		Q_ASSERT(0);
		iOk = false;
		type = Isds::Type::BT_SYSTEM; /* FIXME */
		break;
	}

	if (ok != Q_NULLPTR) {
		*ok = iOk;
	}
	return type;
}

isds_DbType IsdsInternal::dbType2libisdsDbType(enum Isds::Type::DbType bt,
    bool *ok)
{
	bool iOk = true;
	isds_DbType ibt = DBTYPE_SYSTEM;

	switch (bt) {
	/*
	 * Isds::Type::BT_NULL same as default.
	 */
	case Isds::Type::BT_SYSTEM: ibt = DBTYPE_SYSTEM; break;
	case Isds::Type::BT_OVM: ibt = DBTYPE_OVM; break;
	case Isds::Type::BT_OVM_NOTAR: ibt = DBTYPE_OVM_NOTAR; break;
	case Isds::Type::BT_OVM_EXEKUT: ibt = DBTYPE_OVM_EXEKUT; break;
	case Isds::Type::BT_OVM_REQ: ibt = DBTYPE_OVM_REQ; break;
	case Isds::Type::BT_OVM_FO: ibt = DBTYPE_OVM_FO; break;
	case Isds::Type::BT_OVM_PFO: ibt = DBTYPE_OVM_PFO; break;
	case Isds::Type::BT_OVM_PO: ibt = DBTYPE_OVM_PO; break;
	case Isds::Type::BT_PO: ibt = DBTYPE_PO; break;
	case Isds::Type::BT_PO_ZAK: ibt = DBTYPE_PO_ZAK; break;
	case Isds::Type::BT_PO_REQ: ibt = DBTYPE_PO_REQ; break;
	case Isds::Type::BT_PFO: ibt = DBTYPE_PFO; break;
	case Isds::Type::BT_PFO_ADVOK: ibt = DBTYPE_PFO_ADVOK; break;
	case Isds::Type::BT_PFO_DANPOR: ibt = DBTYPE_PFO_DANPOR; break;
	case Isds::Type::BT_PFO_INSSPR: ibt = DBTYPE_PFO_INSSPR; break;
	case Isds::Type::BT_PFO_AUDITOR: ibt = DBTYPE_PFO_AUDITOR; break;
	case Isds::Type::BT_FO: ibt = DBTYPE_FO; break;
	default:
		Q_ASSERT(0);
		iOk = false;
		break;
	}

	if (ok != Q_NULLPTR) {
		*ok = iOk;
	}
	return ibt;
}

enum Isds::Type::OtpMethod IsdsInternal::libisdsOtpMethod2OtpMethod(
    isds_otp_method iom, bool *ok)
{
	bool iOk = true;
	enum Isds::Type::OtpMethod method = Isds::Type::OM_UNKNOWN;

	switch (iom) {
	case OTP_HMAC: method = Isds::Type::OM_HMAC; break;
	case OTP_TIME: method = Isds::Type::OM_TIME; break;
	default:
		Q_ASSERT(0);
		iOk = false;
		break;
	}

	if (ok != Q_NULLPTR) {
		*ok = iOk;
	}
	return method;
}

isds_otp_method IsdsInternal::otpMethod2libisdsOtpMethod(
    enum Isds::Type::OtpMethod om, bool *ok)
{
	bool iOk = true;
	isds_otp_method iom = OTP_HMAC;

	switch (om) {
	/*
	 * Isds::Type::OM_UNKNOWN same as default.
	 */
	case Isds::Type::OM_HMAC: iom = OTP_HMAC; break;
	case Isds::Type::OM_TIME: iom = OTP_TIME; break;
	default:
		Q_ASSERT(0);
		iOk = false;
		break;
	}

	if (ok != Q_NULLPTR) {
		*ok = iOk;
	}
	return iom;
}

enum Isds::Type::OtpResolution IsdsInternal::libisdsOtpResolution2OtpResolution(
    isds_otp_resolution ior, bool *ok)
{
	bool iOk = true;
	enum Isds::Type::OtpResolution resolution = Isds::Type::OR_UNKNOWN;

	switch (ior) {
	case OTP_RESOLUTION_SUCCESS: resolution = Isds::Type::OR_SUCCESS; break;
	case OTP_RESOLUTION_UNKNOWN: resolution = Isds::Type::OR_UNKNOWN; break;
	case OTP_RESOLUTION_BAD_AUTHENTICATION: resolution = Isds::Type::OR_BAD_AUTH; break;
	case OTP_RESOLUTION_ACCESS_BLOCKED: resolution = Isds::Type::OR_BLOCKED; break;
	case OTP_RESOLUTION_PASSWORD_EXPIRED: resolution = Isds::Type::OR_PWD_EXPIRED; break;
	case OTP_RESOLUTION_TO_FAST: resolution = Isds::Type::OR_TOO_FAST; break;
	case OTP_RESOLUTION_UNAUTHORIZED: resolution = Isds::Type::OR_UNAUTHORIZED; break;
	case OTP_RESOLUTION_TOTP_SENT: resolution = Isds::Type::OR_TOTP_SENT; break;
	case OTP_RESOLUTION_TOTP_NOT_SENT: resolution = Isds::Type::OR_TOTP_NOT_SENT; break;
	default:
		Q_ASSERT(0);
		iOk = false;
		break;
	}

	if (ok != Q_NULLPTR) {
		*ok = iOk;
	}
	return resolution;
}

isds_otp_resolution IsdsInternal::otpResolution2libisdsOtpResolution(
    enum Isds::Type::OtpResolution ores, bool *ok)
{
	bool iOk = true;
	isds_otp_resolution ior = OTP_RESOLUTION_UNKNOWN;

	switch (ores) {
	case Isds::Type::OR_SUCCESS: ior = OTP_RESOLUTION_SUCCESS; break;
	case Isds::Type::OR_UNKNOWN: ior = OTP_RESOLUTION_UNKNOWN; break;
	case Isds::Type::OR_BAD_AUTH: ior = OTP_RESOLUTION_BAD_AUTHENTICATION; break;
	case Isds::Type::OR_BLOCKED: ior = OTP_RESOLUTION_ACCESS_BLOCKED; break;
	case Isds::Type::OR_PWD_EXPIRED: ior = OTP_RESOLUTION_PASSWORD_EXPIRED; break;
	case Isds::Type::OR_TOO_FAST: ior = OTP_RESOLUTION_TO_FAST; break;
	case Isds::Type::OR_UNAUTHORIZED: ior = OTP_RESOLUTION_UNAUTHORIZED; break;
	case Isds::Type::OR_TOTP_SENT: ior = OTP_RESOLUTION_TOTP_SENT; break;
	case Isds::Type::OR_TOTP_NOT_SENT: ior = OTP_RESOLUTION_TOTP_NOT_SENT; break;
	break;
		Q_ASSERT(0);
		iOk = false;
		break;
	}

	if (ok != Q_NULLPTR) {
		*ok = iOk;
	}
	return ior;
}
