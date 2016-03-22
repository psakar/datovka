/*
 * Copyright (C) 2014-2016 CZ.NIC
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

#include <QDebug> /* TODO -- Remove this include. */
#include <QPainter>

#include "src/delegates/tag_item.h"
#include "src/log/log.h"

/*!
 * @brief Adjust foreground colour according to the supplied label colour.
 *
 * @param[in] fgColour  Foreground colour.
 * @param[in] tagColour Tag rectangle colour.
 * @return Colour adjusted to the background colour.
 */
QColor adjustForegroundColour(const QColor &fgColour, const QColor &tagColour)
{
	Q_UNUSED(fgColour);

	int r = tagColour.red();
	int g = tagColour.green();
	int b = tagColour.blue();

#if 0
	int colour = (r << 16) + (g << 8) + b;
	return (colour > (0xffffff / 2)) ? Qt::black : Qt::white;
#else
	int yiq = ((299 * r) + (587 * g) + (114 * b)) / 1000;
	return (yiq >= 128) ? Qt::black : Qt::white;
#endif
}

TagItem::TagItem(void)
    : id(-1),
    name(),
    colour()
{
}

TagItem::TagItem(int i, const QString &n, const QString &c)
    : id(i),
    name(n),
    colour(c)
{
}

bool TagItem::isValid(void) const
{
	return (id >= 0) && !name.isEmpty() && (6 == colour.size());
}

#define PADDING 5 /* Horizontal padding in pixels. */
#define MARGIN 2 /* Vertical margin. */

int TagItem::paint(class QPainter *painter, const QRect &rect,
    const QFont &font, const QPalette &palette) const
{
	Q_UNUSED(palette);

	Q_ASSERT(0 != painter);

	painter->save();

	int width = sizeHint(rect, font).width();
	int height = rect.height() - (2 * MARGIN);

	QRectF drawnRect(0, MARGIN, width, height);

	QPainterPath path;
	int rounding = (int) (0.15 * height);
	path.addRoundedRect(drawnRect, rounding, rounding, Qt::AbsoluteSize);

	painter->setRenderHint(QPainter::Antialiasing, true);
	painter->translate(rect.x(), rect.y());

	QColor rectColour("#" + colour);
	if (!rectColour.isValid()) {
		logWarningNL("Invalid tag colour '%s'. Using 'ffffff'",
		    colour.toUtf8().constData());
		rectColour = QColor("#ffffff");
		Q_ASSERT(rectColour.isValid());
	}

	QPen pen(rectColour, 1);
	painter->setPen(pen);
	painter->fillPath(path, rectColour);
	painter->drawPath(path);

	/* TODO -- Obtain foreground colour. */
	painter->setPen(
	    QPen(adjustForegroundColour(Qt::black, rectColour), 1));
	painter->drawText(drawnRect, Qt::AlignCenter, name);

	painter->restore();

	return width + (2 * MARGIN);
}

QSize TagItem::sizeHint(const QRect &rect, const QFont &font) const
{
	Q_UNUSED(rect);

	int width = QFontMetrics(font).width(name);
	width += 2 * PADDING;

	//int height = rect.height();
	//height -= 2 * MARGIN;
	//if (height < 0 ) {
	//	height = 1;
	//}

	return QSize(width, 1);
}

#if 1
void TagItemList::paint(class QPainter *painter, const QRect &rect,
    const QFont &font, const QPalette &palette) const
{
	if (0 == painter) {
		Q_ASSERT(0);
		return;
	}

	painter->save();

	foreach (const TagItem &tag, *this) {
		int width = tag.paint(painter, rect, font, palette);
		painter->translate(width, 0);
	}

	painter->restore();
}

QSize TagItemList::sizeHint(const QRect &rect, const QFont &font) const
{
	int width = 0;

	foreach (const TagItem &tag, *this) {
		width += tag.sizeHint(rect, font).width();
		width += 2 * MARGIN;
	}

	return QSize(width, 1);
}
#else
#define myStarCount 2
#define myMaxStarCount 5
#define PaintingScaleFactor 10

void TagItemList::paint(class QPainter *painter, const QRect &rect,
    const QFont &font, const QPalette &palette) const
{
//
	Q_UNUSED(font);
	painter->save();

	QPolygonF starPolygon;
	{
		starPolygon << QPointF(1.0, 0.5);
	    for (int i = 1; i < 5; ++i)
        	starPolygon << QPointF(0.5 + 0.5 * std::cos(0.8 * i * 3.14),
                	               0.5 + 0.5 * std::sin(0.8 * i * 3.14));
	}

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);

    painter->setBrush(palette.foreground());

    int yOffset = (rect.height() - PaintingScaleFactor) / 2;
    painter->translate(rect.x(), rect.y() + yOffset);
    painter->scale(PaintingScaleFactor, PaintingScaleFactor);

    for (int i = 0; i < myMaxStarCount; ++i) {
        if (i < myStarCount) {
            painter->drawPolygon(starPolygon, Qt::WindingFill);
        }
        painter->translate(1.0, 0.0);
    }

    painter->restore();
//
}

QSize TagItemList::sizeHint(const QRect &rect, const QFont &font) const
{
//
	Q_UNUSED(rect);
	Q_UNUSED(font);
	return PaintingScaleFactor * QSize(myMaxStarCount, 1);
//
}
#endif
