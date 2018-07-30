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

#pragma once

#include <QDialog>

class MemoryLog; /* Forward declaration. */

namespace Ui {
	class DlgViewLog;
}

/*!
 * @brief Dialogue for viewing of memory log content.
 */
class DlgViewLog : public QDialog {
	Q_OBJECT

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] parent Parent widget.
	 * @param[in] flags Window flags.
	 */
	explicit DlgViewLog(QWidget *parent = Q_NULLPTR,
	    Qt::WindowFlags flags = Qt::WindowFlags());

	/*!
	 * @brief Destructor.
	 */
	~DlgViewLog(void);

private slots:
	/*!
	 * @brief Appends newly logged message.
	 *
	 * @param[in] key Message key.
	 */
	void handleNewMsg(quint64 key);

private:
	/*!
	 * @brief Append log message entry.
	 */
	void appendNewMsg(const QString &msg);

	Ui::DlgViewLog *m_ui; /*!< UI generated from UI file. */

	MemoryLog *m_memLog; /*!< Pointer to memory log. */
};
