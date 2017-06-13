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

#ifndef _PREFERENCES_H_
#define _PREFERENCES_H_

#include <QSettings>
#include <QString>

class GlobPreferences {

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
	GlobPreferences(void);

	/*!
	 * @brief Destructor.
	 */
	~GlobPreferences(void);

	QString confSubdir; /*!< Configuration directory. */
	QString loadFromConf; /*!< Configuration file to load from. */
	QString saveToConf; /*!< Configuration file to save to. */
	const QString accountDbFile; /*!< Account db file. */
	const QString tagDbFile; /*!< Tag db file. */
	const QString documentServiceDbFile; /*!< Document service db file. */
	const QString tagWebDatovkaDbFile;  /*!< Tag webdatovka db file. */
	bool auto_download_whole_messages;
	bool default_download_signed; /*!< Default downloading method. */
	//bool store_passwords_on_disk;
	bool store_messages_on_disk;
	int toolbar_button_style;
	bool store_additional_data_on_disk;
	enum CertValDate certificate_validation_date;
	bool check_crl;
	bool check_new_versions;
	bool send_stats_with_version_checks;
	bool download_on_background;
	int timer_value;
	bool download_at_start;
	enum DateFmt date_format;
	QString language;
	enum SelectType after_start_select;
	int message_mark_as_read_timeout;
	bool use_global_paths;
	QString save_attachments_path;
	QString add_file_to_attachments_path;
	bool all_attachments_save_zfo_delinfo;
	bool all_attachments_save_zfo_msg;
	bool all_attachments_save_pdf_msgenvel;
	bool all_attachments_save_pdf_delinfo;
	QString message_filename_format;
	QString delivery_filename_format;
	QString attachment_filename_format;
	QString delivery_filename_format_all_attach;
	bool delivery_info_for_every_file;
	int isds_download_timeout_ms;
	int timestamp_expir_before_days;

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
	QString accountDbPath(void) const;

	/*!
	 * @brief Returns whole tag db path.
	 *
	 * @return Whole path to tag database file.
	 */
	QString tagDbPath(void) const;

	/*!
	 * @brief Returns whole document service db path.
	 *
	 * @return Whole path to document service database file.
	 */
	QString documentServiceDbPath(void) const;

	/*!
	 * @brief Returns whole webdatovka tag db path.
	 *
	 * @return Whole path to webdatovka tag database file.
	 */
	QString tagWebDatovkaDbPath(void) const;
};

/*!
 * @brief Global instance of the structure.
 */
extern GlobPreferences globPref;

#endif /* _PREFERENCES_H_ */
