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
#include "src/datovka_shared/worker/pool.h"
#include "src/global.h"
#include "src/gov_services/gui/dlg_gov_service.h"
#include "src/gov_services/gui/dlg_gov_services.h"
#include "src/gui/datovka.h"
#include "src/io/account_db.h"
#include "src/io/isds_sessions.h"
#include "src/worker/task_keep_alive.h"
#include "ui_dlg_gov_services.h"

DlgGovServices::DlgGovServices(
    const QList<Task::AccountDescr> &messageDbSetList, const QString &userName,
    class MainWindow *mw, QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgGovServices),
    m_keepAliveTimer(this),
    m_messageDbSetList(messageDbSetList),
    m_userName(userName),
    m_isLoggedIn(false),
    m_dbSet(Q_NULLPTR),
    m_govServices(),
    m_govServiceListProxyModel(this),
    m_govServiceModel(this),
    m_mw(mw)
{
	m_ui->setupUi(this);

	/* Initialise e-gov services. */
	m_govServices = Gov::allServiceMap();

	m_govServiceListProxyModel.setSourceModel(&m_govServiceModel);
	m_ui->govServiceListView->setModel(&m_govServiceListProxyModel);
	m_ui->govServiceListView->setSelectionMode(QAbstractItemView::SingleSelection);
	m_ui->govServiceListView->setSelectionBehavior(QAbstractItemView::SelectItems);

	m_ui->filterLine->setToolTip(tr("Enter sought expression"));
	m_ui->filterLine->setClearButtonEnabled(true);

	QStandardItemModel *model = qobject_cast<QStandardItemModel *>(
	    m_ui->fromComboBox->model());
	if (Q_UNLIKELY(model == Q_NULLPTR)) {
		Q_ASSERT(0);
		return;
	}
	foreach (const Task::AccountDescr &acnt, m_messageDbSetList) {
		const AcntSettings &acntSettings((*GlobInstcs::acntMapPtr)[acnt.userName]);

		const QString accountName(acntSettings.accountName() +
		    QLatin1String(" (") + acnt.userName + QLatin1String(")"));
		m_ui->fromComboBox->addItem(accountName, QVariant(acnt.userName));
		int i = m_ui->fromComboBox->count() - 1;
		if (m_userName == acnt.userName) {
			m_ui->fromComboBox->setCurrentIndex(i);
			setAccountInfo(i);
		}

		if (acntSettings.isTestAccount()) {
			/* Disable testing environment accounts. */
			QStandardItem *item = model->item(i);
			if (Q_UNLIKELY(item == Q_NULLPTR)) {
				Q_ASSERT(0);
				continue;
			}
			item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
		}
	}

	/* Connect signal section. */
	connect(m_ui->fromComboBox, SIGNAL(currentIndexChanged(int)),
	    this, SLOT(setAccountInfo(int)));
	connect(m_ui->filterLine, SIGNAL(textChanged(QString)),
	    this, SLOT(filterServices(QString)));
	connect(m_ui->govServiceListView, SIGNAL(activated(QModelIndex)),
	    this, SLOT(onServiceActivated(QModelIndex)));

	m_keepAliveTimer.start(DLG_ISDS_KEEPALIVE_MS);

	connect(&m_keepAliveTimer, SIGNAL(timeout()), this, SLOT(pingIsdsServer()));
}

DlgGovServices::~DlgGovServices(void)
{
	Gov::clearServiceMap(m_govServices);
	delete m_ui;
}

void DlgGovServices::sendRequest(
    const QList<Task::AccountDescr> &messageDbSetList, const QString &userName,
    class MainWindow *mw, QWidget *parent)
{
	DlgGovServices dlg(messageDbSetList, userName, mw, parent);
	dlg.exec();
}

/*!
 * @brief Checks whether the related account has an active connection to ISDS.
 *
 * @param[in] keepAliveTimer Keep-alive timer.
 * @param[in] mw Pointer to main window.
 * @param[in] userName User name identifying the account.
 * @return True if active connection is present.
 */
static
bool isLoggedIn(QTimer &keepAliveTimer, MainWindow *const mw,
    const QString &userName)
{
	if (Q_UNLIKELY(userName.isEmpty())) {
		Q_ASSERT(0);
		return false;
	}

	bool loggedIn = false;
	keepAliveTimer.stop();
	{
		TaskKeepAlive *task =
		    new (std::nothrow) TaskKeepAlive(userName);
		if (Q_UNLIKELY(task == Q_NULLPTR)) {
			return false;
		}
		task->setAutoDelete(false);
		GlobInstcs::workPoolPtr->runSingle(task);

		loggedIn = task->m_isAlive;

		delete task;
	}
	if (!loggedIn) {
		if (Q_NULLPTR != mw) {
			loggedIn = mw->connectToIsds(userName);
		}
	}
	keepAliveTimer.start(DLG_ISDS_KEEPALIVE_MS);

	/* Check the presence of session. */
	if (!GlobInstcs::isdsSessionsPtr->holdsSession(userName)) {
		logErrorNL("%s", "Missing ISDS session.");
		loggedIn = false;
	}

	return loggedIn;
}

/*!
 * @brief Find database set relate to account.
 *
 * @param[in] acntDescrs Account descriptors.
 * @param[in] userName User name identifying the account.
 * @return Pointer related to specified account, Q_NULLPTR on error.
 */
static
MessageDbSet *getDbSet(const QList<Task::AccountDescr> &acntDescrs,
    const QString &userName)
{
	if (Q_UNLIKELY(userName.isEmpty())) {
		Q_ASSERT(0);
		return Q_NULLPTR;
	}

	MessageDbSet *dbSet = Q_NULLPTR;
	foreach (const Task::AccountDescr &acnt, acntDescrs) {
		if (acnt.userName == userName) {
			dbSet = acnt.messageDbSet;
			break;
		}
	}

	return dbSet;
}

void DlgGovServices::setAccountInfo(int fromComboIdx)
{
	debugSlotCall();

	/* Get user name for selected account. */
	const QString userName(
	    m_ui->fromComboBox->itemData(fromComboIdx).toString());
	if (Q_UNLIKELY(userName.isEmpty())) {
		Q_ASSERT(0);
		return;
	}

	m_userName = userName;
	loadServicesToModel();
	/* Show notification that this data box cannot send any requests. */
	m_ui->cannotSendLabel->setVisible(m_govServiceModel.rowCount() == 0);

	m_isLoggedIn = isLoggedIn(m_keepAliveTimer, m_mw, m_userName);

	m_dbSet = getDbSet(m_messageDbSetList, m_userName);
	if (Q_UNLIKELY(Q_NULLPTR == m_dbSet)) {
		Q_ASSERT(0);
		return;
	}
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

	/* Open service form dialogue. */
	DlgGovService::openGovServiceForm(m_userName, gs.take(), m_dbSet, this);
}

void DlgGovServices::pingIsdsServer(void) const
{
	if (Q_UNLIKELY(m_userName.isEmpty())) {
		Q_ASSERT(0);
		return;
	}

	TaskKeepAlive *task = new (std::nothrow) TaskKeepAlive(m_userName);
	if (Q_UNLIKELY(task == Q_NULLPTR)) {
		return;
	}
	task->setAutoDelete(true);
	GlobInstcs::workPoolPtr->assignHi(task);
}

void DlgGovServices::loadServicesToModel(void)
{
	debugFuncCall();

	m_govServiceModel.clearAll();

	const AcntSettings &acntSettings((*GlobInstcs::acntMapPtr)[m_userName]);
	if (Q_UNLIKELY(acntSettings.isTestAccount())) {
		/* Skip testing environment accounts. */
		Q_ASSERT(0);
		return;
	}

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
