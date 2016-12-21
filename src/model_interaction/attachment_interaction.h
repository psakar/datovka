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

#ifndef _ATTACHMENT_INTERACTION_H_
#define _ATTACHMENT_INTERACTION_H_

#include <QCoreApplication> /* Q_DECLARE_TR_FUNCTIONS */
#include <QWidget>

#include "src/views/attachment_table_view.h"

/*!
 * @brief Provides a namespace for convenience functions dealing with the
 *     attachment model.
 *
 * @note These functions are not part of the attachment model code because
 *     mostly they require some sort of QUI interaction and require Qt widgets
 *     to be compiled.
 */
class AttachmentInteraction {
	Q_DECLARE_TR_FUNCTIONS(AttachmentInteraction)

private:
	/*!
	 * @brief Private constructor.
	 */
	AttachmentInteraction(void);

public:
	/*!
	 * @brief Open attachment file in associated application.
	 *
	 * @param[in,out] parent Parent widget to call  dialogues from.
	 * @param[in]     view Table view to determine selection from.
	 * @param[in]     index Selection index, if invalid then selection is
	 *                      determined.
	 * @param[out]    attName Set to attachment name if can be determined.
	 * @param[out]    tmpPath Set to temporary file path if can be
	 *                        determined.
	 * @return True on success, false else.
	 */
	static
	bool openAttachment(QWidget *parent, const AttachmentTableView &view,
	    QModelIndex index = QModelIndex(), QString *attName = Q_NULLPTR,
	    QString *tmpPath = Q_NULLPTR);
};

#endif /* _ATTACHMENT_INTERACTION_H_ */
