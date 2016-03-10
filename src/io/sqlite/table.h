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

#ifndef _SQLITE_TABLE_H_
#define _SQLITE_TABLE_H_

#include <QMap>
#include <QPair>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

/*!
 * @brief Used data types in databases.
 */
enum EntryType {
	DB_INTEGER = 1,
	DB_INT_PROCESSING_STATE, /* Integer. */
	DB_TEXT,
	DB_BOOLEAN,
	DB_BOOL_READ_LOCALLY, /* Boolean. */
	DB_BOOL_ATTACHMENT_DOWNLOADED, /* Boolean. */
	DB_DATETIME,
	DB_DATE
};

/*!
 * @brief Table prototype.
 */
class SQLiteTbl {
public:
	/*!
	* @brief Table attribute property.
	 */
	class AttrProp {
	public:
		enum EntryType type; /*!< Attribute type. */
		QString desc; /*!< Attribute description. */
	};

	/*!
	 * @brief Constructor.
	 */
	SQLiteTbl(const QString &name,
	    const QVector< QPair<QString, enum EntryType> > &attrs,
	    const QMap<QString, AttrProp> &props,
	    const QMap<QString, QString> &colCons = emptyColConstraints,
	    const QString &tblCons = emptyTblConstraint);

	/*!
	 * @brief Check whether table in database exists.
	 *
	 * @param[in] db Database.
	 * @return True if table in database already exists.
	 */
	bool existsInDb(const QSqlDatabase &db) const;

	/*!
	 * @brief Create empty table in supplied database.
	 *
	 * @param[in] db Database.
	 * @return True if table in database was successfully created.
	 */
	bool createEmpty(QSqlDatabase &db) const;

	/*!
	 * @brief Reloads descriptions according to selected localisation.
	 */
	void reloadLocalisedDescription(void);

	/*! Table name. */
	const QString &tabName;
	/*! Known attributes. */
	const QVector< QPair<QString, enum EntryType> > &knownAttrs;
	/*! Attribute properties as described in source codes. */
	const QMap<QString, AttrProp> &attrPropsSources;
	/*! Attribute properties that have (or not) been translated. */
	QMap<QString, AttrProp> attrProps;

private:
	/*!
	 * @brief Converts db types to strings.
	 *
	 * @param[in] entryType Entry type value.
	 * @return Name of entry type.
	 */
	static
	const QString &entryType2SQLiteDataType(enum EntryType entryType);

	/*! Column constraints. */
	const QMap<QString, QString> &colConstraints;
	/*! Table constraint. */
	const QString &tblConstraint;

	/*! Empty column constraints. */
	static
	const QMap<QString, QString> emptyColConstraints;
	/*! Empty table constraint. */
	static
	const QString emptyTblConstraint;
};

#endif /* _SQLITE_TABLE_H_ */
