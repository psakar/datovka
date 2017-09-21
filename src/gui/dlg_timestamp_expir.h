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

#ifndef _DLG_TIMESTAMP_EXPIR_H_
#define _DLG_TIMESTAMP_EXPIR_H_

#include <QDialog>

namespace Ui {
	class DlgTimestampExpir;
}

/*!
 * @brief Shows question about time stamp expiration check.
 */
class DlgTimestampExpir : public QDialog {
	Q_OBJECT

public:
	enum TSaction {
		CHECK_TIMESTAMP_CURRENT,
		CHECK_TIMESTAMP_ALL,
		CHECK_TIMESTAMP_ZFO,
		CHECK_TIMESTAMP_ZFO_SUB
	};

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] parent Parent widget.
	 */
	explicit DlgTimestampExpir(QWidget *parent = Q_NULLPTR);

	/*!
	 * @brief Destructor.
	 */
	~DlgTimestampExpir(void);

signals:
	void returnAction(enum DlgTimestampExpir::TSaction);

private slots:
	void setRetValue(void);
	void ChangeRadioBox(void);

private:
	Ui::DlgTimestampExpir *m_ui; /*!< UI generated from UI file. */
};

#endif /* _DLG_TIMESTAMP_EXPIR_H_ */
