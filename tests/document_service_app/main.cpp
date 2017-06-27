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

#include <QApplication>
#include <QCommandLineParser>

#include "tests/document_service_app/gui/app.h"

#define BASE_URL_OPT "base-url"
#define TOKEN_OPT "token"
#define CA_CERT_OPT "ca-cert"
#define IGNORE_SSL_ERRORS "ignore-ssl-errors"

static
int setupCmdLineParser(QCommandLineParser &parser)
{
	parser.setApplicationDescription(QObject::tr("Experimental application"));
	parser.addHelpOption();
	parser.addVersionOption();

	if (!parser.addOption(QCommandLineOption(BASE_URL_OPT,
	        "Use <url> for service base URL.", "url"))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(TOKEN_OPT,
	        "Use <token-string> to use to connect to service.", "token-string"))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(CA_CERT_OPT,
	        "Use <cert-file> as trusted certificate to validate SSL connection.", "cert-file"))) {
		return -1;
	}
	if (!parser.addOption(QCommandLineOption(IGNORE_SSL_ERRORS,
	        "Set this option to ignore SSL connection errors."))) {
		return -1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	QCoreApplication::setApplicationName("ds_test_app");
	QCoreApplication::setApplicationVersion(VERSION);

	QCommandLineParser parser;
	if (0 != setupCmdLineParser(parser)) {
		qCritical("%s", "Cannot set up command-line parser.");
		return EXIT_FAILURE;
	}

	/* Process command-line arguments. */
	parser.process(app);

	MainWindow mainWin(
	    parser.isSet(BASE_URL_OPT) ? parser.value(BASE_URL_OPT) : QString(),
	    parser.isSet(TOKEN_OPT) ? parser.value(TOKEN_OPT) : QString(),
	    parser.isSet(CA_CERT_OPT) ? parser.value(CA_CERT_OPT) : QString(),
	    parser.isSet(IGNORE_SSL_ERRORS));
	mainWin.show();
	return app.exec();
}
