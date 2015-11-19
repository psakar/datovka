/*
 * Copyright (C) 2014-2015 CZ.NIC
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

#ifndef _ATTACHMENT_TABLE_VIEW_H_
#define _ATTACHMENT_TABLE_VIEW_H_

#include <QList>
#include <QMouseEvent>
#include <QString>
#include <QTableView>
#include <QTemporaryDir>
#include <QUrl>

/*!
 * @brief Custom attachment table view class with dragging enabled.
 */
class AttachmentTableView : public QTableView {
	Q_OBJECT

public:
	/*!
	 * @brief Constructor.
	 */
	AttachmentTableView(QWidget *parent = 0);

protected:
	/*!
	 * @brief Activates the drag.
	 */
	virtual
	void mouseMoveEvent(QMouseEvent *event);

	/*
	 * @brief Used for storing the beginning position of the drag.
	 */
	virtual
	void mousePressEvent(QMouseEvent *event);

private:
	/*!
	 * @brief Creates temporary files related to selected view items.
	 *
	 * @param[in] tmpDir Temporary directory object.
	 * @return List of absolute file names or empty list on error.
	 */
	QList<QString> temporaryFiles(const QTemporaryDir &tmpDir) const;

	/*!
	 * @brief Converts list of absolute file names to list of URLs.
	 *
	 * @param[in] tmpFileNames List of absolute file names.
	 * @return List of URLs or empty list on error.
	 */
	static
	QList<QUrl> temporaryFileUrls(const QList<QString> &tmpFileNames);

	QPoint m_dragStartPosition /*!< Holds the starting drag point. */;
};

#endif /* _ATTACHMENT_TABLE_VIEW_H_ */
