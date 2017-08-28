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
#include "src/models/sort_filter_proxy_model.h"
#include "src/worker/task_search_owner.h"
#include "src/worker/task_search_owner_fulltext.h"

class DlgDsSearch; /* Forward declaration. */

/*!
 * @brief Thread performing full-text data box searching.
 */
class FulltextSearchThread : public QThread {
	Q_OBJECT

public:
	/*!
	 * @brief Constructor.
	 */
	explicit FulltextSearchThread(DlgDsSearch *dlg);

signals:
	/*!
	 * @brief Emitted when search finishes.
	 *
	 * @param[in] result Task result code.
	 * @param[in] longErrMsg Long error message.
	 */
	void searchFinished(int result, const QString &longErrMsg);

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
	 *
	 * @return List of data-box identifiers.
	 */
	static
	QStringList search(const QString &userName, const QString &dbType,
	    bool dbEffectiveOVM, bool dbOpenAddressing,
	    QWidget *parent = Q_NULLPTR);

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
	 * @brief Run on search thread exit.
	 *
	 * @param[in] result Full-text task result code.
	 * @param[in] longErrMsg Long error message.
	 */
	void collectFulltextThreadOutcome(int result,
	    const QString &longErrMsg);

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

	/*!
	 * @brief Apply filter text on the table.
	 */
	void filterContact(const QString &text);

private:
	/*!
	 * @brief Normal search result.
	 */
	class SearchResult {
	public:
		/*!
		 * @brief Constructor.
		 */
		SearchResult(enum TaskSearchOwner::Result r,
		    const QString &lErr)
		    : result(r), longErrMsg(lErr)
		{
		}

		enum TaskSearchOwner::Result result;
		QString longErrMsg;
	};

	/*!
	 * @brief Full-text search result.
	 */
	class SearchResultFt {
	public:
		/*!
		 * @brief Constructor.
		 */
		SearchResultFt(enum TaskSearchOwnerFulltext::Result r,
		    const QString &lErr, bool lp)
		    : result(r), longErrMsg(lErr), lastPage(lp)
		{
		}

		enum TaskSearchOwnerFulltext::Result result;
		QString longErrMsg;
		bool lastPage; /*!< True if last page acquired. */
	};

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
	SearchResult searchDataBoxNormal(void);

	/*!
	 * @brief Full-text search for data boxes according given criteria.
	 */
	SearchResultFt searchDataBoxFulltext(void);

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
	 * @return Search result.
	 */
	SearchResult queryBoxNormal(const QString &boxId,
	    enum TaskSearchOwner::BoxType boxType, const QString &ic,
	    const QString &name, const QString &zipCode);

	/*!
	 * @brief Display message about search result.
	 *
	 * @param[in] searchResult Search task result code.
	 */
	void displaySearchResult(const SearchResult &searchResult);

	/*!
	 * @brief Encapsulates full-text query for a single page.
	 *
	 * @param[in] target Which field to search in.
	 * @param[in] boxType Type of data box to search for.
	 * @param[in] phrase Text phrase to search for.
	 * @param[in] pageNum Page umber to ask for.
	 * @return Search result.
	 */
	SearchResultFt queryBoxFulltextPage(
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
	SearchResultFt queryBoxFulltextAll(
	    enum TaskSearchOwnerFulltext::FulltextTarget target,
	    enum TaskSearchOwnerFulltext::BoxType boxType,
	    const QString &phrase);

	/*!
	 * @brief Display message about full-text search result.
	 *
	 * @param[in] result Full-text task result code.
	 * @param[in] longErrMsg Long error message.
	 */
	void displaySearchResultFt(int result, const QString &longErrMsg);

	/*!
	 * @brief Returns total found string.
	 *
	 * @param[in] total Total found number.
	 * @return String containing total found summary.
	 */
	static
	QString totalFoundStr(int total);

	/* Allow access from search thread to dialogue controls. */
	friend class FulltextSearchThread;

	Ui::DlgDsSearch *m_ui; /*!< UI generated from UI file. */

	FulltextSearchThread m_fulltextThread; /*!< Tread that's waiting for incoming full-text search result. */

	const QString m_userName; /*!< User name used for searching. */
	const QString m_dbType; /*!< Data box type used for searching.  */
	const bool m_dbEffectiveOVM; /*!< True if box has OVM status. */
	const bool m_dbOpenAddressing; /*!< True if open addressing is enabled. */

	SortFilterProxyModel m_contactListProxyModel; /*!<
	                                               * Used for message
	                                               * sorting and filtering.
	                                               */
	BoxContactsModel m_contactTableModel; /*!< Model of found data boxes. */
	CBoxModel m_boxTypeCBoxModel; /*!< Data box type combo box model. */
	CBoxModel m_fulltextCBoxModel; /*!< Full-text combo box model. */

	QStringList *m_dbIdList; /*!< List of box identifiers to append to. */

	volatile bool m_breakDownloadLoop; /*!< Setting to true interrupts download loop. */

	bool m_showInfoLabel; /*!< Controls the notification about limited search results. */
};

#endif /* DLG_DS_SEARCH_H */
