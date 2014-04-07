

#ifndef _COMMON_H_
#define _COMMON_H_


#include <QSettings>
#include <QString>
#include <QDebug>

#define ICON_16x16_PATH ":/icons/16x16/"
#define ICON_24x24_PATH ":/icons/24x24/"
#define ICON_128x128_PATH ":/icons/128x128/"
#define ICON_3PARTY_PATH ":/icons/3party/"
#define VERSION "0.1"


class GlobPreferences {

public:
	typedef enum {
		DOWNLOAD_DATE = 1,
		CURRENT_DATE = 2
	} CertValDate;

	typedef enum {
		DATE_FORMAT_LOCALE = 1,
		DATE_FORMAT_ISO = 2,
		DATE_FORMAT_DEFAULT = 3//,
		//DATE_FORMAT_CUSTOM = 4
	} DateFmt;

	typedef enum {
		SELECT_NEWEST = 1,
		SELECT_LAST_VISITED = 2,
		SELECT_NOTHING = 3
	} SelectType;

	GlobPreferences(void);
	~GlobPreferences(void);

	bool auto_download_whole_messages;
	bool default_download_signed; /*!< Default downloading method. */
	//bool store_passwords_on_disk;
	bool store_messages_on_disk;
	bool store_additional_data_on_disk;
	CertValDate certificate_validation_date;
	bool check_crl;
	bool check_new_versions;
	bool send_stats_with_version_checks;
	DateFmt date_format;
	QString language;
	SelectType after_start_select;

	/*!
	 * @brief Load data from supplied settings.
	 */
	void loadFromSettings(const QSettings &settings);

	/*!
	 * @brief Store data to settings structure.
	 */
	void saveToSettings(QSettings &settings) const;
};


class ProxySettings {

public:
	QString https_proxy;
	QString http_proxy;
};


/* Global preferences structure. */
extern GlobPreferences globPref;
extern ProxySettings globProxSet;


#endif /* _COMMON_H_ */
