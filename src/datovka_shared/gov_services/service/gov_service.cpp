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

#include "src/datovka_shared/gov_services/helper.h"
#include "src/datovka_shared/gov_services/service/gov_service.h"

Gov::Service::Service(void)
    : m_formFields()
{
}

Gov::Service::~Service(void)
{
}

const QList<Gov::FormField> &Gov::Service::fields(void) const
{
	return m_formFields;
}

QList<Gov::FormField> &Gov::Service::fields(void)
{
	return m_formFields;
}

QString Gov::Service::fieldVal(const QString &key, bool *ok) const
{
	foreach (const FormField &ff, m_formFields) {
		if (key == ff.key()) {
			if (ok != Q_NULLPTR) {
				*ok = true;
			}
			return ff.val();
		}
	}

	if (ok != Q_NULLPTR) {
		*ok = false;
	}
	return QString();
}

bool Gov::Service::setFieldVal(const QString &key, const QString &val)
{
	for (int i = 0; i < m_formFields.size(); ++i) {
		FormField &ff(m_formFields[i]);
		if (key == ff.key()) {
			ff.setVal(val);
			return true;
		}
	}

	return false;
}

bool Gov::Service::haveAllMandatoryFields(void) const
{
	foreach (const FormField &ff, m_formFields) {
		if ((ff.properties() & FormFieldType::PROP_MANDATORY) &&
		    ff.val().isEmpty()) {
			return false;
		}
	}

	return true;
}

bool Gov::Service::containsMandatoryUserFields(void) const
{
	foreach (const FormField &ff, m_formFields) {
		const FormFieldType::Properties props = ff.properties();
		if ((props & FormFieldType::PROP_MANDATORY) &&
		    (props & FormFieldType::PROP_USER_INPUT)) {
			return true;
		}
	}

	return false;
}

bool Gov::Service::containsBoxOwnerDataFields(void) const
{
	foreach (const FormField &ff, m_formFields) {
		if (ff.properties() & FormFieldType::PROP_BOX_INPUT) {
			return true;
		}
	}

	return false;
}

Isds::Message Gov::Service::dataMessage(void) const
{
	Isds::Message message;

	{
		Isds::Envelope envelope;
		/* Fill message envelope */
		envelope.setDmAnnotation(dmAnnotation());
		envelope.setDbIDRecipient(boxId());
		envelope.setDmRecipient(instituteName());
		envelope.setDmSenderIdent(dmSenderIdent());
		envelope.setDmPublishOwnID(Isds::Type::BOOL_FALSE);
		envelope.setDmAllowSubstDelivery(Isds::Type::BOOL_FALSE);
		envelope.setDmPersonalDelivery(Isds::Type::BOOL_FALSE);
		envelope.setDmMessageStatus(Isds::Type::MS_POSTED);
		/* Attachment size of GOV xml is < 1KB */
		envelope.setDmAttachmentSize(1);
		message.setEnvelope(envelope);
	}

	{
		Isds::Document document;
		QList<Isds::Document> documents;
		/* Load xml content as attachment file */
		document.setFileDescr(dmFileDescr());
		document.setFileMetaType(Isds::Type::FMT_MAIN);
		document.setMimeType(QStringLiteral("application/xml")); /* FIXME -- Properly detect mime type. */
		document.setBinaryContent(binaryXmlContent());
		documents.append(document);
		message.setDocuments(documents);
	}

	return message;
}

int Gov::Service::fieldIndex(const QString &key) const
{
	for (int i = 0; i < m_formFields.size(); ++i) {
		const FormField &ff(m_formFields[i]);
		if (key == ff.key()) {
			return i;
		}
	}

	return -1;
}

bool Gov::Service::checkDate(const QString &key, QString *errDescr)
{
	int idx = -1;

	idx = fieldIndex(key);
	if (Q_UNLIKELY(idx < 0)) {
		Q_ASSERT(0);
		if (errDescr != Q_NULLPTR) {
			*errDescr = tr("Cannot access IC field.");
		}
		return false;
	}
	FormField &ff(m_formFields[idx]);
	ff.setVal(Helper::withoutWhiteSpaces(ff.val()));
	if (QDate::fromString(ff.val(), GOV_DATE_FORMAT).isNull()) {
		if (errDescr != Q_NULLPTR) {
			*errDescr =
			    tr("The field '%1' contains an invalid date '%2'.")
			        .arg(ff.descr()).arg(ff.val());
		}
		return false;
	}

	if (errDescr != Q_NULLPTR) {
		errDescr->clear();
	}
	return true;
}

bool Gov::Service::checkIc(const QString &key, QString *errDescr)
{
	int idx = -1;

	idx = fieldIndex(key);
	if (Q_UNLIKELY(idx < 0)) {
		Q_ASSERT(0);
		if (errDescr != Q_NULLPTR) {
			*errDescr = tr("Cannot access IC field.");
		}
		return false;
	}
	FormField &ff(m_formFields[idx]);
	ff.setVal(Helper::withoutWhiteSpaces(ff.val()));
	if (!Helper::isValidIcStr(ff.val())) {
		if (errDescr != Q_NULLPTR) {
			*errDescr =
			    tr("The field '%1' contains an invalid value '%2'.")
			        .arg(ff.descr()).arg(ff.val());
		}
		return false;
	}

	if (errDescr != Q_NULLPTR) {
		errDescr->clear();
	}
	return true;
}

bool Gov::Service::checkStrRemoveWhiteSpace(const QString &key, QString *errDescr)
{
	int idx = -1;

	idx = fieldIndex(key);
	if (Q_UNLIKELY(idx < 0)) {
		Q_ASSERT(0);
		if (errDescr != Q_NULLPTR) {
			*errDescr = tr("Cannot access IC field.");
		}
		return false;
	}
	FormField &ff(m_formFields[idx]);
	ff.setVal(Helper::withoutWhiteSpaces(ff.val()));
	if (ff.val().isEmpty()) {
		if (errDescr != Q_NULLPTR) {
			*errDescr = tr("The field '%1' contains no value.")
			    .arg(ff.descr());
		}
		return false;
	}

	if (errDescr != Q_NULLPTR) {
		errDescr->clear();
	}
	return true;
}

bool Gov::Service::checkStrTrimmed(const QString &key, QString *errDescr)
{
	int idx = -1;

	idx = fieldIndex(key);
	if (Q_UNLIKELY(idx < 0)) {
		Q_ASSERT(0);
		if (errDescr != Q_NULLPTR) {
			*errDescr = tr("Cannot access IC field.");
		}
		return false;
	}
	FormField &ff(m_formFields[idx]);
	ff.setVal(ff.val().trimmed());
	if (ff.val().isEmpty()) {
		if (errDescr != Q_NULLPTR) {
			*errDescr = tr("The field '%1' contains no value.")
			    .arg(ff.descr());
		}
		return false;
	}

	if (errDescr != Q_NULLPTR) {
		errDescr->clear();
	}
	return true;
}
