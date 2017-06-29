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

#ifndef _SVG_VIEW_H_
#define _SVG_VIEW_H_

#include <QByteArray>
#include <QGraphicsView>

/*!
 * @brief Graphics view class dedicated for viewing SVG content.
 */
class SvgView : public QGraphicsView {
	Q_OBJECT

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] parent Parent widget.
	 */
	explicit SvgView(QWidget *parent = Q_NULLPTR);

	/*!
	 * @brief Set SVG data.
	 *
	 * @param[in] svgData SVG data to be painted.
	 */
	void setSvgData(const QByteArray &svgData);

protected:
	/*!
	 * @brief Resizes the displayed SVG content.
	 */
	virtual
	void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;

private:
	/*!
	 * @brief Configures the graphics view.
	 */
	void setUp(void);

	/*!
	 * @brief Displays SVG image.
	 *
	 * @param[in] svgData SVG data to be displayed.
	 */
	void displaySvg(const QByteArray &svgData);

	QByteArray m_svgData; /*!< Brief holds SVG data to be painted. */
};

#endif /* _SVG_VIEW_H_ */
