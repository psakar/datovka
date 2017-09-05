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

#ifndef _DLG_CHANGE_DIRECTORY_H_
#define _DLG_CHANGE_DIRECTORY_H_

#include <QDialog>
#include <QString>

class MessageDbSet; /* Forward declaration. */

namespace Ui {
	class DlgChangeDirectory;
}

/*!
 * @brief Provides interface for database file location change.
 */
class DlgChangeDirectory : public QDialog {
	Q_OBJECT

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] currentDir Current database location directory.
	 * @param[in] parent Parent widget.
	 */
	explicit DlgChangeDirectory(const QString &currentDir,
	    QWidget *parent = Q_NULLPTR);

	/*!
	 * @brief Destructor.
	 */
	virtual
	~DlgChangeDirectory(void);

	/*!
	 * @brief Asks the user for the new database location and relocates it.
	 *
	 * @param[in] userName User name identifying related account.
	 * @param[in] dbSet Affected database set.
	 * @param[in] parent Parent widget.
	 * @return True on success.
	 */
	static
	bool changeDataDirectory(const QString &userName, MessageDbSet *dbSet,
	    QWidget *parent = Q_NULLPTR);

private slots:
	/*!
	 * @brief Choose new data directory.
	 */
	void chooseNewDirectory(void);

private:
	/*!
	 * @brief Identifies chosen action.
	 */
	enum Action {
		ACT_MOVE, /*!< Move database location, delete in original location. */
		ACT_COPY, /*!< Copy database to new location, leave in original location. */
		ACT_NEW /*!< Start new database in new location, leave in original location. */
	};

	/*!
	 * @brief Generates a dialogue asking the user for new location.
	 *
	 * @param[in]  currentDir Current directory.
	 * @param[out] newDir Chosen new directory.
	 * @param[out] action Chosen relocation action.
	 * @param[in] parent Parent widget.
	 * @return True if a new directory and relocation action has been
	 *     selected.
	 */
	static
	bool chooseAction(const QString &currentDir, QString &newDir,
	    enum Action &action, QWidget *parent = Q_NULLPTR);

	/*!
	 * @brief Performs a database relocation.
	 *
	 * @param[in] userName User name identifying related account.
	 * @param[in] dbSet Affected database set.
	 * @param[in] oldDir Old location.
	 * @param[in] newDir New location.
	 * @param[in] action Type or relocation operation.
	 * @param[in] parent Parent widget.
	 * @return True on success.
	 */
	static
	bool relocateDatabase(const QString &userName, MessageDbSet *dbSet,
	    const QString &oldDir, const QString &newDir, enum Action action,
	    QWidget *parent = Q_NULLPTR);

	Ui::DlgChangeDirectory *m_ui; /*!< UI generated from UI file. */
};

#endif /* _DLG_CHANGE_DIRECTORY_H_ */
