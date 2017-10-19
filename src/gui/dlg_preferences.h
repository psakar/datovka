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

#include "src/settings/pin.h"
#include "src/settings/preferences.h"

namespace Ui {
	class DlgPreferences;
}

/*!
 * @brief Preferences dialogue.
 */
class DlgPreferences : public QDialog {
	Q_OBJECT

private:
	/*!
	 * @brief Constructor.
	 *
	 * @param[in] prefs Preferences according to which to set the dialogue.
	 * @param[in] pinSett PIN settings to use to set dialogue.
	 * @param[in] parent Parent widget.
	 */
	explicit DlgPreferences(const GlobPreferences &prefs,
	    const PinSettings &pinSett, QWidget *parent = Q_NULLPTR);

public:
	/*!
	 * @brief Destructor.
	 */
	~DlgPreferences(void);

	/*!
	 * @brief Modifies global preferences.
	 *
	 * @param[in,out] prefs Preferences to be modified.
	 * @param[in,out] pinSett PIN settings to be modified.
	 * @param[in]     parent Parent widget.
	 * @return True when dialogue has been accepted and preference data
	 *     have been updated.
	 */
	static
	bool modify(GlobPreferences &prefs, PinSettings &pinSett,
	    QWidget *parent = Q_NULLPTR);

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
	 * @brief Set PIN value.
	 */
	void setPin(void);

	/*!
	 * @brief Change PIN value.
	 */
	void changePin(void);

	/*!
	 * @brief Clear PIN value.
	 */
	void clearPin(void);

private:
	/*!
	 * @brief Save dialogue content to settings.
	 *
	 * @param[out] prefs Preferences to be modified.
	 * @param[out] pinSett PIN settings to be modified.
	 */
	void saveSettings(GlobPreferences &prefs, PinSettings &pinSett) const;

	/*!
	 * @brief Initialises the dialogue according to settings.
	 *
	 * @param[in] prefs Preferences according to which to set the dialogue.
	 */
	void initDialogue(const GlobPreferences &prefs);

	/*!
	 * @brief Set PIN button activity.
	 *
	 * @param[in] pinSett PIN settings.
	 */
	void activatePinButtons(const PinSettings &pinSett);

	Ui::DlgPreferences *m_ui; /*!< UI generated from UI file. */

	PinSettings m_pinSett; /*!< PIN settings copy. */
};

#endif /* _DLG_PREFERENCES_H_ */
