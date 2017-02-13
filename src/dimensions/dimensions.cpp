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
	Q_ASSERT(Q_NULLPTR != widget);

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

QRect Dimensions::availableScreenSize(void)
{
	return QApplication::desktop()->availableGeometry();
}

QRect Dimensions::screenSize(void)
{
	return QApplication::desktop()->screenGeometry();
}

QSize Dimensions::windowSize(const QWidget *widget, qreal wr, qreal hr)
{
	if (widget == Q_NULLPTR || wr <= 0.0 || hr <= 0.0) {
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
	if (widget == Q_NULLPTR) {
		return QRect(40, 40, 400, 300);
	}

	int x = 0; /* top */
	int y = 0; /* left */
	int w = 400; /* width */
	int h = 300; /* height */

	/* Reduce dimensions if they exceed screen width. */
	QRect availRect(availableScreenSize());

	if (wr <= 0.0 || hr <= 0.0) {
		/* Use available screen space to compute geometry. */
		QRect windowRect(widget->geometry());
		QRect frameRect(widget->frameGeometry());

		/* Frame differences don't work on X11. */
		x = frameRect.x() - windowRect.x();
		y = frameRect.y() - windowRect.y();
		if (x < 0) {
			x = -x;
		}
		if (y < 0) {
			y = -y;
		}
		w = frameRect.width() - windowRect.width();
		h = frameRect.height() - windowRect.height();
		if (w < 0) {
			w = -w;
		}
		if (h < 0) {
			h = -h;
		}
		//int right = w - x;
		int bottom = h - y;

		if (y != 0) {
			w = availRect.width() - (2 * (y + bottom));
			h = availRect.height() - (2 * (y + bottom));
		} else {
			w = availRect.width() * m_screenRatio;
			h = availRect.height() * m_screenRatio;
		}
	} else {
		/* Use font height to determine window size. */
		int fh = fontHeight(widget);
		w = fh * wr;
		h = fh * hr;

		if (availRect.width() < w) {
			w = availRect.width() * m_screenRatio;
		}
		if (availRect.height() < h) {
			h = availRect.height() * m_screenRatio;
		}
	}

	/* Compute centred window position. */
	x = availRect.x() + ((availRect.width() - w) / 2);
	y = availRect.y() + ((availRect.height() - h) / 2);

	return QRect(x, y, w, h);
}
