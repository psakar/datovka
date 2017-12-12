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

#ifndef _TAG_DB_H_
#define _TAG_DB_H_

#include <QList>
#include <QString>

#include "src/io/sqlite/db.h"

/*!
 * @brief Encapsulates tag database.
 */
class TagDb : public SQLiteDb {

public:
	/*!
	 * @brief Encapsulates database tag entry.
	 */
	class TagEntry {
	public:
		/*!
		 * @brief Constructor of invalid tag entry.
		 *
		 * @note Identifier is -1, has empty name, colour is 'ffffff';
		 */
		TagEntry(void);

		/*!
		 * @brief Constructs a tag entry from supplied parameters.
		 *
		 * @param[in] i Tag identifier.
		 * @param[in] n Tag name.
		 * @param[in] c Tag colour in hex format without the leading hashtag.
		 */
		TagEntry(int i, const QString &n, const QString &c);

		/*!
		 * @brief Check for validity.
		 *
		 * @note Invalid tag has id equal to -1, empty name and bogus colour.
		 *
		 * @return True if tag contains valid data.
		 */
		bool isValid(void) const;

		/*!
		 * @brief Returns true if colour string is valid.
		 *
		 * @param[in] colourStr Colour string.
		 * @return True if colour string is valid.
		 */
		static
		bool isValidColourStr(const QString &colourStr);

		/*!
		 * @brief Comparison operator.
		 *
		 * @param[in] other Another object to compare this one with.
		 * @return True is the other entry is equal to this one.
		 */
		bool operator==(const TagEntry &other) const;

		int id; /*!< Tag identifier. */
		QString name; /*!< Name of the tag. */
		QString colour; /*!<
		                 * Colour of the tag in hex format without
		                 * the leading hashtag.
		                 */
	};

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
	 * @brief Delete all tags from database file.
	 *
	 * @return True on success, false on any error.
	 */
	bool deleteAllTags(void);

	/*!
	 * @brief Get tag data from database file.
	 *
	 * @param[in] id    id of tag.
	 * @return tag struct with data.
	 */
	TagEntry getTagData(int id) const;

	/*!
	 * @brief Get all tags from database file.
	 *
	 * @return List of tag entries.
	 */
	QList<TagEntry> getAllTags(void) const;

	/*!
	 * @brief Get all tags related to given message.
	 *
	 * @param[in] userName account identifier.
	 * @param[in] dmgId Message identifier.
	 * @return List of tags related to message.
	 */
	QList<TagEntry> getMessageTags(const QString &userName,
	    const quint64 msgId) const;

	/*!
	 * @brief Delete all tags for message ID
	 *        in message_tags table.
	 *
	 * @param[in] userName account identifier.
	 * @param[in] msgId    id of message.
	 * @return True on success, false on any error.
	 */
	bool removeAllTagsFromMsg(const QString &userName, qint64 msgId);

	/*!
	 * @brief Assign existing tag to message.
	 *
	 * @param[in] userName account identifier.
	 * @param[in] tagId    id of tag.
	 * @param[in] msgId    id of message.
	 * @return True on success, false on any error.
	 */
	bool assignTagToMsg(const QString &userName, int tagId, qint64 msgId);

	/*!
	 * @brief Remove tag from message.
	 *
	 * @param[in] userName account identifier.
	 * @param[in] tagId    id of tag.
	 * @param[in] msgId    id of message.
	 * @return True on success, false on any error.
	 */
	bool removeTagFromMsg(const QString &userName, int tagId, qint64 msgId);

	/*!
	 * @brief Remove tag from all messages in account specified by username.
	 *
	 * @param[in] userName account identifier.
	 * @return True on success, false on any error.
	 */
	bool removeAllMsgTagsFromAccount(const QString &userName);

	/*!
	 * @brief Get message IDs related from tag contain search text.
	 *
	 * @param[in] text    search text of tag name.
	 * @return List of message IDs related to tag text.
	 */
	QList<qint64> getMsgIdsContainSearchTagText(const QString &text) const;

protected:
	/*!
	 * @brief Returns list of tables.
	 *
	 * @return List of pointers to tables.
	 */
	virtual
	QList<class SQLiteTbl *> listOfTables(void) const Q_DECL_OVERRIDE;
};

/*!
 * @brief Global tag database.
 */
extern TagDb *globTagDbPtr;

/*!
 * @brief Return a hash value for the supplied tag entry.
 *
 * @param[in] entry Tag entry to compute hash from.
 * @param[in] seed Is used to initialise the hash if specified.
 * @return Hashed tag entry.
 */
uint qHash(const TagDb::TagEntry &entry, uint seed = 0);

#endif /* _TAG_DB_H_ */
