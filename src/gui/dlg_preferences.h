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

#ifndef _DLG_PREFERENCES_H_
#define _DLG_PREFERENCES_H_

#include <QDialog>
#include <QString>

namespace Ui {
	class DlgPreferences;
}

/*!
 * @brief Preferences dialogue.
 */
class DlgPreferences : public QDialog {
	Q_OBJECT

public:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] parent Parent widget.
	 */
	explicit DlgPreferences(QWidget *parent = Q_NULLPTR);

	/*!
	 * @brief Destructor.
	 */
	~DlgPreferences(void);

private slots:
	/*!
	 * @brief Enables background download timer settings.
	 *
	 * @param[in] checkState Background timer checkbox state.
	 */
	void activateBackgroundTimer(int checkState);

	/*!
	 * @brief Sets path for attachment saving.
	 */
	void setSavePath(void);

	/*!
	 * @brief Sets path for adding attachments.
	 */
	void setAddFilePath(void);

	/*!
	 * @brief Save dialogue content to settings.
	 */
	void saveSettings(void) const;

private:
	/*!
	 * @brief Initialises the dialogue according to settings.
	 */
	void initDialogue(void);

	static
	int getLangugeIndex(const QString &language);

	static
	const QString &getIndexFromLanguge(int index);

	Ui::DlgPreferences *m_ui; /*!< UI generated from UI file. */
};

#endif /* _DLG_PREFERENCES_H_ */
