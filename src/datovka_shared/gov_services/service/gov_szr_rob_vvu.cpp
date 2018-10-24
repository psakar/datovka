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

#include <QDate>

#include "src/datovka_shared/gov_services/service/gov_szr_rob_vvu.h"

static const char xml_template[] =
"<?xml version='1.0' encoding='utf-8'?>""\n"
"<d:FormularData xmlns:d=\"http://software602.cz/formulare\" "
  "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
  "xmlns:date=\"http://exslt.org/dates-and-times\" "
  "xsi:schemaLocation=\"http://software602.cz/formulare "
  "FormularData_ZRDS11.xsd\" version=\"1.3o\">""\n"
"  <d:Identifikace>""\n"
"    <d:Formular>ZRDS5</d:Formular>""\n"
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
"    <d:CasOd>__repl_FROM_DATE__</d:CasOd>""\n"
"    <d:CasDo>__repl_TO_DATE__</d:CasDo>""\n"
"  </d:Parametry>""\n"
"</d:FormularData>";

#define FROM_KEY "fromDate"
#define TO_KEY "toDate"

class SzrRobVvuData {
	Q_DECLARE_TR_FUNCTIONS(SzrRobVvuData)

public:
	SzrRobVvuData(void)
	    : m_userFromDate(), m_userToDate()
	{ }

	QList<Gov::FormField> allFields(void) const;

private:
	QDate m_userFromDate; /*!< Start date. */
	QDate m_userToDate; /*!< Stop date. */
};

QList<Gov::FormField> SzrRobVvuData::allFields(void) const
{
	QList<Gov::FormField> formList;

	{
		Gov::FormField ff;
		ff.setKey(FROM_KEY);
		ff.setVal(m_userFromDate.isNull() ? QString() : m_userFromDate.toString(GOV_DATE_FORMAT));
		ff.setDescr(tr("From"));
		ff.setPlaceholder(tr("Select start date"));
		ff.setProperties(Gov::FormFieldType::PROP_MANDATORY |
		    Gov::FormFieldType::PROP_USER_INPUT |
		    Gov::FormFieldType::PROP_TYPE_DATE);
		formList.append(ff);
	}
	{
		Gov::FormField ff;
		ff.setKey(TO_KEY);
		ff.setVal(m_userToDate.isNull() ? QString() : m_userToDate.toString(GOV_DATE_FORMAT));
		ff.setDescr(tr("To"));
		ff.setPlaceholder(tr("Select end date"));
		ff.setProperties(Gov::FormFieldType::PROP_MANDATORY |
		    Gov::FormFieldType::PROP_USER_INPUT |
		    Gov::FormFieldType::PROP_TYPE_DATE);
		formList.append(ff);
	}

	return formList;
}

Gov::SrvcSzrRobVvu::SrvcSzrRobVvu(void)
   : Service()
{
	m_formFields = SzrRobVvuData().allFields();
}

Gov::Service *Gov::SrvcSzrRobVvu::createNew(void) const
{
	return new (std::nothrow) SrvcSzrRobVvu;
}

const QString &Gov::SrvcSzrRobVvu::internalId(void) const
{
	static const QString shortName("SrvcSzrRobVvu");
	return shortName;
}

const QString &Gov::SrvcSzrRobVvu::fullName(void) const
{
	static const QString fullName(tr("Printout about the usage of entries from the resident register"));
	// "Výpis o využití údajů z registru obyvatel"
	return fullName;
}

const QString &Gov::SrvcSzrRobVvu::instituteName(void) const
{
	static const QString instituteName(GOV_SZR_FULL_NAME);
	return instituteName;
}

const QString &Gov::SrvcSzrRobVvu::boxId(void) const
{
	static const QString boxId(GOV_SZR_DB_ID);
	return boxId;
}

const QString &Gov::SrvcSzrRobVvu::dmAnnotation(void) const
{
	static const QString dmAnnotation("Výpis o využití údajů z registru obyvatel");
	return dmAnnotation;
}

const QString &Gov::SrvcSzrRobVvu::dmSenderIdent(void) const
{
	static const QString dmSenderIdent; /* Null string. */
	return dmSenderIdent;
}

const QString &Gov::SrvcSzrRobVvu::dmFileDescr(void) const
{
	static const QString dmFileDescr(GOV_SZR_XML_FILE_NAME);
	return dmFileDescr;
}

bool Gov::SrvcSzrRobVvu::canSend(enum Isds::Type::DbType dbType) const
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

bool Gov::SrvcSzrRobVvu::setFieldVal(const QString &key, const QString &val)
{
	if ((key == FROM_KEY) || (key == TO_KEY)) {
		if (QDate::fromString(val, GOV_DATE_FORMAT).isNull()) {
			return false;
		}
	}

	return Service::setFieldVal(key, val);
}

bool Gov::SrvcSzrRobVvu::setOwnerInfoFields(const Isds::DbOwnerInfo &dbOwnerInfo)
{
	if (Q_UNLIKELY(dbOwnerInfo.isNull())) {
		return false;
	}

	return true;
}

bool Gov::SrvcSzrRobVvu::haveAllValidFields(QString *errDescr)
{
	bool ret = true;

	ret = Service::checkDate(FROM_KEY, errDescr);
	if (!ret) {
		return ret;
	}
	ret = Service::checkDate(TO_KEY, errDescr);
	if (!ret) {
		return ret;
	}

	const QDate fromDate(QDate::fromString(fieldVal(FROM_KEY), GOV_DATE_FORMAT));
	const QDate toDate(QDate::fromString(fieldVal(TO_KEY), GOV_DATE_FORMAT));
	if (Q_UNLIKELY(fromDate.isNull() || toDate.isNull())) {
		/* This branch shouldn't be taken as the checks before passed. */
		Q_ASSERT(0);
		return false;
	}

	if (fromDate > toDate) {
		if (errDescr != Q_NULLPTR) {
			*errDescr = tr("The date of start cannot be later than the date of end.");
		}
		return false;
	}

	if (errDescr != Q_NULLPTR) {
		errDescr->clear();
	}
	return true;
}

QByteArray Gov::SrvcSzrRobVvu::binaryXmlContent(void) const
{
	QString xml(xml_template);
	xml.replace("__repl_FROM_DATE__", fieldVal(FROM_KEY));
	xml.replace("__repl_TO_DATE__", fieldVal(TO_KEY));
	return xml.toUtf8();
}
