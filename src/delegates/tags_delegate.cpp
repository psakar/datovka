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

#include "src/delegates/tag_item.h"
#include "src/delegates/tags_delegate.h"
#include "src/log/log.h"

TagsDelegate::TagsDelegate(QWidget *parent)
    : QStyledItemDelegate(parent)
{
}

void TagsDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
    const QModelIndex &index) const
{
	if (index.column() == 7) {
//		logInfoNL("%s", "AAA001");
		static TagItemList tagList;
		if (tagList.isEmpty()) {
			tagList.append(TagItem(0, "one", "ff0000"));
			tagList.append(TagItem(0, "two", "00ff00"));
			tagList.append(TagItem(0, "three", "0000ff"));
			tagList.append(TagItem(0, "four", "ef4444"));
			tagList.append(TagItem(0, "five", "faa31b"));
			tagList.append(TagItem(0, "six", "fff000"));
			tagList.append(TagItem(0, "seven", "82c341"));
			tagList.append(TagItem(0, "eight", "009f75"));
			tagList.append(TagItem(0, "nine", "88c6ed"));
			tagList.append(TagItem(0, "ten", "394ba0"));
			tagList.append(TagItem(0, "eleven", "d54799"));
		}

		tagList.paint(painter, option.rect, option.font,
		    option.palette);
		return;
	}

//	if (index.data().canConvert<TagItemList>()) {
//		TagItemList tagList = qvariant_cast<TagItemList>(index.data());
//		/* TODO */
//		tagList.paint(painter, option.rect, option.palette);
//	} else {
		QStyledItemDelegate::paint(painter, option, index);
//	}
}

QSize TagsDelegate::TagsDelegate::sizeHint(const QStyleOptionViewItem &option,
    const QModelIndex &index) const
{
	if (index.column() == 7) {
//		logInfoNL("%s", "AAA000");
		static TagItemList tagList;
		if (tagList.isEmpty()) {
			tagList.append(TagItem(0, "one", "ff0000"));
			tagList.append(TagItem(0, "two", "00ff00"));
			tagList.append(TagItem(0, "three", "0000ff"));
			tagList.append(TagItem(0, "four", "ef4444"));
			tagList.append(TagItem(0, "five", "faa31b"));
			tagList.append(TagItem(0, "six", "fff000"));
			tagList.append(TagItem(0, "seven", "82c341"));
			tagList.append(TagItem(0, "eight", "009f75"));
			tagList.append(TagItem(0, "nine", "88c6ed"));
			tagList.append(TagItem(0, "ten", "394ba0"));
			tagList.append(TagItem(0, "eleven", "d54799"));
		}

		return tagList.sizeHint(option.rect, option.font);
	}

//	if (index.data().canConvert<TagItemList>()) {
//		TagItemList tagList = qvariant_cast<TagItemList>(index.data());
//		return tagList.sizeHint();
//	} else {
		return QStyledItemDelegate::sizeHint(option, index);
//	}
}
