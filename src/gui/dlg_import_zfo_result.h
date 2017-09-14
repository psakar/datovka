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

#ifndef _DLG_IMPORT_ZFO_RESULT_H_
#define _DLG_IMPORT_ZFO_RESULT_H_

#include <QDialog>
#include <QList>
#include <QPair>
#include <QString>

namespace Ui {
	class DlgImportZFOResult;
}

/*!
 * @brief Import ZFO files result dialogue.
 */
class DlgImportZFOResult : public QDialog {
	Q_OBJECT

private:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] fileCnt Total number of imported files.
	 * @param[in] succImportList List of successfully imported file names
	 *                           and import messages.
	 * @param[in] existImportList List of already exiting file names
	 *                            and import messages.
	 * @param[in] errImportList List of file names which could not be
	 *                          imported.
	 * @param[in] parent Parent widget.
	 */
	DlgImportZFOResult(int fileCnt,
	    const QList< QPair<QString, QString> > &succImportList,
	    const QList< QPair<QString, QString> > &existImportList,
	    const QList< QPair<QString, QString> > &errImportList,
	    QWidget *parent = Q_NULLPTR);

public:
	/*!
	 * @brief Destructor.
	 */
	~DlgImportZFOResult(void);

	/*!
	 * @brief Shows ZFO import notification dialogue with displaying
	 *     results.
	 *
	 * @param[in] fileCnt Total number of imported files.
	 * @param[in] succImportList List of successfully imported file names
	 *                           and import messages.
	 * @param[in] existImportList List of already exiting file names
	 *                            and import messages.
	 * @param[in] errImportList List of file names which could not be
	 *                          imported.
	 * @param[in] parent Parent widget.
	 */
	static
	void view(int fileCnt,
	    const QList< QPair<QString, QString> > &succImportList,
	    const QList< QPair<QString, QString> > &existImportList,
	    const QList< QPair<QString, QString> > &errImportList,
	    QWidget *parent = Q_NULLPTR);

private:
	Ui::DlgImportZFOResult *m_ui; /*!< UI generated from UI file. */
};

#endif /* _DLG_IMPORT_ZFO_RESULT_H_ */
