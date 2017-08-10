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

#ifndef _CLI_PARSER_H_
#define _CLI_PARSER_H_

#include <QCommandLineParser>
#include <QStringList>

#define CONF_SUBDIR_OPT "conf-subdir"
#define LOAD_CONF_OPT "load-conf"
#define SAVE_CONF_OPT "save-conf"
#define LOG_FILE "log-file"

#define LOG_VERBOSITY_OPT "log-verbosity"
#define DEBUG_OPT "debug"
#define DEBUG_VERBOSITY_OPT "debug-verbosity"

/*!
 * @brief Provides namespace for convenience methods dealing with command line
 *     parser.
 */
class CLIParser {

private:
	/*!
	 * @brief Private constructor.
	 */
	CLIParser(void);

public:
	/*!
	 * @brief Performs command-line parser setup.
	 *
	 * @param[in,out] parser Parser to add options to.
	 * @return 0 on success, -1 else.
	 */
	static
	int setupCmdLineParser(QCommandLineParser &parser);

	/*!
	 * @brief Returns list of options related to CLI services.
	 *
	 * @param[in] options Command line options.
	 * @return List of options related to services.
	 */
	static
	QStringList CLIServiceArgs(const QStringList &options);

	/*!
	 * @brief Calls command-line service.
	 *
	 * @param[in] srvcArgs Service arguments.
	 * @patam[in] parser Command line parser.
	 * @return EXIT_SUCCESS on success, EXIT_FAILURE on failure.
	 */
	static
	int runCLIService(const QStringList &srvcArgs,
	    const QCommandLineParser &parser);
};

#endif /* _CLI_PARSER_H_ */
