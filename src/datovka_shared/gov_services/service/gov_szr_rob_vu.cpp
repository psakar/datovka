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

#include "src/datovka_shared/gov_services/service/gov_szr_rob_vu.h"

static const char xml_template[] =
"<?xml version='1.0' encoding='utf-8'?>""\n"
"<d:FormularData xmlns:d=\"http://software602.cz/formulare\">""\n"
"  <d:Identifikace>""\n"
"    <d:Formular>ZRDS1</d:Formular>""\n"
"    <d:Zadatel>""\n"
"      <d:Aifo/>""\n"
"      <d:Idds/>""\n"
"      <d:DatumNarozeni/>""\n"
"      <d:Jmeno/>""\n"
"      <d:MistoNarozeniNazev/>""\n"
"      <d:Prijmeni/>""\n"
"      <d:Doklady>""\n"
"        <d:Cislo/>""\n"
"        <d:Druh/>""\n"
"      </d:Doklady>""\n"
"      <d:AdresaPobytu>""\n"
"        <d:OkresNazev/>""\n"
"        <d:ObecNazev/>""\n"
"        <d:CastObceNazev/>""\n"
"        <d:UliceNazev/>""\n"
"        <d:PostaKod/>""\n"
"        <d:TypCislaDomovnihoKod/>""\n"
"        <d:CisloDomovni/>""\n"
"        <d:CisloOrientacni/>""\n"
"        <d:CisloOrientacniPismeno/>""\n"
"      </d:AdresaPobytu>""\n"
"    </d:Zadatel>""\n"
"  </d:Identifikace>""\n"
"  <d:Parametry>""\n"
"    <d:SeznamUdaju/>""\n"
"  </d:Parametry>""\n"
"</d:FormularData>";

class SzrRobVuData {
	Q_DECLARE_TR_FUNCTIONS(SzrRobVuData)

public:
	SzrRobVuData(void)
	{ }

	QList<Gov::FormField> allFields(void) const;
};

QList<Gov::FormField> SzrRobVuData::allFields(void) const
{
	return QList<Gov::FormField>();
}

Gov::SrvcSzrRobVu::SrvcSzrRobVu(void)
    : Service()
{
	m_formFields = SzrRobVuData().allFields();
}

Gov::Service *Gov::SrvcSzrRobVu::createNew(void) const
{
	return new (std::nothrow) SrvcSzrRobVu;
}

const QString &Gov::SrvcSzrRobVu::internalId(void) const
{
	static const QString shortName("SrvcSzrRobVu");
	return shortName;
}

const QString &Gov::SrvcSzrRobVu::fullName(void) const
{
	static const QString fullName(tr("Printout from the resident register"));
	// "Výpis z Registru obyvatel"
	return fullName;
}

const QString &Gov::SrvcSzrRobVu::instituteName(void) const
{
	static const QString instituteName(GOV_SZR_FULL_NAME);
	return instituteName;
}

const QString &Gov::SrvcSzrRobVu::boxId(void) const
{
	static const QString boxId(GOV_SZR_DB_ID);
	return boxId;
}

const QString &Gov::SrvcSzrRobVu::dmAnnotation(void) const
{
	static const QString dmAnnotation("Výpis z Registru obyvatel");
	return dmAnnotation;
}

const QString &Gov::SrvcSzrRobVu::dmSenderIdent(void) const
{
	static const QString dmSenderIdent; /* Null string. */
	return dmSenderIdent;
}

const QString &Gov::SrvcSzrRobVu::dmFileDescr(void) const
{
	static const QString dmFileDescr(GOV_SZR_XML_FILE_NAME);
	return dmFileDescr;
}

bool Gov::SrvcSzrRobVu::canSend(enum Isds::Type::DbType dbType) const
{
	switch (dbType) {
	case Isds::Type::BT_OVM_FO:
	case Isds::Type::BT_FO:
		return true;
		break;
	default:
		return false;
		break;
	}
}

bool Gov::SrvcSzrRobVu::setFieldVal(const QString &key, const QString &val)
{
	return Service::setFieldVal(key, val);
}

bool Gov::SrvcSzrRobVu::setOwnerInfoFields(const Isds::DbOwnerInfo &dbOwnerInfo)
{
	if (Q_UNLIKELY(dbOwnerInfo.isNull())) {
		return false;
	}

	return true;
}

bool Gov::SrvcSzrRobVu::haveAllValidFields(QString *errDescr)
{
	if (errDescr != Q_NULLPTR) {
		errDescr->clear();
	}
	return true;
}

QByteArray Gov::SrvcSzrRobVu::binaryXmlContent(void) const
{
	return QString(xml_template).toUtf8();
}
