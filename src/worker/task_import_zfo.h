/*
 * Copyright (C) 2014-2016 CZ.NIC
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

#ifndef _TASK_IMPORT_ZFO_H_
#define _TASK_IMPORT_ZFO_H_

#include <QString>

#include "src/worker/task.h"

/*!
 * @brief Task describing ZFO file import.
 */
class TaskImportZfo : public Task {
public:
	/*!
	 * @brief ZFO type.
	 */
	enum ZfoType {
		ZT_UKNOWN = 0, /*!< Unknown format. */
		ZT_MESSAGE = 1, /*!< ZFO holds message. */
		ZT_DELIVERY_INFO = 2 /*!< ZFO holds delivery information. */
	};

	/*!
	 * @brief Holds information about accounts that should be processed.
	 */
	class AccountData {
	public:
		/*!
		 * @brief Constructors.
		 */
//		AccountData(void)
//		    : userName(), messageDbSet(0)
//		{ }
		AccountData(const QString &uN, class MessageDbSet *mDS)
		    : userName(uN), messageDbSet(mDS)
		{ }

		QString userName; /*!< Account identifier (user name). */
		class MessageDbSet *messageDbSet; /*!< Database set related to account. */
	};

	/*!
	 * @brief Constructor.
	 *
	 * @param[in] userName Account identifier (user login name).
	 * @param[in] fileName Full path to ZFO file.
	 */
	explicit TaskImportZfo(const QString &userName,
	    const QString &fileName);

	/*!
	 * @brief Performs action.
	 */
	virtual
	void run(void);

	/*!
	 * @brief Determines the type of the ZFO file.
	 *
	 * @param[in] fileName Full path to ZFO file.
	 * @return Type of ZFO file. Returns unknown format on all errors.
	 */
	static
	enum ZfoType determineFileType(const QString &fileName);

private:
	const QString m_userName; /*!< Account identifier (user login name). */
	const QString m_fileName; /*!< Full file path to imported file. */
};

#endif /* _TASK_IMPORT_ZFO_H_ */
