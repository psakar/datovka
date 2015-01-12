/*
 * Copyright (C) 2014-2015 CZ.NIC
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


#ifndef _DB_TABLES_H_
#define _DB_TABLES_H_


#include <QMap>
#include <QPair>
#include <QSqlDatabase>
#include <QString>
#include <QVector>


/*!
 * @brief Used data types in databases.
 */
enum EntryType{
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
 * @brief Converts db types strings.
 */
const QString & entryTypeStr(EntryType entryType);


/*!
 * @brief Table attribute property.
 */
class AttrProp {
public:
	EntryType type; /*!< Attribute type. */
	QString desc; /*!< Attribute description. */
};


/*!
 * @brief Table prototype.
 */
class Tbl {
public:
	/*
	 * Constructor.
	 */
	Tbl(const QString &name,
	    const QVector< QPair<QString, EntryType> > &attrs,
	    const QMap<QString, AttrProp> &props,
	    const QMap<QString, QString> &colCons = Tbl::emptyColConstraints,
	    const QString &tblCons = emptyTblConstraint);

	/*! Table name. */
	const QString &tabName;

	/*! Known attributes. */
	const QVector< QPair<QString, EntryType> > &knownAttrs;

	/*! Attribute properties. */
	const QMap<QString, AttrProp> &attrPropsSources;
	QMap<QString, AttrProp> attrProps;

	/*!
	 * @brief Return true if table in database exists.
	 */
	bool existsInDb(const QSqlDatabase &db) const;

	/*!
	 * @brief Create empty table in supplied database.
	 */
	bool createEmpty(QSqlDatabase &db) const;

	/*!
	 * @brief Reloads the translation according to the selected
	 *     localisation.
	 */
	void reloadLocalisedDescription(void);

private:
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


/*
 * Account database.
 */
extern Tbl accntinfTbl; /*!< Table 'account_info'. */
extern Tbl pwdexpdtTbl; /*!< Table 'password_expiration_date'. */


/*
 * Message database.
 */
extern Tbl msgsTbl; /*!< Table 'messages'. */
extern Tbl flsTbl; /*!< Table 'files'. */
extern Tbl hshsTbl; /*!< Table 'hashes'. */
extern Tbl evntsTbl; /*!< Table 'events'. */
extern Tbl prcstTbl; /*!< Table 'process_state'. */
extern Tbl rwmsgdtTbl; /*!< Table 'raw_message_data'. */
extern Tbl rwdlvrinfdtTbl; /*!< Table 'raw_delivery_info_data'. */
extern Tbl smsgdtTbl; /*!< Table 'supplementary_message_data'. */
extern Tbl crtdtTbl; /*!< Table 'certificate_data'. */
extern Tbl msgcrtdtTbl; /*!< Table 'message_certificate_data'. */


#endif /* _DB_TABLES_H_ */
