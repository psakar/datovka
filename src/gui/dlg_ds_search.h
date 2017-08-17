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
#include <QItemSelection>
#include <QThread>

#include "src/models/combo_box_model.h"
#include "src/models/data_box_contacts_model.h"
#include "src/worker/task_search_owner.h"
#include "src/worker/task_search_owner_fulltext.h"

class DlgDsSearch; /* Forward declaration. */

/*!
 * @brief Thread performing full-text data box searching.
 */
class FulltextSearchThread : public QThread {

public:
	/*!
	 * @brief Constructor.
	 */
	explicit FulltextSearchThread(DlgDsSearch *dlg);

protected:
	/*!
	 * @brief Thread function.
	 */
	virtual
	void run(void) Q_DECL_OVERRIDE;

private:
	DlgDsSearch *m_dlg; /*!< Pointer to data box search window. */
};

namespace Ui {
	class DlgDsSearch;
}

/*!
 * @brief Data box search dialogue.
 */
class DlgDsSearch : public QDialog {
	Q_OBJECT
private:
	/*!
	 * @brief Constructor.
	 */
	DlgDsSearch(const QString &userName, const QString &dbType,
	    bool dbEffectiveOVM, bool dbOpenAddressing,
	    QStringList *dbIdList = Q_NULLPTR, QWidget *parent = Q_NULLPTR);

public:
	/*!
	 * @brief Destructor.
	 */
	virtual
	~DlgDsSearch(void);

	/*!
	 * @brief Show data box search dialogue.
	 */
	static
	void search(const QString &userName, const QString &dbType,
	    bool dbEffectiveOVM, bool dbOpenAddressing,
	    QStringList *dbIdList = Q_NULLPTR, QWidget *parent = Q_NULLPTR);

private slots:
	/*!
	 * @brief Activates confirmation button.
	 */
	void enableOkButton(void);

	/*!
	 * @brief Set first column containing checkboxes active.
	 *
	 * @param[in] selected Newly selected items.
	 * @param[in] deselected Deselected items.
	 */
	void setFirstColumnActive(const QItemSelection &selected,
	    const QItemSelection &deselected);

	/*!
	 * @brief Check input fields sanity and activate search button.
	 */
	void checkInputFields(void);

	/*!
	 * @brief Enables search controls.
	 */
	void enableSearchControls(void);

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

	/*!
	 * @brief Signals breaking of download loop.
	 */
	void setBreakDownloadLoop(void);

	/*!
	 * @brief Displays elements relevant for normal or full-text search.
	 *
	 * @param[in] fulltextCheckState STate of full-text checkbox state.
	 */
	void makeSearchElelementsVisible(int fulltextState);

private:
	/*!
	 * @brief Initialise dialogue content.
	 */
	void initContent(void);

	/*!
	 * @brief Check input field sanity for normal search.
	 */
	void checkInputFieldsNormal(void);

	/*!
	 * @brief Check input field sanity for full-text search.
	 */
	void checkInputFieldsFulltext(void);

	/*!
	 * @brief Normal search for data boxes according given criteria.
	 */
	void searchDataBoxNormal(void);

	/*!
	 * @brief Full-text search for data boxes according given criteria.
	 */
	void searchDataBoxFulltext(void);

	/*!
	 * @brief Full-text search for data boxes according given criteria
	 *     run in background.
	 */
	void searchDataBoxFulltextThread(void);

	/*!
	 * @brief Encapsulates query.
	 *
	 * @param[in] boxId Data box identifier.
	 * @param[in] boxTye Type of sought data box.
	 * @param[in] ic Identifier number.
	 * @param[in] name Name to search for.
	 * @param[in] zipCode ZIP code.
	 */
	void queryBoxNormal(const QString &boxId,
	    enum TaskSearchOwner::BoxType boxType, const QString &ic,
	    const QString &name, const QString &zipCode);

	/*!
	 * @brief Encapsulates full-text query for a single page.
	 *
	 * @param[in] target Which field to search in.
	 * @param[in] boxType Type of data box to search for.
	 * @param[in] phrase Text phrase to search for.
	 * @param[in] pageNum Page umber to ask for.
	 * @return False on error or when last page downloaded.
	 */
	bool queryBoxFulltextPage(
	    enum TaskSearchOwnerFulltext::FulltextTarget target,
	    enum TaskSearchOwnerFulltext::BoxType boxType,
	    const QString &phrase, qint64 pageNum);

	/*!
	 * @brief Encapsulated full-text query for all entries.
	 *
	 * @param[in] target Which field to search in.
	 * @param[in] boxType Type of data box to search for.
	 * @param[in] phrase Text phrase to search for.
	 */
	void queryBoxFulltextAll(
	    enum TaskSearchOwnerFulltext::FulltextTarget target,
	    enum TaskSearchOwnerFulltext::BoxType boxType,
	    const QString &phrase);

	/*!
	 * @brief Returns total found string.
	 *
	 * @param[in] total Total found number.
	 * @return String containing total found summary.
	 */
	static
	QString totalFoundStr(int total);

	friend class FulltextSearchThread;

	Ui::DlgDsSearch *m_ui; /*!< UI generated from UI file. */

	FulltextSearchThread m_fulltextThread; /*!< Tread that;s waiting for incoming full-text search result. */

	const QString m_userName; /*!< User name used for searching. */
	const QString m_dbType; /*!< Data box type used for searching.  */
	const bool m_dbEffectiveOVM;
	const bool m_dbOpenAddressing;

	BoxContactsModel m_contactTableModel; /*!< Model of found data boxes. */
	CBoxModel m_boxTypeCBoxModel; /*!< Data box type combo box model. */
	CBoxModel m_fulltextCBoxModel; /*!< Full-text combo box model. */

	QStringList *m_dbIdList; /*!< List of box identifiers to append to. */

	volatile bool m_breakDownloadLoop; /*!< Setting to true interrupts download loop. */

	bool m_showInfoLabel;
};

#endif /* DLG_DS_SEARCH_H */
