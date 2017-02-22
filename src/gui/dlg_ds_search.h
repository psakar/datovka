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

#ifndef _DLG_DS_SEARCH_H_
#define _DLG_DS_SEARCH_H_

#include <QDialog>
#include <QTimer>

#include "src/common.h"
#include "src/worker/task_search_owner.h"
#include "ui_dlg_ds_search.h"

/*!
 * @brief Data box search dialogue.
 */
class DlgDsSearch : public QDialog, public Ui::DsSearch {
	Q_OBJECT
public:
	/*!
	 * @brief Constructor.
	 */
	DlgDsSearch(const QString &userName, const QString &dbType,
	    bool dbEffectiveOVM, bool dbOpenAddressing,
	    QStringList *dbIdList = Q_NULLPTR, QWidget *parent = Q_NULLPTR);

private slots:
	/*!
	 * @brief Activates confirmation button.
	 */
	void enableOkButton(void);

	/*!
	 * @brief Set first column containing checkboxes active.
	 */
	void setFirtsColumnActive(void);

	/*!
	 * @brief Check input fields sanity and activate search button.
	 */
	void checkInputFields(void);

	/*!
	 * @brief Search for data boxes according given criteria.
	 */
	void searchDataBox(void);

	/*!
	 * @brief Makes a selection and closes the dialogue.
	 */
	void contactItemDoubleClicked(const QModelIndex &index);

	/*!
	 * @brief Appends selected box identifiers into identifier list.
	 */
	void addSelectedDbIDs(void);

	void pingIsdsServer(void);

private:
	void initSearchWindow(void);

	/*!
	 * @brief Encapsulates query.
	 *
	 * @param[in] boxId Data box identifier.
	 * @param[in] boxTye Type of sought data box.
	 * @param[in] ic Identifier number.
	 * @param[in] name Name to search for.
	 * @param[in] zipCode ZIP code.
	 */
	void queryBox(const QString &boxId,
	    enum TaskSearchOwner::BoxType boxType, const QString &ic,
	    const QString &name, const QString &zipCode);

	const QString m_userName; /*!< User name used for searching. */
	const QString m_dbType; /*!< Data box type used for searching.  */
	const bool m_dbEffectiveOVM;
	const bool m_dbOpenAddressing;

	QStringList *m_dbIdList; /*!< List of box identifiers to append to. */

	QTimer *pingTimer;
	bool m_showInfoLabel;
};

#endif /* DLG_DS_SEARCH_H */
