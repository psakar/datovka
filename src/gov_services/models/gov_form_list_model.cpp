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

#include "src/gov_services/models/gov_form_list_model.h"

GovFormListModel::GovFormListModel(QObject *parent)
    : QAbstractListModel(parent),
    m_service(Q_NULLPTR)
{
}

GovFormListModel::GovFormListModel(const GovFormListModel &other,
    QObject *parent)
    : QAbstractListModel(parent),
    m_service(Q_NULLPTR)
{
	/* Copy service data. */
	if (other.m_service != Q_NULLPTR) {
		m_service = other.m_service->createNew();
		if (m_service != Q_NULLPTR) {
			foreach (const Gov::FormField &ff, other.m_service->fields()) {
				m_service->setFieldVal(ff.key(), ff.val());
			}
		}
	}
}

GovFormListModel::~GovFormListModel(void)
{
	if (m_service != Q_NULLPTR) {
		delete m_service; m_service = Q_NULLPTR;
	}
}

int GovFormListModel::rowCount(const QModelIndex &parent) const
{
	return ((!parent.isValid()) && (m_service != Q_NULLPTR)) ?
	    m_service->fields().size() : 0;
}

QVariant GovFormListModel::data(const QModelIndex &index, int role) const
{
	if (Q_UNLIKELY(m_service == Q_NULLPTR)) {
		Q_ASSERT(0);
		return QVariant();
	}

	int row = index.row();
	if (Q_UNLIKELY((row < 0) || (row >= m_service->fields().size()))) {
		Q_ASSERT(0);
		return QVariant();
	}

	const Gov::FormField &ff(m_service->fields()[row]);

	switch (role) {
	case Qt::DisplayRole:
		return ff.key();
		break;
	default:
		/* Do nothing. */
		break;
	}

	return QVariant();
}

const Gov::Service *GovFormListModel::service(void) const
{
	return m_service;
}

Gov::Service *GovFormListModel::setService(Gov::Service *service)
{
	Gov::Service *oldService = m_service;

	beginResetModel();
	m_service = service;
	endResetModel();

	return oldService;
}

void GovFormListModel::setKeyValue(const QString &key, const QString &value)
{
	beginResetModel();
	m_service->setFieldVal(key, value);
	endResetModel();
}

bool GovFormListModel::haveAllMandatory(void) const
{
	if (m_service != Q_NULLPTR) {
		return m_service->haveAllMandatoryFields();
	} else {
		return true;
	}
}

bool GovFormListModel::containsMandatoryUser(void) const
{
	if (m_service != Q_NULLPTR) {
		return m_service->containsMandatoryUserFields();
	} else {
		return false;
	}
}

bool GovFormListModel::containsBoxOwnerData(void) const
{
	if (m_service != Q_NULLPTR) {
		return m_service->containsBoxOwnerDataFields();
	} else {
		return false;
	}
}

bool GovFormListModel::haveAllValid(void)
{
	if (m_service != Q_NULLPTR) {
		bool ret = false;
		QString errDescr;
		beginResetModel();
		ret = m_service->haveAllValidFields(&errDescr);
		endResetModel();
		emit validityNotification(errDescr);
		return ret;
	} else {
		return true;
	}
}
