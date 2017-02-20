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


#ifndef _IMPORT_ZFO_H_
#define _IMPORT_ZFO_H_

#include <QObject>

#include "src/worker/task.h"
#include "src/gui/dlg_import_zfo.h"

/*!
 * @brief Provides zfo import to local database.
 */
class ImportZfo : public QObject {
	Q_OBJECT

public:
	/*!
	 * @brief Constructor.
	 */
	explicit ImportZfo(QObject *parent = 0);

	/*!
	 * @brief Import ZFO file(s) into database by ZFO type.
	 *
	 * @param[in] files - List of ZFO paths.
	 * @param[in] accountList - List of account databases.
	 * @param[in] zfoType     - ZFO type for import.
	 * @param[in] authenticate - Check ZFO validity in the ISDS.
	 * @param[out] errTxt      - Error text.
	 */
	void importZfoIntoDatabase(const QStringList &files,
	    const QList<Task::AccountDescr> &accountList,
	    enum ImportZFODialog::ZFOtype zfoType, bool authenticate,
	    QString &errTxt);

private slots:

	/*!
	 * @brief Collects information about import status.
	 *
	 * @param[in] fileName - zfo file name.
	 * @param[in] result   - import result.
	 * @param[in] resultDesc - result desctiprion.
	 */
	void collectImportZfoStatus(const QString &fileName, int result,
	    const QString &resultDesc);

private:

	QSet<QString> m_zfoFilesToImport; /*!< Set of files to be imported. */
	int m_numFilesToImport; /*!< Input ZFO count. */
	QList< QPair<QString, QString> > m_importSucceeded,
	                                 m_importExisted,
	                                 m_importFailed; /*!< Import resulty lists. */

	/*!
	 * @brief Show ZFO import notification dialog with results of imports.
	 *
	 * @param[in] filesCnt - input ZFO count.
	 * @param[in] successFilesList - List of success import files.
	 * @param[in] existFilesList   - List of existing files.
	 * @param[in] errorFilesList   - List of error import files.
	 */
	void showImportZfoResultDialogue(int filesCnt,
	    const QList<QPair<QString,QString>> &successFilesList,
	    const QList<QPair<QString,QString>> &existFilesList,
	    const QList<QPair<QString,QString>> &errorFilesList);
};

/*!
 * @brief Global instance.
 */
extern ImportZfo globImportZfo;

#endif /* _IMPORT_ZFO_H_ */
