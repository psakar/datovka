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

#include "src/views/external_size_hint_tool_button.h"

ExternalSizeHintToolButton::ExternalSizeHintToolButton(QWidget *parent)
    : QToolButton(parent),
    m_sizeHintWidget(Q_NULLPTR)
{
}

void ExternalSizeHintToolButton::setVerticalSizeHintOrigin(QWidget *widget)
{
	m_sizeHintWidget = widget;
}

/*
 * There are still some problems with the size of the tool buttons.
 * The buttons appear not to enlarge properly on 1080p displays. When testing
 * on a smaller-resolution display (1366x768 on Win 10) the buttons seem
 * to enlarge according to the size of the origin widget.
 */

QSize ExternalSizeHintToolButton::sizeHint(void) const
{
	QSize sizeHint(QToolButton::sizeHint());

	if (m_sizeHintWidget == Q_NULLPTR) {
		return sizeHint;
	}

	/*
	 * Don't use m_sizeHintWidget->sizeHint().height() at this may
	 * be smaller than the actual size.
	 */
	int edgeHint = m_sizeHintWidget->height();

	sizeHint.setHeight(edgeHint);
	if (sizeHint.width() < edgeHint) {
		sizeHint.setWidth(edgeHint);
	}

	return sizeHint;
}
