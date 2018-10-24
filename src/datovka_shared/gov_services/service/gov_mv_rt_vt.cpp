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

#include "src/datovka_shared/gov_services/service/gov_mv_rt_vt.h"

static const char xml_template[] =
"<?xml version='1.0' encoding='utf-8'?>"
"<d:root xmlns:d=\"http://software602.cz/sample\" "
  "xmlns:date=\"http://exslt.org/dates-and-times\" "
  "xmlns:rs=\"http://portal.gov.cz/rejstriky/ISRS/1.2/\" ancestor_id=\"\" "
  "folder_id=\"\" formdata_id=\"\" fsuser_id=\"\" institute_type=\"\" "
  "ldapPass=\"\" nazev=\"\" page=\"0\" page_id=\"\" query_seq=\"1\" "
  "register=\"262\" retry=\"0\" seq=\"\" templateVersion_id=\"\" url=\"\" "
  "url_release=\"\" user_name=\"\" version=\"1.4o\" xml:lang=\"cs\">""\n"
"  <d:gw_sn/>""\n"
"  <d:gw_form_name/>""\n"
"  <d:stav_form/>""\n"
"  <d:stav_rizeni>""\n"
"    <d:vydano>0</d:vydano>""\n"
"  </d:stav_rizeni>""\n"
"  <d:requestZadostVM>""\n"
"    <zvm:czechPointUrl xmlns:zvm=\"http://fo.rt.cleverlance.com/zadostVM_2.0\"/>""\n"
"    <zvm:vydejniMisto xmlns:zvm=\"http://fo.rt.cleverlance.com/zadostVM_2.0\"/>""\n"
"    <zvm:X509CertificateVM xmlns:zvm=\"http://fo.rt.cleverlance.com/zadostVM_2.0\" KeyName=\"\"/>""\n"
"    <zvm:zadostVM xmlns:zvm=\"http://fo.rt.cleverlance.com/zadostVM_2.0\" identZadost=\"\" vystupDokument=\"V\">""\n"
"      <zvm:ucel zkratka=\"BEZUH\"/>""\n"
"      <zvm:mail/>""\n"
"      <zvm:osoba rozlisovatJmPr=\"true\">""\n"
"        <com:jmeno xmlns:com=\"http://fo.rt.cleverlance.com/commons_2.0\">__repl_NAME__</com:jmeno>""\n"
"        <com:prijmeniRodne xmlns:com=\"http://fo.rt.cleverlance.com/commons_2.0\"/>""\n"
"        <com:prijmeniNynejsi xmlns:com=\"http://fo.rt.cleverlance.com/commons_2.0\">__repl_SURNAME__</com:prijmeniNynejsi>""\n"
"        <com:datumNarozeni xmlns:com=\"http://fo.rt.cleverlance.com/commons_2.0\">""\n"
"          <com:den>__repl_BIRTH_DATE_DAY__</com:den>""\n"
"          <com:mesic>__repl_BIRTH_DATE_MONTH__</com:mesic>""\n"
"          <com:rok>__repl_BIRTH_DATE_YEAR__</com:rok>""\n"
"        </com:datumNarozeni>""\n"
"        <com:pohlavi xmlns:com=\"http://fo.rt.cleverlance.com/commons_2.0\" zkratka=\"\"/>""\n"
"        <com:rodneCislo xmlns:com=\"http://fo.rt.cleverlance.com/commons_2.0\"/>""\n"
"        <com:mistoNarozeni xmlns:com=\"http://fo.rt.cleverlance.com/commons_2.0\">""\n"
"          <com:stat zkratka=\"\">CZ</com:stat>""\n"
"          <com:okres>__repl_REGION__</com:okres>""\n"
"          <com:obec>__repl_CITY__</com:obec>""\n"
"        </com:mistoNarozeni>""\n"
"        <zvm:statniPrislusnost zkratka=\"\"/>""\n"
"        <zvm:bydlisteStPrislusnostMin zkratka=\"\"/>""\n"
"      </zvm:osoba>""\n"
"    </zvm:zadostVM>""\n"
"  </d:requestZadostVM>""\n"
"</d:root>";

#define NAME_KEY "name"
#define SURNAME_KEY "surname"
#define BIRTH_KEY "birthDate"
#define REGION_KEY "region"
#define CITY_KEY "city"

class MvRtVtData {
	Q_DECLARE_TR_FUNCTIONS(MvRtVtData)

public:
	MvRtVtData(void)
	    : m_boxName(), m_boxSurname(), m_boxBirthDate(), m_boxBirthRegion(),
	    m_boxBirthCity()
	{ }

	QList<Gov::FormField> allFields(void) const;

private:
	QString m_boxName; /*!< First name derived from box data. */
	QString m_boxSurname; /*!< Seurname derived from box data. */
	QDate m_boxBirthDate; /*!< Birth date derived from box data. */
	QString m_boxBirthRegion; /*!< Birth region. */
	QString m_boxBirthCity; /*!< Birth city. */
};

QList<Gov::FormField> MvRtVtData::allFields(void) const
{
	QList<Gov::FormField> formList;

	{
		Gov::FormField ff;
		ff.setKey(NAME_KEY);
		ff.setVal(m_boxName);
		ff.setDescr(tr("Data-box owner name"));
		//ff.setPlaceholder(QString());
		ff.setProperties(Gov::FormFieldType::PROP_MANDATORY |
		    Gov::FormFieldType::PROP_BOX_INPUT);
		formList.append(ff);
	}
	{
		Gov::FormField ff;
		ff.setKey(SURNAME_KEY);
		ff.setVal(m_boxSurname);
		ff.setDescr(tr("Data-box owner surname"));
		//ff.setPlaceholder(QString());
		ff.setProperties(Gov::FormFieldType::PROP_MANDATORY |
		    Gov::FormFieldType::PROP_BOX_INPUT);
		formList.append(ff);
	}
	{
		Gov::FormField ff;
		ff.setKey(BIRTH_KEY);
		ff.setVal(m_boxBirthDate.isNull() ? QString() : m_boxBirthDate.toString(GOV_DATE_FORMAT));
		ff.setDescr(tr("Birth date"));
		//ff.setPlaceholder(QString());
		ff.setProperties(Gov::FormFieldType::PROP_MANDATORY |
		    Gov::FormFieldType::PROP_BOX_INPUT);
		formList.append(ff);
	}
	{
		Gov::FormField ff;
		ff.setKey(REGION_KEY);
		ff.setVal(m_boxBirthRegion);
		ff.setDescr(tr("Birth region"));
		//ff.setPlaceholder(QString());
		ff.setProperties(Gov::FormFieldType::PROP_MANDATORY |
		    Gov::FormFieldType::PROP_BOX_INPUT);
		formList.append(ff);
	}
	{
		Gov::FormField ff;
		ff.setKey(CITY_KEY);
		ff.setVal(m_boxBirthCity);
		ff.setDescr(tr("City"));
		//ff.setPlaceholder(QString());
		ff.setProperties(Gov::FormFieldType::PROP_MANDATORY |
		    Gov::FormFieldType::PROP_BOX_INPUT);
		formList.append(ff);
	}

	return formList;
}

Gov::SrvcMvRtVt::SrvcMvRtVt(void)
    : Service()
{
	m_formFields = MvRtVtData().allFields();
}

Gov::Service *Gov::SrvcMvRtVt::createNew(void) const
{
	return new (std::nothrow) SrvcMvRtVt;
}

const QString &Gov::SrvcMvRtVt::internalId(void) const
{
	static const QString shortName("SrvcMvRtVt");
	return shortName;
}

const QString &Gov::SrvcMvRtVt::fullName(void) const
{
	static const QString fullName(tr("Printout from the criminal records"));
	// "Výpis z Rejstříku trestů"
	return fullName;
}

const QString &Gov::SrvcMvRtVt::instituteName(void) const
{
	static const QString instituteName(GOV_MV_FULL_NAME);
	return instituteName;
}

const QString &Gov::SrvcMvRtVt::boxId(void) const
{
	static const QString boxId(GOV_MV_DB_ID);
	return boxId;
}

const QString &Gov::SrvcMvRtVt::dmAnnotation(void) const
{
	static const QString dmAnnotation("CzechPOINT@home - Žádost o výpis z Rejstříku trestů");
	return dmAnnotation;
}

const QString &Gov::SrvcMvRtVt::dmSenderIdent(void) const
{
	static const QString dmSenderIdent("CzechPOINT@home - 262");
	return dmSenderIdent;
}

const QString &Gov::SrvcMvRtVt::dmFileDescr(void) const
{
	static const QString dmFileDescr(GOV_MV_XML_FILE_NAME);
	return dmFileDescr;
}

bool Gov::SrvcMvRtVt::canSend(enum Isds::Type::DbType dbType) const
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

bool Gov::SrvcMvRtVt::setFieldVal(const QString &key, const QString &val)
{
	if (key == BIRTH_KEY) {
		if (QDate::fromString(val, GOV_DATE_FORMAT).isNull()) {
			return false;
		}
	}

	return Service::setFieldVal(key, val);
}

bool Gov::SrvcMvRtVt::setOwnerInfoFields(const Isds::DbOwnerInfo &dbOwnerInfo)
{
	if (Q_UNLIKELY(dbOwnerInfo.isNull())) {
		return false;
	}

	const QString &boxName(dbOwnerInfo.personName().firstName());
	const QString &boxSurname(dbOwnerInfo.personName().lastName());
	const QDate &boxBirthDate(dbOwnerInfo.birthInfo().date());
	const QString &boxBirthRegion(dbOwnerInfo.birthInfo().county());
	const QString &boxBirthCity(dbOwnerInfo.birthInfo().city());

	if (Q_UNLIKELY(boxName.isEmpty() || boxSurname.isEmpty() ||
	        boxBirthDate.isNull() || boxBirthRegion.isEmpty() ||
	        boxBirthCity.isEmpty())) {
		return false;
	}

	if (Q_UNLIKELY(!Service::setFieldVal(NAME_KEY, boxName))) {
		Q_ASSERT(0);
		return false;
	}
	if (Q_UNLIKELY(!Service::setFieldVal(SURNAME_KEY, boxSurname))) {
		Q_ASSERT(0);
		return false;
	}
	if (Q_UNLIKELY(!Service::setFieldVal(BIRTH_KEY, boxBirthDate.toString(GOV_DATE_FORMAT)))) {
		Q_ASSERT(0);
		return false;
	}
	if (Q_UNLIKELY(!Service::setFieldVal(REGION_KEY, boxBirthRegion))) {
		Q_ASSERT(0);
		return false;
	}
	if (Q_UNLIKELY(!Service::setFieldVal(CITY_KEY, boxBirthCity))) {
		Q_ASSERT(0);
		return false;
	}

	return true;
}

bool Gov::SrvcMvRtVt::haveAllValidFields(QString *errDescr)
{
	bool ret = false;

	ret = Service::checkStrTrimmed(NAME_KEY, errDescr);
	if (!ret) {
		return ret;
	}
	ret = Service::checkStrTrimmed(SURNAME_KEY, errDescr);
	if (!ret) {
		return ret;
	}
	ret = Service::checkDate(BIRTH_KEY, errDescr);
	if (!ret) {
		return ret;
	}
	ret = Service::checkStrTrimmed(REGION_KEY, errDescr);
	if (!ret) {
		return ret;
	}
	ret = Service::checkStrTrimmed(CITY_KEY, errDescr);
	if (!ret) {
		return ret;
	}

	if (errDescr != Q_NULLPTR) {
		errDescr->clear();
	}
	return true;
}

QByteArray Gov::SrvcMvRtVt::binaryXmlContent(void) const
{
	const QDate boxBirthDate(
	    QDate::fromString(fieldVal(BIRTH_KEY), GOV_DATE_FORMAT));

	QString xml(xml_template);
	xml.replace("__repl_NAME__", fieldVal(NAME_KEY));
	xml.replace("__repl_SURNAME__", fieldVal(SURNAME_KEY));
	xml.replace("__repl_BIRTH_DATE_DAY__", boxBirthDate.toString("d"));
	xml.replace("__repl_BIRTH_DATE_MONTH__", boxBirthDate.toString("M"));
	xml.replace("__repl_BIRTH_DATE_YEAR__", boxBirthDate.toString("yyyy"));
	xml.replace("__repl_REGION__", fieldVal(REGION_KEY));
	xml.replace("__repl_CITY__", fieldVal(CITY_KEY));
	return xml.toUtf8();
}
