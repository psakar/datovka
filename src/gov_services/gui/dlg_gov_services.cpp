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

#include "src/datovka_shared/gov_services/service/gov_service.h"
#include "src/datovka_shared/gov_services/service/gov_services_all.h"
#include "src/datovka_shared/log/log.h"
#include "src/datovka_shared/log/memory_log.h"
#include "src/global.h"
#include "src/gov_services/gui/dlg_gov_service.h"
#include "src/gov_services/gui/dlg_gov_services.h"
#include "src/io/account_db.h"
#include "ui_dlg_gov_services.h"

DlgGovServices::DlgGovServices(const QString &userName, MessageDbSet *dbSet,
    QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgGovServices),
    m_userName(userName),
    m_dbSet(dbSet),
    m_govServices(),
    m_govServiceModel()
{
	m_ui->setupUi(this);

	/* Init e-gov services. */
	m_govServices = Gov::allServiceMap();

	/* Load and set e-gov model into list view. */
	loadServicesToModel();

	/* Show notification that this data box cannot send any requests. */
	m_ui->label->setVisible(m_govServiceModel.rowCount() != 0);
	m_ui->cannotSendLabel->setVisible(m_govServiceModel.rowCount() == 0);

	m_govServiceListProxyModel.setSourceModel(&m_govServiceModel);
	m_ui->govServiceListView->setModel(&m_govServiceListProxyModel);
	m_ui->govServiceListView->setSelectionMode(QAbstractItemView::SingleSelection);
	m_ui->govServiceListView->setSelectionBehavior(QAbstractItemView::SelectItems);

	m_ui->filterLine->setToolTip(tr("Enter sought expression"));
	m_ui->filterLine->setClearButtonEnabled(true);

	/* Connect signal section. */
	connect(m_ui->filterLine, SIGNAL(textChanged(QString)),
	    this, SLOT(filterServices(QString)));
	connect(m_ui->govServiceListView, SIGNAL(activated(QModelIndex)),
	    this, SLOT(onServiceActivated(QModelIndex)));
}

DlgGovServices::~DlgGovServices(void)
{
	Gov::clearServiceMap(m_govServices);
	delete m_ui;
}

void DlgGovServices::showGovServices(const QString &userName,
    MessageDbSet *dbSet, QWidget *parent)
{
	DlgGovServices *govServicesDialog =
	    new DlgGovServices(userName, dbSet, parent);
	govServicesDialog->exec();
}

void DlgGovServices::filterServices(const QString &text)
{
	m_govServiceListProxyModel.setFilterRegExp(QRegExp(text,
	    Qt::CaseInsensitive, QRegExp::FixedString));
	/* Set filter field background colour. */
	if (text.isEmpty()) {
		m_ui->filterLine->setStyleSheet(
		    SortFilterProxyModel::blankFilterEditStyle);
	} else if (m_govServiceListProxyModel.rowCount() != 0) {
		m_ui->filterLine->setStyleSheet(
		    SortFilterProxyModel::foundFilterEditStyle);
	} else {
		m_ui->filterLine->setStyleSheet(
		    SortFilterProxyModel::notFoundFilterEditStyle);
	}
}

void DlgGovServices::onServiceActivated(const QModelIndex &index)
{
	debugSlotCall();

	if (!index.isValid()) {
		return;
	}

	/* Get service id from index. */
	const QString serId(index.sibling(index.row(), 0)
	    .data(GovServiceListModel::ROLE_INTERN_ID).toString());
	if (Q_UNLIKELY(serId.isEmpty())) {
		return;
	}

	/* Get current e-gov service. */
	const Gov::Service *cgs = m_govServices.value(serId, Q_NULLPTR);
	if (Q_UNLIKELY(cgs == Q_NULLPTR)) {
		logErrorNL("Cannot access e-gov service '%s'.",
		    serId.toUtf8().constData());
		Q_ASSERT(0);
		return;
	}

	/* Create new copy of service. */
	QScopedPointer<Gov::Service> gs(cgs->createNew());
	if (Q_UNLIKELY(gs.isNull())) {
		Q_ASSERT(0);
		return;
	}

	/* Fill some form items from account info. */
	const Isds::DbOwnerInfo dbOwnerInfo(GlobInstcs::accntDbPtr->getOwnerInfo(
	    AccountDb::keyFromLogin(m_userName)));
	if (dbOwnerInfo.isNull()) {
		return;
	}
	gs->setOwnerInfoFields(dbOwnerInfo);

	/* Open service form dialog. */
	DlgGovService::openGovServiceForm(m_userName, gs.take(), m_dbSet, this);
}

void DlgGovServices::loadServicesToModel(void)
{
	debugFuncCall();

	m_govServiceModel.clearAll();

	/* Get account info. */
	const Isds::DbOwnerInfo dbOwnerInfo(GlobInstcs::accntDbPtr->getOwnerInfo(
	    AccountDb::keyFromLogin(m_userName)));
	if (Q_UNLIKELY(dbOwnerInfo.isNull())) {
		return;
	}

	foreach (const QString &key, m_govServices.keys()) {
		const Gov::Service *cgs = m_govServices.value(key);
		if (Q_UNLIKELY(cgs == Q_NULLPTR)) {
			Q_ASSERT(0);
			continue;
		}
		/* Enlist only services which can be used. */
		if (cgs->canSend(dbOwnerInfo.dbType())) {
			m_govServiceModel.appendService(cgs);
		} else {
			logInfo("User '%s' cannot use the e-gov service '%s'.",
			    m_userName.toUtf8().constData(),
			    cgs->internalId().toUtf8().constData());
		}
	}
}
