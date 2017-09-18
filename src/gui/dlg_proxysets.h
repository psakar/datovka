/*
 * Copyright (C) 2014-2017 CZ.NIC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
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

#ifndef _DLG_PROXYSETS_H_
#define _DLG_PROXYSETS_H_

#include <QDialog>

#include "src/settings/proxy.h"

namespace Ui {
	class DlgProxysets;
}

/*!
 * @brief Proxy settings dialogue.
 */
class DlgProxysets : public QDialog {
	Q_OBJECT

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] parent Parent widget.
	 */
	explicit DlgProxysets(QWidget *parent = Q_NULLPTR);

	/*!
	 * @brief Destructor.
	 */
	~DlgProxysets(void);

private slots:
	void showHttpProxyPassword(int state);
	void showHttpsProxyPassword(int state);

	void toggleHttpProxyDetection(bool state);
	void toggleHttpsProxyDetection(bool state);

	void toggleHttpProxyPassword(bool state);
	void toggleHttpsProxyPassword(bool state);

	void saveChanges(void) const;
	void setActiveHttpProxyEdit(bool state);
	void setActiveHttpsProxyEdit(bool state);

private:
	void loadProxyDialog(const ProxiesSettings &proxySettings);

	Ui::DlgProxysets *m_ui; /*!< UI generated from UI file. */
};

#endif /* _DLG_PROXYSETS_H_ */
