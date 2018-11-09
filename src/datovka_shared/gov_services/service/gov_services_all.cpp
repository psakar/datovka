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
#include "src/datovka_shared/gov_services/service/gov_services_all.h"
#include "src/datovka_shared/gov_services/service/gov_szr_rob_vu.h"
#include "src/datovka_shared/gov_services/service/gov_szr_rob_vvu.h"
#include "src/datovka_shared/gov_services/service/gov_szr_ros_vv.h"
#include "src/datovka_shared/log/log.h"

/*!
 * @brief Add a newly allocated service into the service container.
 *
 * @note The service object is freed when it cannot be added into the container.
 *
 * @param[in] map Service container.
 * @param[in] gs Newly allocated service object.
 */
static
void insertService(QMap<QString, const Gov::Service *> &map, Gov::Service *gs)
{
	if (Q_UNLIKELY(gs == Q_NULLPTR)) {
		return;
	}

	const QString &key(gs->internalId());
	if (!map.contains(key)) {
		map.insert(key, gs);
	} else {
		logError("Key '%s' already exists in e-gov service container.",
		    key.toUtf8().constData());
		delete gs;
	}
}

QMap<QString, const Gov::Service *> Gov::allServiceMap(void)
{
	QMap<QString, const Service *> map;

	/* Výpis bodového hodnocení z Centrálního registru řidičů */
	insertService(map, new (std::nothrow) SrvcMvCrrVbh);

	/* Výpis z insolvenčního rejstříku */
	insertService(map, new (std::nothrow) SrvcMvIrVp);

	/* Výpis z Rejstříku trestů */
	insertService(map, new (std::nothrow) SrvcMvRtVt);

	/* Výpis z Rejstříku trestů právnických osob */
	insertService(map, new (std::nothrow) SrvcMvRtpoVt);

	/* Výpis ze seznamu kvalifikovaných dodavatelů */
	insertService(map, new (std::nothrow) SrvcMvSkdVp);

	/* Výpis z veřejného rejstříku */
	insertService(map, new (std::nothrow) SrvcMvVrVp);

	/* Výpis z živnostenského rejstříku */
	insertService(map, new (std::nothrow) SrvcMvZrVp);

	/* Výpis z Registru obyvatel */
	insertService(map, new (std::nothrow) SrvcSzrRobVu);

	/* Výpis o využití údajů z registru obyvatel */
	insertService(map, new (std::nothrow) SrvcSzrRobVvu);

	/* Veřejný výpis z registru osob */
	insertService(map, new (std::nothrow) SrvcSzrRosVv);

	return map;
}

void Gov::clearServiceMap(QMap<QString, const Service *> &map)
{
	foreach (const Service *gs, map.values()) {
		delete const_cast<Service *>(gs);
	}
	map.clear();
}
