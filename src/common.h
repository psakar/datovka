

#ifndef _COMMON_H_
#define _COMMON_H_


#include <QString>
#include <QDebug>

#define ICON_16x16_PATH ":/icons/16x16/"
#define ICON_24x24_PATH ":/icons/24x24/"
#define ICON_128x128_PATH ":/icons/128x128/"
#define ICON_3PARTY_PATH ":/icons/3party/"
#define VERSION "0.1"


class Preferences {

public:
	bool store_messages_on_disk;
	int date_format;	//1
	bool default_download_signed;
	bool check_crl;
	QString language;  //system/en/cz
	bool check_new_versions;
	bool store_additional_data_on_disk;
	bool send_stats_with_version_checks;
	int certificate_validation_date; //1
	int after_start_select; //1
	bool auto_download_whole_messages;
};

class ProxySettings {


public:
	int https_proxy;
	int http_proxy;
};


extern Preferences globPref;
extern ProxySettings globProxSet;


#endif /* _COMMON_H_ */
