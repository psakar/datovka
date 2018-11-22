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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
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
//#define ICON_128x128_PATH ":/icons/128x128/"
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
	case ICON_DATOVKA_ACCOUNT_SYNC:
		ico.addFile(QStringLiteral(ICON_16x16_PATH "datovka-account-sync.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_24x24_PATH "datovka-account-sync.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_32x32_PATH "datovka-account-sync.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_48x48_PATH "datovka-account-sync.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_64x64_PATH "datovka-account-sync.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_DATOVKA_ALL_ACCOUNTS_SYNC:
		ico.addFile(QStringLiteral(ICON_16x16_PATH "datovka-all-accounts-sync.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_24x24_PATH "datovka-all-accounts-sync.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_32x32_PATH "datovka-all-accounts-sync.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_48x48_PATH "datovka-all-accounts-sync.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_64x64_PATH "datovka-all-accounts-sync.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_DATOVKA_ERROR:
		ico.addFile(QStringLiteral(ICON_16x16_PATH "datovka-error.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_24x24_PATH "datovka-error.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_32x32_PATH "datovka-error.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_48x48_PATH "datovka-error.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_64x64_PATH "datovka-error.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_DATOVKA_MESSAGE:
		ico.addFile(QStringLiteral(ICON_16x16_PATH "datovka-message.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_24x24_PATH "datovka-message.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_32x32_PATH "datovka-message.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_48x48_PATH "datovka-message.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_64x64_PATH "datovka-message.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_DATOVKA_MESSAGE_DOWNLOAD:
		ico.addFile(QStringLiteral(ICON_16x16_PATH "datovka-message-download.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_24x24_PATH "datovka-message-download.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_32x32_PATH "datovka-message-download.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_48x48_PATH "datovka-message-download.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_64x64_PATH "datovka-message-download.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_DATOVKA_MESSAGE_REPLY:
		ico.addFile(QStringLiteral(ICON_16x16_PATH "datovka-message-reply.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_24x24_PATH "datovka-message-reply.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_32x32_PATH "datovka-message-reply.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_48x48_PATH "datovka-message-reply.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_64x64_PATH "datovka-message-reply.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_DATOVKA_MESSAGE_SIGNATURE:
		ico.addFile(QStringLiteral(ICON_16x16_PATH "datovka-message-signature.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_24x24_PATH "datovka-message-signature.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_32x32_PATH "datovka-message-signature.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_48x48_PATH "datovka-message-signature.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_64x64_PATH "datovka-message-signature.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_DATOVKA_MESSAGE_UPLOAD:
		ico.addFile(QStringLiteral(ICON_16x16_PATH "datovka-message-upload.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_24x24_PATH "datovka-message-upload.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_32x32_PATH "datovka-message-upload.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_48x48_PATH "datovka-message-upload.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_64x64_PATH "datovka-message-upload.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_DATOVKA_MESSAGE_VERIFY:
		ico.addFile(QStringLiteral(ICON_16x16_PATH "datovka-message-verify.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_24x24_PATH "datovka-message-verify.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_32x32_PATH "datovka-message-verify.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_48x48_PATH "datovka-message-verify.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_64x64_PATH "datovka-message-verify.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_DATOVKA_OK:
		ico.addFile(QStringLiteral(ICON_16x16_PATH "datovka-ok.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_24x24_PATH "datovka-ok.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_32x32_PATH "datovka-ok.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_48x48_PATH "datovka-ok.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_64x64_PATH "datovka-ok.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_DATOVKA_STOCK_KEY:
		ico.addFile(QStringLiteral(ICON_16x16_PATH "datovka-stock-key.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_24x24_PATH "datovka-stock-key.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_32x32_PATH "datovka-stock-key.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_48x48_PATH "datovka-stock-key.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_64x64_PATH "datovka-stock-key.png"), QSize(), QIcon::Normal, QIcon::Off);
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
	case ICON_MACOS_WINDOW:
		ico.addFile(QStringLiteral(ICON_16x16_PATH "macos_window.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_24x24_PATH "macos_window.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_32x32_PATH "macos_window.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_48x48_PATH "macos_window.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_64x64_PATH "macos_window.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_READCOL:
		ico.addFile(QStringLiteral(ICON_16x16_PATH "readcol.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_24x24_PATH "readcol.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_32x32_PATH "readcol.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_48x48_PATH "readcol.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_64x64_PATH "readcol.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_SAVE:
		ico.addFile(QStringLiteral(ICON_16x16_PATH "save.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_24x24_PATH "save.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_32x32_PATH "save.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_48x48_PATH "save.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_64x64_PATH "save.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_SAVE_ALL:
		ico.addFile(QStringLiteral(ICON_16x16_PATH "save_all.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_24x24_PATH "save_all.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_32x32_PATH "save_all.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_48x48_PATH "save_all.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_64x64_PATH "save_all.png"), QSize(), QIcon::Normal, QIcon::Off);
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
	case ICON_ADDRESS:
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "address_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "address_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_BRIEFCASE:
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "briefcase_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "briefcase_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_BRIEFCASE_GREY:
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "briefcase_grey_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "briefcase_grey_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_CLIPBOARD:
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "clipboard_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "clipboard_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_DELETE:
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "delete_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "delete_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_DOWN:
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "down_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "down_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_FOLDER:
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "folder_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "folder_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_GEAR:
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "gear_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "gear_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_GLOBE:
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "globe_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "globe_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_HELP:
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "help_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "help_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_HOME:
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "home_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "home_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_INFO:
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "info_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "info_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_LABEL:
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "label_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "label_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_LEFT:
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "left_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "left_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_LETTER:
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "letter_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "letter_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_MONITOR:
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "monitor_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "monitor_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_PENCIL:
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "pencil_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "pencil_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_PLUS:
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "plus_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "plus_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_RIGHT:
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "right_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "right_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_SEARCH:
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "search_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "search_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_SHIELD:
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "shield_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "shield_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_STATISTICS:
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "statistics_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "statistics_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_TRASH:
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "trash_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "trash_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_UP:
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "up_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "up_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_USER:
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "user_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(QStringLiteral(ICON_3PARTY_PATH "user_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case ICON_WARNING:
		ico.addFile(ICON_3PARTY_PATH + QString("warning_16.png"), QSize(), QIcon::Normal, QIcon::Off);
		ico.addFile(ICON_3PARTY_PATH + QString("warning_32.png"), QSize(), QIcon::Normal, QIcon::Off);
		break;
	case MAX_ICONNUM:
	default:
		Q_ASSERT(0);
		return nullIcon;
		break;
	}

	return ico;
}
