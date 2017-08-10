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

#include <cstdlib>
#include <QTextStream>

#include "src/cli/cli.h"
#include "src/cli/cli_parser.h"
#include "src/log/log.h"

int CLIParser::setupCmdLineParser(QCommandLineParser &parser)
{
	parser.setApplicationDescription(QObject::tr("Data box application"));
	parser.addHelpOption();
	parser.addVersionOption();
	/* Options with values. */
	if (!parser.addOption(QCommandLineOption(CONF_SUBDIR_OPT,
	        QObject::tr(
	            "Use <conf-subdir> subdirectory for configuration."),
	        QObject::tr("conf-subdir")))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(LOAD_CONF_OPT,
	        QObject::tr("On start load <conf> file."),
	        QObject::tr("conf")))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(SAVE_CONF_OPT,
	        QObject::tr("On stop save <conf> file."),
	        QObject::tr("conf")))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(LOG_FILE,
	        QObject::tr("Log messages to <file>."),
	        QObject::tr("file")))) {
		return -1;
	}
	QCommandLineOption logVerb(QStringList() << "L" << LOG_VERBOSITY_OPT,
	    QObject::tr("Set verbosity of logged messages to <level>. "
	        "Default is ") + QString::number(globLog.logVerbosity()) + ".",
	    QObject::tr("level"));
	if (!parser.addOption(logVerb)) {
		return -1;
	}
	/* Boolean options. */
#ifdef DEBUG
	QCommandLineOption debugOpt(QStringList() << "D" << DEBUG_OPT,
	    "Enable debugging information.");
	if (!parser.addOption(debugOpt)) {
		return -1;
	}
	QCommandLineOption debugVerb(QStringList() << "V" << DEBUG_VERBOSITY_OPT,
	    QObject::tr("Set debugging verbosity to <level>. Default is ") +
	    QString::number(globLog.debugVerbosity()) + ".",
	    QObject::tr("level"));
	if (!parser.addOption(debugVerb)) {
		return -1;
	}
#endif /* DEBUG */

	/* Options with values. */
	if (!parser.addOption(QCommandLineOption(SER_LOGIN,
	        QObject::tr("Service: connect to isds and login into databox."),
	        QObject::tr("string-of-parameters")))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(SER_GET_MSG_LIST,
	        QObject::tr("Service: download list of received/sent "
	        "messages from ISDS."),
	        QObject::tr("string-of-parameters")))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(SER_SEND_MSG,
	        QObject::tr("Service: create and send a new message to ISDS."),
	        QObject::tr("string-of-parameters")))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(SER_GET_MSG,
	        QObject::tr("Service: download complete message with "
	        "signature and time stamp of MV."),
	        QObject::tr("string-of-parameters")))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(SER_GET_DEL_INFO,
	        QObject::tr("Service: download acceptance info of message "
	        "with signature and time stamp of MV."),
	        QObject::tr("string-of-parameters")))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(SER_GET_USER_INFO,
	        QObject::tr("Service: get information about user "
	        "(role, privileges, ...)."),
	        NULL))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(SER_GET_OWNER_INFO,
	        QObject::tr("Service: get information about owner and "
	        "its databox."),
	        NULL))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(SER_CHECK_ATTACHMENT,
	        QObject::tr("Service: get list of messages where "
	        "attachment missing (local database only)."),
	        NULL))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(SER_FIND_DATABOX,
	        QObject::tr("Service: find a databox via several parameters."),
	        QObject::tr("string-of-parameters")))) {
		return -1;
	}

	parser.addPositionalArgument("[zfo-file]",
	    QObject::tr("ZFO file to be viewed."));

	return 0;
}

QStringList CLIParser::CLIServiceArgs(const QStringList &options)
{
	QStringList srvcArgs;

	foreach (const QString &option, options) {
		if (serviceSet.contains(option)) {
			srvcArgs.append(option);
		}
	}

	return srvcArgs;
}

int CLIParser::runCLIService(const QStringList &srvcArgs,
    const QCommandLineParser &parser)
{
	int ret = EXIT_FAILURE;
	QString errmsg;
	QString serName;
	int index = 0;
	QTextStream cout(stderr);

	// every valid CLI action must have only one login parameter
	// or one login parameter and one name service
	switch (srvcArgs.count()) {
	case 0:
		errmsg = "No service has been defined for CLI action!";
		break;
	case 1:
		if (srvcArgs.contains(SER_LOGIN)) {
			ret = runService(parser.value(SER_LOGIN),
			    NULL, NULL);
			return ret;
		} else {
			errmsg = "Only service name was set. "
			    "Login parameter is missing!";
		}
		break;
	case 2:
		if (srvcArgs.contains(SER_LOGIN)) {
			index = srvcArgs.indexOf(SER_LOGIN);
			if (index == 0) {
				serName = srvcArgs.at(1);
			} else {
				serName = srvcArgs.at(0);
			}
			ret = runService(parser.value(SER_LOGIN),
			    serName, parser.value(serName));
			return ret;
		} else {
			errmsg = "Login parameter is missing! "
			    "Maybe two service names were set.";
		}
		break;
	default:
		errmsg = "More than two service names or logins were set! "
		    "This situation is not allowed.";
		break;
	}

	// print error to stderr
	cout << CLI_PREFIX << " error(" << CLI_ERROR << ") : "
	    << errmsg << endl;

	return ret;
}
