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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations including
 * the two.
 */

#include <QImage>
#include <QPainter>
#include <QSvgRenderer>

#include "src/datovka_shared/graphics/graphics.h"

QPixmap Graphics::pixmapFromSvg(const QByteArray &svgData, int edgeLen)
{
	if (svgData.isEmpty()) {
		return QPixmap();
	}

	QSvgRenderer renderer;
	if (!renderer.load(svgData)) {
		return QPixmap();
	}

	QImage image(edgeLen, edgeLen, QImage::Format_ARGB32);
	QPainter painter(&image);
	renderer.render(&painter);

	QPixmap pixmap;
	pixmap.convertFromImage(image);

	return pixmap;
}
