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
#include <QString>

namespace Gov {
	class Service; /* Forward declaration. */
}

/*!
 * @brief Holds information about available gov services.
 */
class GovServiceListModel : public QAbstractListModel {
	Q_OBJECT

public:
	/*!
	 * @brief Model entry.
	 */
	class Entry {
	public:
		Entry(const Entry &sme);
		Entry(const QString &srvcInternId, const QString &srvcFullName,
		    const QString &instName, const QString &srvcBoxId);

		const QString &srvcInternId(void) const;
		const QString &srvcFullName(void) const;
		const QString &instName(void) const;
		const QString &srvcBoxId(void) const;

	private:
		QString m_srvcInternId; /*!< Short unique internal gov service identifier. */
		QString m_srvcFullName; /*!< Gov service full name. */
		QString m_instName; /*!< Gov institute name. */
		QString m_srvcBoxId; /*!< Gov institute databox ID */
	};

	/*!
	 * @brief Roles which this model supports.
	 */
	enum Roles {
		ROLE_GOV_SRVC_INTERN_ID = Qt::UserRole,
		ROLE_GOV_SRVC_FULL_NAME,
		ROLE_GOV_SRVC_INST_NAME,
		ROLE_GOV_SRVC_BOXID
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] parent Pointer to parent object.
	 */
	explicit GovServiceListModel(QObject *parent = Q_NULLPTR);

	/*!
	 * @brief Copy constructor.
	 *
	 * @note Needed for QVariant conversion.
	 *
	 * @param[in] model Model to be copied.
	 * @param[in] parent Pointer to parent object.
	 */
	explicit GovServiceListModel(const GovServiceListModel &model,
	    QObject *parent = Q_NULLPTR);

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
	 * @brief Returns the model's role names.
	 *
	 * @return Model's role names.
	 */
	virtual
	QHash<int, QByteArray> roleNames(void) const Q_DECL_OVERRIDE;

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
	 * @brief Return list of all entries.
	 *
	 * @return List of all entries.
	 */
	const QList<Entry> &allEntries(void) const;

	/*!
	 * @brief Returns item flags for given index.
	 *
	 * @brief[in] index Index specifying the item.
	 * @return Item flags.
	 */
	virtual
	Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;

	/*!
	 * @brief Appends service to model.
	 *
	 * @param[in] gg E-gov service data.
	 */
	void appendService(const Gov::Service *gs);

	/*!
	 * @brief Clears the model.
	 */
	void clearAll(void);

private:
	QList<Entry> m_services; /*!< List of service entries. */
};
