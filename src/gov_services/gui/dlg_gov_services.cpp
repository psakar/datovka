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

#include "src/datovka_shared/gov_services/service/gov_mv_crr_vbh.h"
#include "src/datovka_shared/gov_services/service/gov_mv_ir_vp.h"
#include "src/datovka_shared/gov_services/service/gov_mv_rt_vt.h"
#include "src/datovka_shared/gov_services/service/gov_mv_rtpo_vt.h"
#include "src/datovka_shared/gov_services/service/gov_mv_skd_vp.h"
#include "src/datovka_shared/gov_services/service/gov_mv_vr_vp.h"
#include "src/datovka_shared/gov_services/service/gov_mv_zr_vp.h"
#include "src/datovka_shared/gov_services/service/gov_service.h"
#include "src/datovka_shared/gov_services/service/gov_szr_rob_vu.h"
#include "src/datovka_shared/gov_services/service/gov_szr_rob_vvu.h"
#include "src/datovka_shared/gov_services/service/gov_szr_ros_vv.h"
#include "src/datovka_shared/log/log.h"
#include "src/datovka_shared/log/memory_log.h"
#include "src/gov_services/models/gov_form_list_model.h"
#include "src/gov_services/gui/dlg_gov_services.h"
#include "ui_dlg_gov_services.h"

DlgGovServices::DlgGovServices(QWidget *parent)
    : QDialog(parent),
    m_govServices(),
    m_govServiceModel(new (std::nothrow) GovServiceListModel),
    m_ui(new (std::nothrow) Ui::DlgGovServices)
{
	m_ui->setupUi(this);

	initGovServices();

	m_ui->govServiceListView->setModel(m_govServiceModel);

	loadServicesToModel();
}

DlgGovServices::~DlgGovServices(void)
{
	clearGovServices();
	delete m_ui;
}

/*!
 * @brief Add a newly allocated service into the service container.
 *
 * @note The service object is freed when it cannot be added into the container.
 *
 * @param[in] map Service container.
 * @param[in] gs Newly allocated service object.
 */
static
void insertService(QMap<QString, const Gov::Service *> &map,
    Gov::Service *gs)
{
	if (Q_UNLIKELY(gs == Q_NULLPTR)) {
		return;
	}

	const QString &key(gs->internalId());
	if (!map.contains(key)) {
		map.insert(key, gs);
	} else {
		logError("Key '%s' already exists in gov service container.",
		    key.toUtf8().constData());
		delete gs;
	}
}

void DlgGovServices::initGovServices(void)
{
	debugFuncCall();

	clearGovServices();

	/* Výpis bodového hodnocení z Centrálního registru řidičů */
	insertService(m_govServices, new (std::nothrow) Gov::SrvcMvCrrVbh);

	/* Výpis z insolvenčního rejstříku */
	insertService(m_govServices, new (std::nothrow) Gov::SrvcMvIrVp);

	/* Výpis z Rejstříku trestů */
	insertService(m_govServices, new (std::nothrow) Gov::SrvcMvRtVt);

	/* Výpis z Rejstříku trestů právnických osob */
	insertService(m_govServices, new (std::nothrow) Gov::SrvcMvRtpoVt);

	/* Výpis ze seznamu kvalifikovaných dodavatelů */
	insertService(m_govServices, new (std::nothrow) Gov::SrvcMvSkdVp);

	/* Výpis z veřejného rejstříku */
	insertService(m_govServices, new (std::nothrow) Gov::SrvcMvVrVp);

	/* Výpis z živnostenského rejstříku */
	insertService(m_govServices, new (std::nothrow) Gov::SrvcMvZrVp);

	/* Výpis z Registru obyvatel */
	insertService(m_govServices, new (std::nothrow) Gov::SrvcSzrRobVu);

	/* Výpis o využití údajů z registru obyvatel */
	insertService(m_govServices, new (std::nothrow) Gov::SrvcSzrRobVvu);

	/* Veřejný výpis z registru osob */
	insertService(m_govServices, new (std::nothrow) Gov::SrvcSzrRosVv);
}

void DlgGovServices::clearGovServices(void)
{
	foreach (const Gov::Service *gs, m_govServices.values()) {
		delete const_cast<Gov::Service *>(gs);
	}
	m_govServices.clear();
}

void DlgGovServices::loadServicesToModel(void) const
{
	debugFuncCall();

	foreach (const QString &key, m_govServices.keys()) {
		const Gov::Service *cgs = m_govServices.value(key);
		if (Q_UNLIKELY(cgs == Q_NULLPTR)) {
			Q_ASSERT(0);
			continue;
		}
		m_govServiceModel->appendService(cgs);
	}
}
