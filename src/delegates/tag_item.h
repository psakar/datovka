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
#include <QSize>
#include <QString>
#include <QStyleOptionViewItem>

/*!
 * @brief Describes tag information.
 */
class TagItem {

public:
	/*!
	 * @brief Constructor of invalid tag item.
	 *
	 * @note Identifier is -1, has empty name, colour is 'ffffff';
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
	 * @note Invalid tag has id equal to -1, empty name and bogus colour.
	 *
	 * @return True if tag contains valid data.
	 */
	bool isValid(void) const;

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
	 * @brief Returns true if colour string is valid.
	 *
	 * @param[in] colourStr Colour string.
	 * @return True if colour string is valid.
	 */
	static
	bool isValidColourStr(const QString &colourStr);

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
	 * @brief Constructor.
	 */
	TagItemList(void);

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] tagList List of tag items.
	 */
	TagItemList(const QList<TagItem> &tagList);

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
