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

#ifndef WIN32
#include <clocale>
#endif /* !WIN32 */
#include <QProcessEnvironment>

#include "src/cli/cli_parser.h"
#include "src/crypto/crypto.h"
#include "src/crypto/crypto_funcs.h"
#include "src/initialisation.h"
#include "src/io/file_downloader.h"

void setDefaultLocale(void)
{
#ifndef WIN32
	QString lang(QProcessEnvironment::systemEnvironment().value("LANG"));
	if (lang.isEmpty() || ("c" == lang.toLower())) {
#define LANG_DEF "en_GB.UTF-8"
		logWarning("The LANG environment variable is missing or unset. "
		    "Setting to '%s'.\n", LANG_DEF);
		if (NULL == setlocale(LC_ALL, LANG_DEF)) {
			logError("Setting locale to '%s' failed.\n", LANG_DEF);
		}
#undef LANG_DEF
	}
#endif /* !WIN32 */
}

int preferencesSetUp(const QCommandLineParser &parser,
    GlobPreferences &prefs, LogDevice &log)
{
	int logFileId = -1;

	if (parser.isSet(CONF_SUBDIR_OPT)) {
		prefs.confSubdir = parser.value(CONF_SUBDIR_OPT);
	}
	if (parser.isSet(LOAD_CONF_OPT)) {
		prefs.loadFromConf = parser.value(LOAD_CONF_OPT);
	}
	if (parser.isSet(SAVE_CONF_OPT)) {
		prefs.saveToConf = parser.value(SAVE_CONF_OPT);
	}
	if (parser.isSet(LOG_FILE)) {
		QString logFileName = parser.value(LOG_FILE);
		logFileId = log.openFile(logFileName.toUtf8().constData(),
		    LogDevice::LM_APPEND);
		if (-1 == logFileId) {
			logError("Cannot open log file '%s'.\n",
			    logFileName.toUtf8().constData());
			return -1;
		}
		/* Log warnings. */
		log.setLogLevels(logFileId, LOGSRC_ANY,
		    LOG_UPTO(LOG_WARNING));
	}
#ifdef DEBUG
	if (parser.isSet(DEBUG_OPT) || parser.isSet(DEBUG_VERBOSITY_OPT)) {
		log.setLogLevels(LogDevice::LF_STDERR, LOGSRC_ANY,
		    LOG_UPTO(LOG_DEBUG));
		if (-1 != logFileId) {
			log.setLogLevels(logFileId, LOGSRC_ANY,
			    LOG_UPTO(LOG_DEBUG));
		}
	}
	if (parser.isSet(DEBUG_VERBOSITY_OPT)) {
		bool ok;
		int value = parser.value(DEBUG_VERBOSITY_OPT).toInt(&ok, 10);
		if (!ok) {
			logError("%s\n", "Invalid debug-verbosity parameter.");
			exit(EXIT_FAILURE);
		}
		logInfo("Setting debugging verbosity to value '%d'.\n", value);
		log.setDebugVerbosity(value);
	}
#endif /* DEBUG */
	if (parser.isSet(LOG_VERBOSITY_OPT)) {
		bool ok;
		int value = parser.value(LOG_VERBOSITY_OPT).toInt(&ok, 10);
		if (!ok) {
			logError("%s\n", "Invalid log-verbosity parameter.");
			exit(EXIT_FAILURE);
		}
		logInfo("Setting logging verbosity to value '%d'.\n", value);
		log.setLogVerbosity(value);
	}

	return 0;
}

void downloadCRL(void)
{
	/* Start downloading the CRL files. */
	QList<QUrl> urlList;
	FileDownloader fDown(true);
	const struct crl_location *crl = crl_locations;
	const char **url;
	while ((NULL != crl) && (NULL != crl->file_name)) {
		urlList.clear();

		url = crl->urls;
		while ((NULL != url) && (NULL != *url)) {
			urlList.append(QUrl(*url));
			++url;
		}

		QByteArray data = fDown.download(urlList, 2000);
		if (!data.isEmpty()) {
			if (0 != crypto_add_crl(data.data(),
			        data.size())) {
				logWarning(
				    "Couldn't load downloaded CRL file '%s'.\n",
				    crl->file_name);
			} else {
				logInfo("Loaded CRL file '%s'.\n",
				    crl->file_name);
			}
		} else {
			logWarning("Couldn't download CRL file '%s'.\n",
			    crl->file_name);
		}

		++crl;
	}
}
