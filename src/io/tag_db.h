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

#ifndef _TAG_DB_H_
#define _TAG_DB_H_

#include <QList>
#include <QString>

#include "src/delegates/tag_item.h"
#include "src/io/sqlite/db.h"

/*!
 * @brief Encapsulates tag database.
 */
class TagDb : public SQLiteDb {

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] connectionName Connection name.
	 */
	explicit TagDb(const QString &connectionName);

	/*!
	 * @brief Open database file.
	 *
	 * @param[in] fileName      File name.
	 * @return True on success, false on any error.
	 */
	bool openDb(const QString &fileName);

	/*!
	 * @brief Insert new tag into database file.
	 *
	 * @param[in] tagName       text label of tag.
	 * @param[in] tagColor      color of tag in HEX format.
	 * @return True on success, false on any error.
	 */
	bool insertTag(const QString &tagName, const QString &tagColor);

	/*!
	 * @brief Update tag in database file.
	 *
	 * @param[in] id            id of tag.
	 * @param[in] tagName       text label of tag.
	 * @param[in] tagColor      color of tag in HEX format.
	 * @return True on success, false on any error.
	 */
	bool updateTag(int id, const QString &tagName, const QString &tagColor);

	/*!
	 * @brief Delete tag from database file.
	 *
	 * @param[in] id    id of tag.
	 * @return True on success, false on any error.
	 */
	bool deleteTag(int id);

	/*!
	 * @brief Get tag data from database file.
	 *
	 * @param[in] id    id of tag.
	 * @return tag struct with data.
	 */
	TagItem getTagData(int id);

	/*!
	 * @brief Get all tags from database file.
	 *
	 * @return Llist of TagItem.
	 */
	QList<TagItem> getAllTags(void);

	/*!
	 * @brief Get all tags related to given message.
	 *
	 * @param[in] dmgId Message identifier.
	 * @return List of tags related to message.
	 */
	TagItemList getMessageTags(quint64 msgId);

	/*!
	 * @brief Delete all tags for message ID
	 *        in message_tags table.
	 *
	 * @param[in] msgId    id of message.
	 * @return True on success, false on any error.
	 */
	bool removeAllTagsFromMsg(qint64 msgId);

	/*!
	 * @brief Assign existing tag to message.
	 *
	 * @param[in] tagId    id of tag.
	 * @param[in] msgId    id of message.
	 * @return True on success, false on any error.
	 */
	bool assignTagToMsg(int tagId, qint64 msgId);

	/*!
	 * @brief Remove tag from message.
	 *
	 * @param[in] tagId    id of tag.
	 * @param[in] msgId    id of message.
	 * @return True on success, false on any error.
	 */
	bool removeTagFromMsg(int tagId, qint64 msgId);

	/*!
	 * @brief Get message IDs related from tag contain search text.
	 *
	 * @param[in] text    search text of tag name.
	 * @return List of message IDs related to tag text.
	 */
	QList<qint64> getMsgIdsContainSearchTagText(const QString &text);

private:
	/*!
	 * @brief Returns list of tables.
	 *
	 * @return List of pointers to tables.
	 */
	static
	QList<class SQLiteTbl *> listOfTables(void);
};

/*!
 * @brief Global tag database.
 */
extern TagDb *globTagDbPtr;

#endif /* _TAG_DB_H_ */
