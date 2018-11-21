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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations including
 * the two.
 */

#include <QString>

#include "src/gui/icon_container.h"

#define ICON_16x16_PATH ":/icons/16x16/"
#define ICON_24x24_PATH ":/icons/24x24/"
#define ICON_32x32_PATH ":/icons/32x32/"
#define ICON_48x48_PATH ":/icons/48x48/"
#define ICON_64x64_PATH ":/icons/64x64/"
#define ICON_128x128_PATH ":/icons/128x128/"
#define ICON_256x256_PATH ":/icons/256x256/"
#define ICON_3PARTY_PATH ":/icons/3party/"

/* Null objects - for convenience. */
static const QIcon nullIcon;

IconContainer::IconContainer(void)
    : m_icons(MAX_ICONNUM)
{
}

const QIcon &IconContainer::icon(enum Icon i) {
	if (Q_UNLIKELY(i == MAX_ICONNUM)) {
		Q_ASSERT(0);
		return nullIcon;
	}

	register QIcon &icon(m_icons[i]);
	if (Q_UNLIKELY(icon.isNull())) {
		icon = construcIcon(i);
	}
	return icon;
}

QIcon IconContainer::construcIcon(enum Icon i)
{
	QIcon ico;

	switch (i) {
	case ICON_DATOVKA:
		ico.addFile(QStringLiteral(ICON_16x16_PATH "datovka.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_24x24_PATH "datovka.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_32x32_PATH "datovka.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_48x48_PATH "datovka.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_64x64_PATH "datovka.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_128x128_PATH "datovka.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_256x256_PATH "datovka.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_ATTACHMENT:
		ico.addFile(QStringLiteral(ICON_16x16_PATH "attachment.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_24x24_PATH "attachment.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_32x32_PATH "attachment.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_48x48_PATH "attachment.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_64x64_PATH "attachment.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_FLAG:
		ico.addFile(QStringLiteral(ICON_16x16_PATH "flag.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_24x24_PATH "flag.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_32x32_PATH "flag.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_48x48_PATH "flag.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_64x64_PATH "flag.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_HAND:
		ico.addFile(QStringLiteral(ICON_16x16_PATH "hand.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_24x24_PATH "hand.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_32x32_PATH "hand.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_48x48_PATH "hand.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_64x64_PATH "hand.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_HAND_GREY:
		ico.addFile(QStringLiteral(ICON_16x16_PATH "hand_grey.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_24x24_PATH "hand_grey.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_32x32_PATH "hand_grey.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_48x48_PATH "hand_grey.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_64x64_PATH "hand_grey.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_READCOL:
		ico.addFile(QStringLiteral(ICON_16x16_PATH "readcol.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_24x24_PATH "readcol.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_32x32_PATH "readcol.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_48x48_PATH "readcol.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_64x64_PATH "readcol.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_GREY_BALL:
		ico.addFile(QStringLiteral(ICON_16x16_PATH "grey.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_24x24_PATH "grey.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_32x32_PATH "grey.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_48x48_PATH "grey.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_64x64_PATH "grey.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_YELLOW_BALL:
		ico.addFile(QStringLiteral(ICON_16x16_PATH "yellow.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_24x24_PATH "yellow.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_32x32_PATH "yellow.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_48x48_PATH "yellow.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_64x64_PATH "yellow.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_RED_BALL:
		ico.addFile(QStringLiteral(ICON_16x16_PATH "red.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_24x24_PATH "red.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_32x32_PATH "red.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_48x48_PATH "red.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_64x64_PATH "red.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_GREEN_BALL:
		ico.addFile(QStringLiteral(ICON_16x16_PATH "green.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_24x24_PATH "green.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_32x32_PATH "green.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_48x48_PATH "green.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_64x64_PATH "green.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_UP:
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "up_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "up_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case MAX_ICONNUM:
	default:
		Q_ASSERT(0);
		return nullIcon;
		break;
	}

	return ico;
}
