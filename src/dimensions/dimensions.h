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

#ifndef _DIMENSIONS_H_
#define _DIMENSIONS_H_

#include <QStyleOptionViewItem>

/*!
 * @brief Defines some basic dimensions.
 */
class Dimensions {

public:
	/*!
	 * @brief Returns margin as function of font dimensions.
	 *
	 * @param[in] option Style options.
	 * @return Margin width in pixels.
	 */
	static
	int margin(const QStyleOptionViewItem &option);

	/*!
	 * @brief Returns padding as function of supplied dimensions.
	 *
	 * @param[in] height Font height.
	 * @return Padding width in pixels.
	 */
	static
	int padding(int height);

	/*!
	 * @brief Return default line height to table views and widgets.
	 *
	 * @param[in] option Style options.
	 * @return Line height pixels.
	 */
	static
	int tableLineHeight(const QStyleOptionViewItem &option);

	/*!
	 * @brief Returns size of dialogue specified as the ratio of the
	 *     detected font height.
	 *
	 * @param[in] widget Widget to obtain font metrics from.
	 * @param[in] wr Width ratio relative to font height.
	 * @param[in] hr Height ratio relative to font height.
	 * @return Window dimensions.
	 */
	static
	QSize windowSize(const QWidget *widget, qreal wr, qreal hr);

private:
	/*!
	 * @brief Constructor.
	 *
	 * @note Prohibit any class instance.
	 */
	Dimensions(void);

	static
	const qreal m_margin; /*!< Text margin as ratio of text height. */

	static
	const qreal m_padding; /*!< Text padding as ratio of text height. */

	static
	const qreal m_lineHeight; /*!< Height of line as ration of text height. */
};

#endif /* _DIMENSIONS_H_ */