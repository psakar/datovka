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

#ifndef _IMPORTS_H_
#define _IMPORTS_H_

#include "src/worker/task.h"

/*!
 * @brief Provides zfo and message import to local database.
 */
class Imports {

public:
	/*!
	 * @brief ZFO type.
	 */
	enum Type {
		IMPORT_ANY, /*!< All types. */
		IMPORT_MESSAGE, /*!< Data message. */
		IMPORT_DELIVERY /*!< Delivery info. */
	};

	/*!
	 * @brief Import messages from external databases to local database.
	 *
	 * @param[in] dbSet      Account target database set.
	 * @param[in] dbFileList List of external databases to import.
	 * @param[in] userName   Account username.
	 * @param[in] dbId       Databox ID for import.
	 */
	static
	void importDbMsgsIntoDatabase(MessageDbSet &dbSet,
	    const QStringList &dbFileList, const QString &userName,
	    const QString &dbId);

	/*!
	 * @brief Import ZFO file(s) into database by ZFO type.
	 *
	 * @param[in] fileList          List of file path to import.
	 * @param[in] databaseList      List of databases.
	 * @param[in] zfoType           ZFO type for import.
	 * @param[in] authenticate      Check ZFO validity in the ISDS.
	 * @param[out] zfoFilesToImport List of valid ZFOs for import.
	 * @param[out] zfoFilesInvalid  List of invalid ZFOs.
	 * @param[out] numFilesToImport Number of valid ZFOs for import.
	 * @param[out] errTxt           Error text.
	 */
	static
	void importZfoIntoDatabase(const QStringList &fileList,
	    const QList<Task::AccountDescr> &databaseList,
	    enum Type zfoType, bool authenticate,
	    QSet<QString> &zfoFilesToImport,
	    QList< QPair<QString, QString> > &zfoFilesInvalid,
	    int &numFilesToImport, QString &errTxt);

private:
	/*!
	 * @brief Private constructor.
	 *
	 * @note Just prevent any instances of this class.
	 */
	Imports(void);
};

#endif /* _IMPORTS_H_ */
