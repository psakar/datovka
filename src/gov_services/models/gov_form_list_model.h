/*
 * Copyright (C) 2014-2018 CZ.NIC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
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

#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>

#include "src/datovka_shared/gov_services/service/gov_service.h"
#include "src/datovka_shared/gov_services/service/gov_service_form_field.h"
#include "src/models/table_model.h"

/*!
 * @brief Holds form data for gov services.
 */
class GovFormListModel : public TblModel {
	Q_OBJECT

public:

	/*!
	 * @brief Identifies the column index.
	 */
	enum ColumnNumbers {
		LABEL_COL = 0, /* Label. */
		TEXT_EDIT_COL = 1, /* Column holds textedit. */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] parent Pointer to parent object.
	 */
	explicit GovFormListModel(QObject *parent = Q_NULLPTR);

	/*!
	 * @brief Copy constructor.
	 *
	 * @note Needed for QVariant conversion.
	 *
	 * @param[in] other Model to be copied.
	 * @param[in] parent Pointer to parent object.
	 */
	explicit GovFormListModel(const GovFormListModel &other,
	    QObject *parent = Q_NULLPTR);

	/*!
	 * @brief Destructor.
	 *
	 * @note Deletes held service.
	 */
	virtual
	~GovFormListModel(void);

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
	 * @brief Return data stored in given location under given role.
	 *
	 * @param[in] index Index specifying the item.
	 * @param[in] role  Data role.
	 * @return Data from model.
	 */
	virtual
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole)
	    const Q_DECL_OVERRIDE;

	/*!
	 * @brief Returns item flags for given index.
	 *
	 * @brief[in] index Index specifying the item.
	 * @return Item flags.
	 */
	virtual
	Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;

	/*!
	 * @brief Returns pointer to currently held service.
	 *
	 * @return Pointer to currently associated service.
	 */
	const Gov::Service *service(void) const;

	/*!
	 * @brief Associate a service. The model takes ownership of the service object.
	 *
	 * @note The associated service is deleted in model destruction. It is
	 *     on the caller to delete the old service object.
	 *
	 * @param[in] service New service to be associated with the model.
	 * @return Old associated service.
	 */
	Gov::Service *setService(Gov::Service *service);

	/*!
	 * @brief Used for changing the check state.
	 *
	 * @note Emits dataChanged signal.
	 *
	 * @param[in] index Index specifying the element.
	 * @param[in] value Value to be set.
	 * @param[in] role Specifies role of the modified data.
	 * @return True if check state was changed.
	 */
	virtual
	bool setData(const QModelIndex &index, const QVariant &value,
	    int role = Qt::EditRole) Q_DECL_OVERRIDE;

	/*!
	 * @brief Return true if all mandatory data are set.
	 *
	 * @return True if all mandatory values are set.
	 */
	bool haveAllMandatory(void) const;

	/*!
	 * @brief Return true if model contains mandatory data that must be
	 *     provided by the user.
	 *
	 * @return True if at least one such field is in the model.
	 */
	bool containsMandatoryUser(void) const;

	/*!
	 * @brief Return true if model contains data that have been acquired
	 *     from the data box.
	 *
	 * @return true if at least on such field is in the model.
	 */
	bool containsBoxOwnerData(void) const;

	/*!
	 * @brief Checks whether held date are valid.
	 *
	 * @note Emits validityNotification if a service is set.
	 *
	 * @return True on success.
	 */
	bool haveAllValid(void);

signals:
	/*!
	 * @brief Signal is emitted when validity check ends.
	 *
	 * @param[in] message Empty string on success, error description else.
	 */
	void validityNotification(const QString &message);

private:
	Gov::Service *m_service; /*!< Service data, the model is the owner. */
};
