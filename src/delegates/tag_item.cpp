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

#include <algorithm> /* std::sort */
#include <QPainter>

#include "src/datovka_shared/localisation/localisation.h"
#include "src/datovka_shared/log/log.h"
#include "src/delegates/tag_item.h"
#include "src/dimensions/dimensions.h"

TagItem::TagItem(void)
    : TagDb::TagEntry()
{
}

TagItem::TagItem(const TagDb::TagEntry &entry)
    : TagDb::TagEntry(entry)
{
}

int TagItem::paint(class QPainter *painter,
    const QStyleOptionViewItem &option) const
{
	Q_ASSERT(0 != painter);

	const QRect &rect(option.rect);

	painter->save();

	const QSize hint(sizeHint(option));
	int border = (rect.height() - hint.height()) / 2;

	int width = hint.width();
	int height = hint.height();

	QRectF drawnRect(0, border, width, height);

	QPainterPath path;
	int rounding = (int)(0.15 * height);
	path.addRoundedRect(drawnRect, rounding, rounding, Qt::AbsoluteSize);

	if (option.state & QStyle::State_Selected) {
		painter->fillRect(rect, option.palette.highlight());
	}

	painter->setRenderHint(QPainter::Antialiasing, true);
	painter->translate(rect.x(), rect.y());

	const int margin = Dimensions::margin(option);

	painter->translate(margin, 0);

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

	return width + margin;
}

QSize TagItem::sizeHint(const QStyleOptionViewItem &option) const
{
	const QFont &font(option.font);

	int height = QFontMetrics(font).height();
	const int padding = Dimensions::padding(height);

	int width = QFontMetrics(font).width(name);
	width += 2 * padding;

	height += 2 * padding;

	return QSize(width, height);
}

QColor TagItem::adjustForegroundColour(const QColor &fgColour,
    const QColor &tagColour)
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

TagItemList::TagItemList(void)
    : QList<TagItem>()
{
}

TagItemList::TagItemList(const QList<TagDb::TagEntry> &tagList)
    : QList<TagItem>()
{
	foreach (const TagDb::TagEntry entry, tagList) {
		QList<TagItem>::append(TagItem(entry));
	}
}

void TagItemList::paint(class QPainter *painter,
    const QStyleOptionViewItem &option) const
{
	if (0 == painter) {
		Q_ASSERT(0);
		return;
	}

	painter->save();

	foreach (const TagItem &tag, *this) {
		int width = tag.paint(painter, option);
		painter->translate(width, 0);
	}

	painter->restore();
}

QSize TagItemList::sizeHint(const QStyleOptionViewItem &option) const
{
	int width = 0;
	const int margin = Dimensions::margin(option);

	foreach (const TagItem &tag, *this) {
		width += tag.sizeHint(option).width();
		width += 2 * margin;
	}
	width += margin;

	/* Don't care about vertical dimensions here. */
	return QSize(width, 1);
}

/*!
 * @brief Used for sorting tag lists.
 */
class TagItemLess {
public:
	bool operator()(const TagItem &a, const TagItem &b) const
	{
		return Localisation::stringCollator.compare(a.name, b.name) < 0;
	}
};

void TagItemList::sortNames(void)
{
	::std::sort(this->begin(), this->end(), TagItemLess());
}
