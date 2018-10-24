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

#include "src/datovka_shared/gov_services/service/gov_szr_ros_vv.h"

static const char xml_template[] =
"<?xml version='1.0' encoding='utf-8'?>""\n"
"<d:FormularData xmlns:d=\"http://software602.cz/formulare\" "
  "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
  "xmlns:date=\"http://exslt.org/dates-and-times\" "
  "xsi:schemaLocation=\"http://software602.cz/formulare FormularData_ZRDS11.xsd\" "
  "version=\"1.3o\">""\n"
"  <d:Identifikace>""\n"
"    <d:Formular>ZRDS3</d:Formular>""\n"
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
"    <d:Osoba>""\n"
"      <d:Ico>__repl_ICO_ICO__</d:Ico>""\n"
"      <d:NazevOsoby/>""\n"
"      <d:AdresaSidla>""\n"
"        <d:AdresaTextem/>""\n"
"        <d:OkresNazev/>""\n"
"        <d:ObecNazev/>""\n"
"        <d:CastObceNazev/>""\n"
"        <d:UliceNazev/>""\n"
"        <d:PostaKod/>""\n"
"        <d:TypCislaDomovnihoKod/>""\n"
"        <d:CisloDomovni/>""\n"
"        <d:CisloOrientacni/>""\n"
"        <d:CisloOrientacniPismeno/>""\n"
"      </d:AdresaSidla>""\n"
"    </d:Osoba>""\n"
"  </d:Identifikace>""\n"
"  <d:Parametry/>""\n"
"</d:FormularData>";

#define IC_KEY "ic"

class SzrRosVvData {
	Q_DECLARE_TR_FUNCTIONS(SzrRosVvData)

public:
	SzrRosVvData(void)
	    : m_userIc()
	{ }

	QList<Gov::FormField> allFields(void) const;

private:
	QString m_userIc; /*!< Identification number. */
};

QList<Gov::FormField> SzrRosVvData::allFields(void) const
{
	QList<Gov::FormField> formList;

	{
		Gov::FormField ff;
		ff.setKey(IC_KEY);
		ff.setVal(m_userIc);
		ff.setDescr(tr("Subject ID (IČ)"));
		ff.setPlaceholder(tr("Enter subject ID (IČ)"));
		ff.setProperties(Gov::FormFieldType::PROP_MANDATORY |
		    Gov::FormFieldType::PROP_USER_INPUT);
		formList.append(ff);
	}

	return formList;
}

Gov::SrvcSzrRosVv::SrvcSzrRosVv(void)
    : Service()
{
	m_formFields = SzrRosVvData().allFields();
}

Gov::Service *Gov::SrvcSzrRosVv::createNew(void) const
{
	return new (std::nothrow) SrvcSzrRosVv;
}

const QString &Gov::SrvcSzrRosVv::internalId(void) const
{
	static const QString shortName("SrvcSzrRosVv");
	return shortName;
}

const QString &Gov::SrvcSzrRosVv::fullName(void) const
{
	static const QString fullName(tr("Public printout from the person register"));
	// "Veřejný výpis z registru osob"
	return fullName;
}

const QString &Gov::SrvcSzrRosVv::instituteName(void) const
{
	static const QString instituteName(GOV_SZR_FULL_NAME);
	return instituteName;
}

const QString &Gov::SrvcSzrRosVv::boxId(void) const
{
	static const QString boxId(GOV_SZR_DB_ID);
	return boxId;
}

const QString &Gov::SrvcSzrRosVv::dmAnnotation(void) const
{
	static const QString dmAnnotation("Veřejný výpis z registru osob");
	return dmAnnotation;
}

const QString &Gov::SrvcSzrRosVv::dmSenderIdent(void) const
{
	static const QString dmSenderIdent; /* Null string. */
	return dmSenderIdent;
}

const QString &Gov::SrvcSzrRosVv::dmFileDescr(void) const
{
	static const QString dmFileDescr(GOV_SZR_XML_FILE_NAME);
	return dmFileDescr;
}

bool Gov::SrvcSzrRosVv::canSend(enum Isds::Type::DbType dbType) const
{
	switch (dbType) {
	case Isds::Type::BT_OVM_FO:
	case Isds::Type::BT_OVM_PFO:
	case Isds::Type::BT_OVM_PO:
	case Isds::Type::BT_PO:
	case Isds::Type::BT_PO_ZAK:
	case Isds::Type::BT_PO_REQ:
	case Isds::Type::BT_PFO:
	case Isds::Type::BT_PFO_ADVOK:
	case Isds::Type::BT_PFO_DANPOR:
	case Isds::Type::BT_PFO_INSSPR:
	case Isds::Type::BT_PFO_AUDITOR:
	case Isds::Type::BT_FO:
		return true;
		break;
	default:
		return false;
		break;
	}
}

bool Gov::SrvcSzrRosVv::setFieldVal(const QString &key, const QString &val)
{
	return Service::setFieldVal(key, val);
}

bool Gov::SrvcSzrRosVv::setOwnerInfoFields(const Isds::DbOwnerInfo &dbOwnerInfo)
{
	if (Q_UNLIKELY(dbOwnerInfo.isNull())) {
		return false;
	}

	return true;
}

bool Gov::SrvcSzrRosVv::haveAllValidFields(QString *errDescr)
{
	return Service::checkIc(IC_KEY, errDescr);
}

QByteArray Gov::SrvcSzrRosVv::binaryXmlContent(void) const
{
	QString xml(xml_template);
	xml.replace("__repl_ICO_ICO__", fieldVal(IC_KEY));
	return xml.toUtf8();
}
