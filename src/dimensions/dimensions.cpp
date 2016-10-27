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

#include <QApplication>
#include <QDesktopWidget>
#include <QFontMetrics>
#include <QStyleOption>

#include "src/dimensions/dimensions.h"

const qreal Dimensions::m_margin = 0.4;
const qreal Dimensions::m_padding = 0.1;
const qreal Dimensions::m_lineHeight = 1.5;
const qreal Dimensions::m_screenRatio = 0.8;

/*!
 * @brief Obtain default widget font height.
 *
 * @param[in] widget Pointer to widget.
 * @return Font heigh in pixels.
 */
static inline
int fontHeight(const QWidget *widget)
{
	Q_ASSERT(0 != widget);

	QStyleOption option;
	option.initFrom(widget);
	return option.fontMetrics.height();
}

int Dimensions::margin(const QStyleOptionViewItem &option)
{
	return QFontMetrics(option.font).height() * m_margin;
}

int Dimensions::padding(int height)
{
	return height * m_padding;
}

int Dimensions::tableLineHeight(const QStyleOptionViewItem &option)
{
	return QFontMetrics(option.font).height() * m_lineHeight;
}

QSize Dimensions::windowSize(const QWidget *widget, qreal wr, qreal hr)
{
	if (widget == 0 || wr <= 0.0 || hr <= 0.0) {
		return QSize();
	}

	int height = fontHeight(widget);
	int w = height * wr;
	int h = height * hr;

	/* Reduce window with if it exceeds screen width. */
	QRect screenRect(QApplication::desktop()->screenGeometry());

	if (screenRect.width() < w) {
		qreal ratio =
		    (qreal)w / ((qreal)screenRect.width() * m_screenRatio);
		w /= ratio;
		h *= ratio;
	}

	return QSize(w, h);
}

QRect Dimensions::windowDimensions(const QWidget *widget, qreal wr, qreal hr)
{
	if (widget == 0 || wr <= 0.0 || hr <= 0.0) {
		return QRect(40, 40, 400, 300);
	}

	int height = fontHeight(widget);
	int w = height * wr;
	int h = height * hr;

	/* Reduce dimensions with if they exceed screen width. */
	QRect screenRect(QApplication::desktop()->screenGeometry());

	if (screenRect.width() < w) {
		w = screenRect.width() * m_screenRatio;
	}
	if (screenRect.height() < h) {
		h = screenRect.height() * m_screenRatio;
	}

	/* Compute centred window position. */
	int x = (screenRect.width() - w) / 2;
	int y = (screenRect.height() - h) / 2;

	return QRect(x, y, w, h);
}
