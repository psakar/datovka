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

#ifndef _ATTACHMENT_TABLE_WIDGET_H_
#define _ATTACHMENT_TABLE_WIDGET_H_

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QList>
#include <QString>
#include <QUrl>

#include "src/views/lowered_table_widget.h"

/*
 * Column indexes into attachment table widget.
 */
#define ATW_FILE 0
#define ATW_TYPE 1
#define ATW_MIME 2
#define ATW_SIZE 3
#define ATW_PATH 4
#define ATW_DATA 5 /* Base64 encoded and hidden. */

/*!
 * @brief Custom attachment table widget class with dropping enabled.
 */
class AttachmentTableWidget : public LoweredTableWidget {
	Q_OBJECT

public:
	/*!
	 * @brief Constructor.
	 */
	AttachmentTableWidget(QWidget *parent = 0);

	/*!
	 * @brief Adds a file of not already present.
	 *
	 * @return File size or -1 on error.
	 */
	int addFile(const QString &filePath);

protected:
	/*!
	 * @brief Allows the drop.
	 */
	virtual
	void dragEnterEvent(QDragEnterEvent *event) Q_DECL_OVERRIDE;

	/*!
	 * @brief Allow drag move.
	 */
	virtual
	void dragMoveEvent(QDragMoveEvent *event) Q_DECL_OVERRIDE;

	/*!
	 * @brief Processes the drop.
	 */
	virtual
	void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;

private:
	/*!
	 * @brief Convert a list of URLs to a list of absolute file paths.
	 *
	 * @param[in] uriList List of file URLs.
	 * @return List of absolute file paths or empty list on error.
	 */
	static
	QList<QString> filePaths(const QList<QUrl> &uriList);

	/*!
	 * @brief Read file content and encode it into base64.
	 *
	 * @param[in] filePath Path to file.
	 * @return Base64-encoded file content.
	 */
	static
	QByteArray getFileBase64(const QString &filePath);

	/* TODO -- File size counter. */
};

#endif /* _ATTACHMENT_TABLE_WIDGET_H_ */
