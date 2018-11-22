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

#define ICON_128x128_PATH ":/icons/128x128/" /* Used in PIN dialogue. */

/*!
 * @brief Container holding icons used in the application. Its purpose is to
 *     provide icons without the need to build them every time they are needed.
 */
class IconContainer {

public:
	enum Icon {
		ICON_DATOVKA = 0,
		ICON_DATOVKA_ACCOUNT_SYNC,
		ICON_DATOVKA_ALL_ACCOUNTS_SYNC,
		ICON_DATOVKA_ERROR,
		ICON_DATOVKA_MESSAGE,
		ICON_DATOVKA_MESSAGE_DOWNLOAD,
		ICON_DATOVKA_MESSAGE_REPLY,
		ICON_DATOVKA_MESSAGE_SIGNATURE,
		ICON_DATOVKA_MESSAGE_UPLOAD,
		ICON_DATOVKA_MESSAGE_VERIFY,
		ICON_DATOVKA_OK,
		ICON_DATOVKA_STOCK_KEY,
		ICON_ATTACHMENT,
		ICON_FLAG,
		ICON_HAND,
		ICON_HAND_GREY,
		ICON_MACOS_WINDOW,
		ICON_READCOL,
		ICON_SAVE,
		ICON_SAVE_ALL,
		ICON_GREY_BALL,
		ICON_YELLOW_BALL,
		ICON_RED_BALL,
		ICON_GREEN_BALL,
		/* 3rd party icons below */
		ICON_ADDRESS,
		ICON_BRIEFCASE,
		ICON_BRIEFCASE_GREY,
		ICON_CLIPBOARD,
		ICON_DELETE,
		ICON_DOWN,
		ICON_FOLDER,
		ICON_GEAR,
		ICON_GLOBE,
		ICON_HELP,
		ICON_HOME,
		ICON_INFO,
		ICON_LABEL,
		ICON_LEFT,
		ICON_LETTER,
		ICON_MONITOR,
		ICON_PENCIL,
		ICON_PLUS,
		ICON_RIGHT,
		ICON_SEARCH,
		ICON_SHIELD,
		ICON_STATISTICS,
		ICON_TRASH,
		ICON_UP,
		ICON_USER,
		ICON_WARNING,

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
