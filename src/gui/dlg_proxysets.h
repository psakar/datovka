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
	/*!
	 * @brief View/hide HTTP proxy password settings.
	 *
	 * @param[in] checkState Check box state controlling the visibility.
	 */
	void showHttpProxyPassword(int checkState);

	/*!
	 * @brief View/hide HTTPS proxy password settings.
	 *
	 * @param[in] checkState Check box state controlling the visibility.
	 */
	void showHttpsProxyPassword(int checkState);

	/*!
	 * @brief Enable/disable HTTP proxy detection label.
	 *
	 * @param[in] enabled True if the label should be enabled.
	 */
	void toggleHttpProxyDetection(bool enabled);

	/*!
	 * @brief Enable/disable HTTPS proxy detection label.
	 *
	 * @param[in] enabled True if the label should be enabled.
	 */
	void toggleHttpsProxyDetection(bool enabled);

	/*!
	 * @brief Enable/disable HTTP proxy password settings.
	 *
	 * @param[in] checked False if the label should be enabled.
	 */
	void toggleHttpProxyPassword(bool checked);

	/*!
	 * @brief Enable/disable HTTPS proxy password settings.
	 *
	 * @param[in] checked False if the label should be enabled.
	 */
	void toggleHttpsProxyPassword(bool checked);

	/*!
	 * @brief Enable/disable HTTP proxy address settings.
	 *
	 * @param[in] enabled True if the controls should be enabled.
	 */
	void setActiveHttpProxyEdit(bool enabled);

	/*!
	 * @brief Enable/disable HTTPS proxy address settings.
	 *
	 * @param[in] enabled True if the controls should be enabled.
	 */
	void setActiveHttpsProxyEdit(bool enabled);

	void saveChanges(void) const;

private:
	void loadProxyDialog(const ProxiesSettings &proxySettings);

	Ui::DlgProxysets *m_ui; /*!< UI generated from UI file. */
};

#endif /* _DLG_PROXYSETS_H_ */
