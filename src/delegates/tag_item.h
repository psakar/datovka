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

#ifndef _TAG_ITEM_H_
#define _TAG_ITEM_H_

#include <QList>
#include <QMetaType>
#include <QPalette>
#include <QRect>
#include <QSize>
#include <QString>

/*!
 * @brief Describes tag information.
 */
class TagItem {

public:
	/*!
	 * @brief Constructor of invalid tag item.
	 */
	TagItem(void);

	/*!
	 * @brief Constructs a tag item from supplied parameters.
	 *
	 * @param[in] i Tag identifier.
	 * @param[in] n Tag name.
	 * @param[in] c Tag colour in hex format without the leading hashtag.
	 */
	TagItem(int i, const QString &n, const QString &c);

	/*!
	 * @brief Check for validity.
	 *
	 * @return True if tag contains valid data.
	 */
	bool isValid(void) const;

	/*!
	 * @brief Paint tag rectangle.
	 *
	 * @param[in,out] painter Painter.
	 * @param[in]     rect    Rectangle to be used.
	 * @param[in]     font    Font to be used.
	 * @param[in]     palette Palette to be used.
	 * @return Width of the drawn rectangle (including margin);
	 */
	int paint(class QPainter *painter, const QRect &rect,
	    const QFont &font, const QPalette &palette) const;

	/*!
	 * @brief Gives size hint for the tag rectangle.
	 *
	 * @param[in] rect Rectangle to be drawn into.
	 * @param[in] font Font to be used.
	 * @return Size of the element.
	 */
	QSize sizeHint(const QRect &rect, const QFont &font) const;

	int id; /*!< Tag identifier. */
	QString name; /*!< Name of the rag. */
	QString colour; /*!<
	                 * Colour of the tag in hex format without the leading
	                 * hashtag.
	                 */
};

class TagItemList : public QList<TagItem> {

public:
	/*!
	 * @brief Paint all list elements.
	 */
	void paint(class QPainter *painter, const QRect &rect,
	    const QFont &font, const QPalette &palette) const;

	/*!
	 * @brief Gives size hint for the tag rectangles.
	 *
	 * @param[in] rect Rectangle to be drawn into.
	 * @param[in] font Font to be used.
	 * @return Size of the element.
	 */
	QSize sizeHint(const QRect &rect, const QFont &font) const;
};

Q_DECLARE_METATYPE(TagItem)
Q_DECLARE_METATYPE(TagItemList)

#endif /* _TAG_ITEM_H_ */
