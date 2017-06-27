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

#ifndef _GRAPHICS_H_
#define _GRAPHICS_H_

#include <QByteArray>
#include <QPixmap>

/*!
 * @brief Encapsulates some graphics-related routines.
 */
class Graphics {
public:
	/*!
	 * @brief Draws a pixmap from supplied SVG data.
	 *
	 * @param[in] svgData SVG image data.
	 * @param[in] edgeLen Edge size of the drawn image square.
	 * @return Null pixmap on error.
	 */
	static
	QPixmap pixmapFromSvg(const QByteArray &svgData, int edgeLen);

private:
	/*!
	 * @brief Private constructor.
	 */
	Graphics(void);
};

#endif /* _GRAPHICS_H_ */
