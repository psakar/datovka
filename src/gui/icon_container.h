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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations including
 * the two.
 */

#pragma once

#include <QIcon>
#include <QVector>

/*!
 * @brief Container holding icons used in the application. Its purpose is to
 *     provide icons without the need to build them every time they are needed.
 */
class IconContainer {

public:
	enum Icon {
		ICON_DATOVKA = 0,
		ICON_ATTACHMENT,
		ICON_FLAG,
		ICON_HAND,
		ICON_HAND_GREY,
		ICON_READCOL,
		ICON_GREY_BALL,
		ICON_YELLOW_BALL,
		ICON_RED_BALL,
		ICON_GREEN_BALL,
		/* 3rd party icons below */
		ICON_UP,

		MAX_ICONNUM /* Maximal number of icons (convenience value). */
	};

	/*!
	 * @brief Constructor.
	 */
	IconContainer(void);

	/*!
	 * @brief Returns icon object held within the container. The icon is
	 *     created the first time it is needed. Subsequent calls will return
	 *     the already constructed icon.
	 *
	 * @param[i] i Icon enumeration identifier.
	 * @return Non-null icon object held within the container, null icon on error.
	 */
	const QIcon &icon(enum Icon i);

	/*!
	 * @brief Builds the icon from image resources.
	 *
	 * @param[i] i Icon enumeration identifier.
	 * @return Non-null icon object, null icon on error.
	 */
	static
	QIcon construcIcon(enum Icon i);

private:
	QVector<QIcon> m_icons; /*!< Vector holding all constructed icons. */
};
