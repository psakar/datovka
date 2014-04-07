

#include "common.h"


GlobPreferences globPref;
GlobProxySettings globProxSet;


/* Defaults. */
static const
GlobPreferences dlftlGlobPref; /* Defaults. */
static const
GlobProxySettings dfltGlobProxSet;


/* ========================================================================= */
GlobPreferences::GlobPreferences(void)
/* ========================================================================= */
    : auto_download_whole_messages(false),
    default_download_signed(true),
    //store_passwords_on_disk(false),
    store_messages_on_disk(true),
    store_additional_data_on_disk(true),
    certificate_validation_date(DOWNLOAD_DATE),
    check_crl(true),
    check_new_versions(true),
    send_stats_with_version_checks(true),
    date_format(DATE_FORMAT_DEFAULT),
    language("system"), /* Use local settings. */
    after_start_select(SELECT_NEWEST)
{
}


/* ========================================================================= */
GlobPreferences::~GlobPreferences(void)
/* ========================================================================= */
{
}


/* ========================================================================= */
/*
 * Load data from supplied settings.
 */
void GlobPreferences::loadFromSettings(const QSettings &settings)
/* ========================================================================= */
{
	int value;

	auto_download_whole_messages = settings.value(
	    "preferences/auto_download_whole_messages",
	    dlftlGlobPref.auto_download_whole_messages).toBool();

	default_download_signed = settings.value(
	    "preferences/default_download_signed",
	    dlftlGlobPref.default_download_signed).toBool();

	store_messages_on_disk = settings.value(
	    "preferences/store_messages_on_disk",
	    dlftlGlobPref.store_messages_on_disk).toBool();

	store_additional_data_on_disk = settings.value(
	    "preferences/store_additional_data_on_disk",
	    dlftlGlobPref.store_additional_data_on_disk).toBool();

	value = settings.value("preferences/certificate_validation_date",
	    dlftlGlobPref.certificate_validation_date).toInt();
	switch (value) {
	case DOWNLOAD_DATE:
		certificate_validation_date = DOWNLOAD_DATE;
		break;
	case CURRENT_DATE:
		certificate_validation_date = CURRENT_DATE;
		break;
	default:
		certificate_validation_date =
		    dlftlGlobPref.certificate_validation_date;
		Q_ASSERT(0);
		break;
	}

	check_crl = settings.value("preferences/check_crl",
	    dlftlGlobPref.check_crl).toBool();

	check_new_versions = settings.value("preferences/check_new_versions",
	    dlftlGlobPref.check_new_versions).toBool();

	send_stats_with_version_checks = settings.value(
	    "preferences/send_stats_with_version_checks",
	    dlftlGlobPref.send_stats_with_version_checks).toBool();

	value = settings.value("preferences/date_format",
	    dlftlGlobPref.date_format).toInt();
	switch (value) {
	case DATE_FORMAT_LOCALE:
		date_format = DATE_FORMAT_LOCALE;
		break;
	case DATE_FORMAT_ISO:
		date_format = DATE_FORMAT_ISO;
		break;
	case DATE_FORMAT_DEFAULT:
		date_format = DATE_FORMAT_DEFAULT;
		break;
	default:
		date_format = dlftlGlobPref.date_format;
		Q_ASSERT(0);
		break;
	}

	language = settings.value("preferences/language",
	    dlftlGlobPref.language).toString();
	if (language.isEmpty()) {
		language = dlftlGlobPref.language;
	}

	value = settings.value("preferences/after_start_select",
	    dlftlGlobPref.after_start_select).toInt();
	switch (value) {
	case SELECT_NEWEST:
		after_start_select = SELECT_NEWEST;
		break;
	case SELECT_LAST_VISITED:
		after_start_select = SELECT_LAST_VISITED;
		break;
	case SELECT_NOTHING:
		after_start_select = SELECT_LAST_VISITED;
		break;
	default:
		after_start_select = dlftlGlobPref.after_start_select;
		Q_ASSERT(0);
		break;
	}
}


/* ========================================================================= */
/*
 * Store data to settings structure.
 */
void GlobPreferences::saveToSettings(QSettings &settings) const
/* ========================================================================= */
{
	settings.beginGroup("preferences");

	/* Only values differing from defaults are written. */

	if (dlftlGlobPref.auto_download_whole_messages !=
	    auto_download_whole_messages) {
		settings.setValue("auto_download_whole_messages",
		    auto_download_whole_messages);
	}

	if (dlftlGlobPref.default_download_signed != default_download_signed) {
		settings.setValue("default_download_signed",
		    default_download_signed);
	}

	if (dlftlGlobPref.store_messages_on_disk != store_messages_on_disk) {
		settings.setValue("store_messages_on_disk",
		    store_messages_on_disk);
	}

	if (dlftlGlobPref.store_additional_data_on_disk !=
	    store_additional_data_on_disk) {
		settings.setValue("store_additional_data_on_disk",
		    store_additional_data_on_disk);
	}

	if (dlftlGlobPref.certificate_validation_date !=
	    certificate_validation_date) {
		settings.setValue("certificate_validation_date",
		    certificate_validation_date);
	}

	if (dlftlGlobPref.check_crl != check_crl) {
		settings.setValue("check_crl", check_crl);
	}

	if (dlftlGlobPref.check_new_versions != check_new_versions) {
		settings.setValue("check_new_versions", check_new_versions);
	}

	if (dlftlGlobPref.send_stats_with_version_checks !=
	    send_stats_with_version_checks) {
		settings.setValue("send_stats_with_version_checks",
		    send_stats_with_version_checks);
	}

	if (dlftlGlobPref.date_format != date_format) {
		settings.setValue("date_format", date_format);
	}

	if (dlftlGlobPref.language != language) {
		settings.setValue("language", language);
	}

	if (dlftlGlobPref.after_start_select != after_start_select) {
		settings.setValue("after_start_select", after_start_select);
	}

	settings.endGroup();
}


/* ========================================================================= */
GlobProxySettings::GlobProxySettings(void)
/* ========================================================================= */
    : https_proxy("-1"),
    http_proxy("-1")
{
}


/* ========================================================================= */
GlobProxySettings::~GlobProxySettings(void)
/* ========================================================================= */
{
}


/* ========================================================================= */
/*
 * Load data from supplied settings.
 */
void GlobProxySettings::loadFromSettings(const QSettings &settings)
/* ========================================================================= */
{
	https_proxy = settings.value("connection/https_proxy",
	    dfltGlobProxSet.https_proxy).toString();
	http_proxy = settings.value("connection/http_proxy",
	    dfltGlobProxSet.http_proxy).toString();

	/* TODO */
}


/* ========================================================================= */
/*
 * Store data to settings structure.
 */
void GlobProxySettings::saveToSettings(QSettings &settings) const
/* ========================================================================= */
{
	settings.beginGroup("connection");

	if ("-1" == https_proxy) {
		settings.setValue("https_proxy", https_proxy);
	} else {
		/* TODO */
	}

	if ("-1" == http_proxy) {
		settings.setValue("http_proxy", http_proxy);
	} else {
		/* TODO */
	}

	settings.endGroup();
}
