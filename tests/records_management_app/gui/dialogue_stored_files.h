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

#pragma once

#include <QDialog>

class QLineEdit; /*!< Forward declaration. */

namespace Ui {
	class DialogueStoredFiles;
}

class DialogueStoredFiles : public QDialog {
	Q_OBJECT

public:
	/*!
	 * @brief Dialogue return value.
	 */
	class Values {
	public:
		Values(void) : dmIds(), diIds()
		{
		}

		QList<qint64> dmIds; /*!< Message identifiers. */
		QList<qint64> diIds; /*!< Delivery info identifiers. */
	};

private:
	/*!
	 * @brief Dialogue constructor.
	 */
	explicit DialogueStoredFiles(QWidget *parent = Q_NULLPTR);

	~DialogueStoredFiles(void);

public:
	/*!
	 * @brief Invokes the dialogue.
	 */
	static
	Values getIdentifiers(void);

private slots:
	void generateDmIds(void);

	void generateDiIds(void);

private:
	Ui::DialogueStoredFiles *m_ui;
};
