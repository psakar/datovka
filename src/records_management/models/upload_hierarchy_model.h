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

#ifndef _UPLOAD_HIERARCHY_MODEL_H_
#define _UPLOAD_HIERARCHY_MODEL_H_

#include <QAbstractItemModel>

#include "src/records_management/json/upload_hierarchy.h"

/*!
 * @brief Upload hierarchy model.
 */
class UploadHierarchyModel : public QAbstractItemModel {
	Q_OBJECT
public:
	/*!
	 * @brief Custom roles.
	 */
	enum CustomRoles {
		ROLE_FILTER = (Qt::UserRole + 1), /* Exposes metadata for filtering. */
		ROLE_ID /* Hierarchy entry identifier. */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] parent Pointer to parent object.
	 */
	explicit UploadHierarchyModel(QObject *parent = Q_NULLPTR);

	/*!
	 * @brief Return index specified by supplied parameters.
	 *
	 * @param[in] row    Item row.
	 * @param[in] column Parent column.
	 * @param[in] parent Parent index.
	 * @return Index to desired element or invalid index on error.
	 */
	virtual
	QModelIndex index(int row, int column,
	    const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

	/*!
	 * @brief Return parent index of the item with the given index.
	 *
	 * @param[in] index Child node index.
	 * @return Index of the parent node or invalid index on error.
	 */
	virtual
	QModelIndex parent(const QModelIndex &index) const Q_DECL_OVERRIDE;

	/*!
	 * @brief Return number of rows under the given parent.
	 *
	 * @param[in] parent Parent node index.
	 * @return Number of rows.
	 */
	virtual
	int rowCount(const QModelIndex &parent = QModelIndex()) const
	    Q_DECL_OVERRIDE;

	/*!
	 * @brief Return the number of columns for the children of given parent.
	 *
	 * @param[in] parent Parent node index.
	 * @return Number of columns.
	 */
	virtual
	int columnCount(const QModelIndex &parent = QModelIndex()) const
	    Q_DECL_OVERRIDE;

	/*!
	 * @brief Return data stored in given location under given role.
	 *
	 * @param[in] index Index specifying the item.
	 * @param[in] role  Data role.
	 * @return Data from model.
	 */
	virtual
	QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;

	/*!
	 * @brief Returns header data in given location under given role.
	 *
	 * @brief[in] section     Header position.
	 * @brief[in] orientation Header orientation.
	 * @brief[in] role        Data role.
	 * @return Header data from model.
	 */
	virtual
	QVariant headerData(int section, Qt::Orientation orientation,
	    int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

	/*!
	 * @brief Returns item flags for given index.
	 *
	 * @brief[in] index Index specifying the item.
	 * @return Item flags.
	 */
	virtual
	Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;

	/*!
	 * @brief Set content data.
	 *
	 * @param[in] uhr Upload hierarchy response structure.
	 */
	void setHierarchy(const UploadHierarchyResp &uhr);

private:
	/*!
	 * @brief Check whether to show root name.
	 *
	 * @return True in root node contains a name.
	 */
	bool showRootName(void) const;

	/*!
	 * @brief Return all data related to node which can be used for
	 *     filtering.
	 *
	 * @param[in] entry Node identifier.
	 * @return List of strings.
	 */
	static
	QStringList filterData(const UploadHierarchyResp::NodeEntry *entry);

	/*!
	 * @brief Returns list of all (meta)data (including children).
	 *
	 * @param[in] entry Node identifying the root.
	 * @param[in] takeSuper Set true when data of superordinate node should
	 *                      be taken into account.
	 * @return List of all gathered data according to which can be filtered.
	 */
	static
	QStringList filterDataRecursive(
	    const UploadHierarchyResp::NodeEntry *entry, bool takeSuper);

	UploadHierarchyResp m_hierarchy; /*!< Upload hierarchy structure. */
};

#endif /* _UPLOAD_HIERARCHY_MODEL_H_ */
