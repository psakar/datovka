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

#ifndef _PREFERENCES_H_
#define _PREFERENCES_H_

#include <QSettings>
#include <QString>

class Preferences {

public:
	enum CertValDate {
		DOWNLOAD_DATE = 1,
		CURRENT_DATE = 2
	};

	enum DateFmt {
		DATE_FORMAT_LOCALE = 1,
		DATE_FORMAT_ISO = 2,
		DATE_FORMAT_DEFAULT = 3//,
		//DATE_FORMAT_CUSTOM = 4
	};

	enum SelectType {
		SELECT_NEWEST = 1,
		SELECT_LAST_VISITED = 2,
		SELECT_NOTHING = 3
	};

	/*!
	 * @brief Constructor.
	 */
	Preferences(void);

	/* Set via command line. */
	QString confSubdir; /*!< Configuration directory. */
	QString loadFromConf; /*!< Configuration file to load from. */
	QString saveToConf; /*!< Configuration file to save to. */

	/* Fixed settings. */
	const QString acntDbFile; /*!< Account db file. */
	const QString tagDbFile; /*!< Tag db file. */
	const QString recMgmtDbFile; /*!< Records management db file. */

	bool autoDownloadWholeMessages;
	bool defaultDownloadSigned; /*!<
	                             * Default downloading method (true if
	                             * signed messages should be downloaded).
	                             * This value is unused. TODO -- unused.
	                             */
	//bool storePasswordsOnDisk;
	bool storeMessagesOnDisk;
	bool storeAdditionalDataOnDisk; /*!< Store account information to disk. */
	bool downloadOnBackground;
	int timerValue; /*!< Interval in minutes to check for new messages. */
	bool downloadAtStart;
	bool checkNewVersions;
	bool sendStatsWithVersionChecks;

	int isdsDownloadTimeoutMs;
	int messageMarkAsReadTimeout;

	enum CertValDate certificateValidationDate;
	bool checkCrl;
	int timestampExpirBeforeDays;

	enum SelectType afterStartSelect;

	int toolbarButtonStyle;

	bool useGlobalPaths;
	QString saveAttachmentsPath;
	QString addFileToAttachmentsPath;

	bool allAttachmentsSaveZfoMsg;
	bool allAttachmentsSaveZfoDelinfo;
	bool allAttachmentsSavePdfMsgenvel;
	bool allAttachmentsSavePdfDelinfo;
	QString messageFilenameFormat;
	QString deliveryFilenameFormat;
	QString attachmentFilenameFormat;
	QString deliveryFilenameFormatAllAttach;
	bool deliveryInfoForEveryFile;

	QString language;

	enum DateFmt dateFormat; /*!< TODO -- unused. */

	/*!
	 * @brief Create configuration file if not present.
	 *
	 * @note Location of the configuration file is taken from this
	 *     preferences structure.
	 */
	bool ensureConfPresence(void) const;

	/*!
	 * @brief Load data from supplied settings.
	 *
	 * @param[in] settings Settings structure.
	 */
	void loadFromSettings(const QSettings &settings);

	/*!
	 * @brief Store data to settings structure.
	 *
	 * @param[out] settings Settings structure.
	 */
	void saveToSettings(QSettings &settings) const;

	/*!
	 * @brief Return path to configuration directory.
	 *
	 * @return Path to configuration directory.
	 */
	QString confDir(void) const;

	/*!
	 * @brief Returns whole configuration file path.
	 *
	 * @return Whole path to loading configuration file.
	 */
	QString loadConfPath(void) const;

	/*!
	 * @brief Returns whole configuration file path.
	 *
	 * @return Whole path to saving configuration file.
	 */
	QString saveConfPath(void) const;

	/*!
	 * @brief Returns whole account db path.
	 *
	 * @return Whole path to account database file.
	 */
	QString acntDbPath(void) const;

	/*!
	 * @brief Returns whole tag db path.
	 *
	 * @return Whole path to tag database file.
	 */
	QString tagDbPath(void) const;

	/*!
	 * @brief Returns whole records management db path.
	 *
	 * @return Whole path to records management database file.
	 */
	QString recMgmtDbPath(void) const;
};

#endif /* _PREFERENCES_H_ */
