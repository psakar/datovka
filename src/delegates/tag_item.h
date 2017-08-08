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

#ifndef _TAG_ITEM_H_
#define _TAG_ITEM_H_

#include <QList>
#include <QMetaType>
#include <QSize>
#include <QString>
#include <QStyleOptionViewItem>

#include "src/io/tag_db.h"

/*!
 * @brief Describes tag information.
 */
class TagItem : public TagDb::TagEntry {

public:
	/*!
	 * @brief Constructor of invalid tag item.
	 */
	TagItem(void);

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] entry Tag entry to construct item from.
	 */
	explicit TagItem(const TagDb::TagEntry &entry);

	/*!
	 * @brief Paint tag rectangle.
	 *
	 * @param[in,out] painter Painter.
	 * @param[in]     option  Drawing options.
	 * @return Width of the drawn rectangle (including margin);
	 */
	int paint(class QPainter *painter,
	    const QStyleOptionViewItem &option) const;

	/*!
	 * @brief Gives size hint for the tag rectangle.
	 *
	 * @param[in] option Drawing options.
	 * @return Size of the element.
	 */
	QSize sizeHint(const QStyleOptionViewItem &option) const;

	/*!
	 * @brief Adjust foreground colour according to the supplied label
	 *     colour.
	 *
	 * @param[in] fgColour  Foreground colour.
	 * @param[in] tagColour Tag rectangle colour.
	 * @return Colour adjusted to the background colour.
	 */
	static
	QColor adjustForegroundColour(const QColor &fgColour,
	    const QColor &tagColour);
};

class TagItemList : public QList<TagItem> {

public:
	/*!
	 * @brief Constructor.
	 */
	TagItemList(void);

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] tagList List of tag entries.
	 */
	TagItemList(const QList<TagDb::TagEntry> &tagList);

	/*!
	 * @brief Paint all list elements.
	 *
	 * @param[in,out] painter Painter.
	 * @param[in]     option  Drawing options.
	 */
	void paint(class QPainter *painter,
	    const QStyleOptionViewItem &option) const;

	/*!
	 * @brief Gives size hint for the tag rectangles.
	 *
	 * @param[in] option Drawing options.
	 * @return Size of the element.
	 */
	QSize sizeHint(const QStyleOptionViewItem &option) const;

	/*!
	 * @brief Performs a locale-aware sorting of the tag list according to
	 *     tag names.
	 */
	void sortNames(void);
};

Q_DECLARE_METATYPE(TagItem)
Q_DECLARE_METATYPE(TagItemList)

#endif /* _TAG_ITEM_H_ */
