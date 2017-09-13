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

#ifndef _DLG_IMPORT_ZFO_H_
#define _DLG_IMPORT_ZFO_H_

#include <QDialog>

#include "src/io/imports.h"

namespace Ui {
	class DlgImportZFO;
}

/*!
 * @brief Import ZFO files settings dialogue.
 */
class DlgImportZFO : public QDialog {
	Q_OBJECT

public:
	/*!
	 * Specifies how/where ZFOs will load.
	 */
	enum ZFOlocation {
		IMPORT_FROM_DIR,
		IMPORT_FROM_SUBDIR,
		IMPORT_SEL_FILES
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] parent Parent widget.
	 */
	explicit DlgImportZFO(QWidget *parent = Q_NULLPTR);

	/*!
	 * @brief Destructor.
	 */
	~DlgImportZFO(void);

	/*!
	 * @brief Obtain configuration for ZFO import.
	 *
	 * @param[out] zfoType Specifies ZFO type.
	 * @param[out] locationType Specifies how/where ZFOs will be loaded.
	 * @param[out] checkZfoOnServer Specifies whether ZFOs are going to be
	 *                              checked on the ISDS server.
	 * @param[in] parent Parent widget.
	 * @return True when dialogue has been accepted.
	 */
	static
	bool getImportConfiguration(enum Imports::Type &zfoType,
	    enum ZFOlocation &locationType, bool &checkZfoOnServer,
	    QWidget *parent = Q_NULLPTR);

private slots:
	/*!
	 * @brief Sets activity of control elements according to chosen values.
	 */
	void setControlsActivity(void);

private:
	Ui::DlgImportZFO *m_ui; /*!< UI generated from UI file. */
};

#endif /* _DLG_IMPORT_ZFO_H_ */
