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

#include "src/datovka_shared/gov_services/service/gov_mv_crr_vbh.h"

static const char xml_template[] =
"<?xml version='1.0' encoding='UTF-8'?>""\n"
"<d:root xmlns:d=\"http://software602.cz/sample\" ancestor_id=\"\" "
  "folder_id=\"\" formdata_id=\"\" fsuser_id=\"\" institute_type=\"\" "
  "ldapPass=\"\" nazev=\"\" page=\"0\" page_id=\"\" query_seq=\"2\" "
  "register=\"258\" retry=\"0\" seq=\"\" templateVersion_id=\"\" "
  "url=\"\" url_release=\"\" user_name=\"\" version=\"9.7\" "
  "xml:lang=\"cs\">""\n"
"  <d:crr>""\n"
"    <d:crrVypisBody>""\n"
"      <d:zadost>""\n"
"        <d:uid/>""\n"
"        <d:autor/>""\n"
"        <d:duvod/>""\n"
"        <d:osobaRC/>""\n"
"        <d:cisloRP_prevod/>""\n"
"        <d:prijmeni>__repl_SURNAME__</d:prijmeni>""\n"
"        <d:jmeno>__repl_NAME__</d:jmeno>""\n"
"        <d:rodnePrijmeni/>""\n"
"        <d:datumNarozeni>__repl_BIRTH_DATE__</d:datumNarozeni>""\n"
"        <d:datumNarozeni_prevod/>""\n"
"        <d:cisloRP>__repl_DRIVE_LICENSE_ID__</d:cisloRP>""\n"
"        <d:vystup>PDF</d:vystup>""\n"
"      </d:zadost>""\n"
"    </d:crrVypisBody>""\n"
"    <d:x509Certificate KeyName=\"\"/>""\n"
"    <d:vydanyDokument url=\"\">""\n"
"      <d:pdf/>""\n"
"    </d:vydanyDokument>""\n"
"  </d:crr>""\n"
"</d:root>";

#define DRILIC_KEY "drivingLicenceId"
#define NAME_KEY "name"
#define SURNAME_KEY "surname"
#define BIRTH_KEY "birthDate"

class MvCrrVbhData {
	Q_DECLARE_TR_FUNCTIONS(MvCrrVbhData)

public:
	MvCrrVbhData(void)
	    : m_userDrivingLicenceId(), m_boxName(), m_boxSurname(),
	    m_boxBirthDate()
	{ }

	QList<Gov::FormField> allFields(void) const;

private:
	QString m_userDrivingLicenceId; /*!< Driving licence identifier (number) without white spaces. */
	QString m_boxName; /*!< First name derived from box data. */
	QString m_boxSurname; /*!< Seurname derived from box data. */
	QDate m_boxBirthDate; /*!< Birth date derived from box data. */
};

QList<Gov::FormField> MvCrrVbhData::allFields(void) const
{
	QList<Gov::FormField> formList;

	{
		Gov::FormField ff;
		ff.setKey(DRILIC_KEY);
		ff.setVal(m_userDrivingLicenceId);
		ff.setDescr(tr("Driving licence ID"));
		ff.setPlaceholder(tr("Enter driving licence ID without spaces"));
		ff.setProperties(Gov::FormFieldType::PROP_MANDATORY |
		    Gov::FormFieldType::PROP_USER_INPUT);
		formList.append(ff);
	}

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
		    Gov::FormFieldType::PROP_BOX_INPUT |
		    Gov::FormFieldType::PROP_TYPE_DATE);
		formList.append(ff);
	}

	return formList;
}

Gov::SrvcMvCrrVbh::SrvcMvCrrVbh(void)
    : Service()
{
	m_formFields = MvCrrVbhData().allFields();
}

Gov::Service *Gov::SrvcMvCrrVbh::createNew(void) const
{
	return new (std::nothrow) SrvcMvCrrVbh;
}

const QString &Gov::SrvcMvCrrVbh::internalId(void) const
{
	static const QString shortName("SrvcMvCrrVbh");
	return shortName;
}

const QString &Gov::SrvcMvCrrVbh::fullName(void) const
{
	static const QString fullName(tr("Printout from the driver penalty point system"));
	// "Výpis bodového hodnocení z Centrálního registru řidičů"
	return fullName;
}

const QString &Gov::SrvcMvCrrVbh::instituteName(void) const
{
	static const QString instituteName(GOV_MV_FULL_NAME);
	return instituteName;
}

const QString &Gov::SrvcMvCrrVbh::boxId(void) const
{
	static const QString boxId(GOV_MV_DB_ID);
	return boxId;
}

const QString &Gov::SrvcMvCrrVbh::dmAnnotation(void) const
{
	static const QString dmAnnotation("CzechPOINT@home - Výpis z Centrálního registru řidičů");
	return dmAnnotation;
}

const QString &Gov::SrvcMvCrrVbh::dmSenderIdent(void) const
{
	static const QString dmSenderIdent("CzechPOINT@home - 258");
	return dmSenderIdent;
}

const QString &Gov::SrvcMvCrrVbh::dmFileDescr(void) const
{
	static const QString dmFileDescr(GOV_MV_XML_FILE_NAME);
	return dmFileDescr;
}

bool Gov::SrvcMvCrrVbh::canSend(enum Isds::Type::DbType dbType) const
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

bool Gov::SrvcMvCrrVbh::setFieldVal(const QString &key, const QString &val)
{
	if (key == BIRTH_KEY) {
		if (QDate::fromString(val, GOV_DATE_FORMAT).isNull()) {
			return false;
		}
	}

	return Service::setFieldVal(key, val);
}

bool Gov::SrvcMvCrrVbh::setOwnerInfoFields(const Isds::DbOwnerInfo &dbOwnerInfo)
{
	if (Q_UNLIKELY(dbOwnerInfo.isNull())) {
		return false;
	}

	const QString &boxName(dbOwnerInfo.personName().firstName());
	const QString &boxSurname(dbOwnerInfo.personName().lastName());
	const QDate &boxBirthDate(dbOwnerInfo.birthInfo().date());
	if (Q_UNLIKELY(boxName.isEmpty() || boxSurname.isEmpty() ||
	        boxBirthDate.isNull())) {
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

	return true;
}

bool Gov::SrvcMvCrrVbh::haveAllValidFields(QString *errDescr)
{
		bool ret = false;

	ret = Service::checkStrRemoveWhiteSpace(DRILIC_KEY, errDescr);
	if (!ret) {
		return ret;
	}
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

	if (errDescr != Q_NULLPTR) {
		errDescr->clear();
	}
	return true;
}

QByteArray Gov::SrvcMvCrrVbh::binaryXmlContent(void) const
{
	QString xml(xml_template);
	xml.replace("__repl_DRIVE_LICENSE_ID__", fieldVal(DRILIC_KEY));
	xml.replace("__repl_NAME__", fieldVal(NAME_KEY));
	xml.replace("__repl_SURNAME__", fieldVal(SURNAME_KEY));
	xml.replace("__repl_BIRTH_DATE__", fieldVal(BIRTH_KEY));
	return xml.toUtf8();
}
