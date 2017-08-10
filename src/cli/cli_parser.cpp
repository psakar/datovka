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
	parser.setApplicationDescription(tr("Data box application"));
	parser.addHelpOption();
	parser.addVersionOption();
	/* Options with values. */
	if (!parser.addOption(QCommandLineOption(CONF_SUBDIR_OPT,
	        tr("Use <%1> subdirectory for configuration.").arg(tr("conf-subdir")),
	        tr("conf-subdir")))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(LOAD_CONF_OPT,
	        tr("On start load <%1> file.").arg(tr("conf")), tr("conf")))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(SAVE_CONF_OPT,
	        tr("On stop save <%1> file.").arg(tr("conf")), tr("conf")))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(LOG_FILE,
	        tr("Log messages to <%1>.").arg(tr("file")), tr("file")))) {
		return -1;
	}
	QCommandLineOption logVerb(QStringList() << "L" << LOG_VERBOSITY_OPT,
	    tr("Set verbosity of logged messages to <%1>. Default is %2.")
	        .arg(tr("level")).arg(QString::number(globLog.logVerbosity())),
	    tr("level"));
	if (!parser.addOption(logVerb)) {
		return -1;
	}
	/* Boolean options. */
#ifdef DEBUG
	QCommandLineOption debugOpt(QStringList() << "D" << DEBUG_OPT,
	    tr("Enable debugging information."));
	if (!parser.addOption(debugOpt)) {
		return -1;
	}
	QCommandLineOption debugVerb(QStringList() << "V" << DEBUG_VERBOSITY_OPT,
	    tr("Set debugging verbosity to <%1>. Default is %2.")
	        .arg(tr("level")).arg(QString::number(globLog.debugVerbosity())),
	    tr("level"));
	if (!parser.addOption(debugVerb)) {
		return -1;
	}
#endif /* DEBUG */

	/* Options with values. */
	if (!parser.addOption(QCommandLineOption(SER_LOGIN,
	        tr("Service: connect to ISDS and login into data box."),
	        tr("string-of-parameters")))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(SER_GET_MSG_LIST,
	        tr("Service: download list of received/sent messages from ISDS."),
	        tr("string-of-parameters")))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(SER_SEND_MSG,
	        tr("Service: create and send a new message to ISDS."),
	        tr("string-of-parameters")))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(SER_GET_MSG,
	        tr("Service: download complete message with signature and time stamp of MV."),
	        tr("string-of-parameters")))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(SER_GET_DEL_INFO,
	        tr("Service: download acceptance info of message with signature and time stamp of MV."),
	        tr("string-of-parameters")))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(SER_GET_USER_INFO,
	        tr("Service: get information about user (role, privileges, ...)."),
	        QString()))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(SER_GET_OWNER_INFO,
	        tr("Service: get information about owner and its data box."),
	        QString()))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(SER_CHECK_ATTACHMENT,
	        tr("Service: get list of messages where attachment missing (local database only)."),
	        QString()))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(SER_FIND_DATABOX,
	        tr("Service: find a data box via several parameters."),
	        tr("string-of-parameters")))) {
		return -1;
	}

	parser.addPositionalArgument(tr("[zfo-file]"),
	    tr("ZFO file to be viewed."));

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
	QTextStream errStream(stderr);

	/*
	 * Every valid CLI action must have only one login parameter
	 * or one login parameter and one service name.
	 */
	switch (srvcArgs.count()) {
	case 0:
		errmsg = "No service has been defined for CLI action!";
		break;
	case 1:
		if (srvcArgs.contains(SER_LOGIN)) {
			ret = runService(parser.value(SER_LOGIN),
			    QString(), QString());
			return ret;
		} else {
			errmsg = "Only service name was set. "
			    "Login parameter is missing!";
		}
		break;
	case 2:
		if (srvcArgs.contains(SER_LOGIN)) {
			if (0 == srvcArgs.indexOf(SER_LOGIN)) {
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

	/* Print error to error output. */
	errStream << CLI_PREFIX << " error(" << CLI_ERROR << ") : "
	    << errmsg << endl;

	return ret;
}
