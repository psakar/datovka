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

#include <cstdio>
#include <cstring>
#include <QtGlobal> // Q_OS_*

#if defined(Q_OS_WIN)
#  include <windows.h>
#elif defined(Q_OS_MACOS) || defined(Q_OS_UNIX) || defined(Q_OS_LINUX)
#  include <termios.h>
#  include <unistd.h>
#else
#  error "Unknown or unsupported architecture."
#endif

#include "src/cli/cli_pin.h"
#include "src/log/log.h"

/*
 * Some code is based on:
 * https://stackoverflow.com/questions/1413445/reading-a-password-from-stdcin
 * https://stackoverflow.com/questions/6856635/hide-password-input-on-terminal
 */

/*!
 * @brief Set echo mode for standard input.
 *
 * @param[in] enable True if echo mode should be enabled.
 */
static
void enableStdinEcho(bool enable)
{
#if defined(Q_OS_WIN)
	/*
	 * This solution works when application is run from
	 * command line (cmd.exe) or from PowerShell.
	 * It does not work when the application is run from emulated terminals
	 * on Windows such as Git Bash or MinGW.
	 */

	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	DWORD mode;
	GetConsoleMode(hStdin, &mode);

	if(!enable) {
		mode &= ~ENABLE_ECHO_INPUT;
	} else {
		mode |= ENABLE_ECHO_INPUT;
	}

	SetConsoleMode(hStdin, mode );

#elif defined(Q_OS_MACOS) || defined(Q_OS_UNIX) || defined(Q_OS_LINUX)

	struct termios tty;
	tcgetattr(STDIN_FILENO, &tty);
	if(!enable) {
		tty.c_lflag &= ~ECHO;
	} else {
		tty.c_lflag |= ECHO;
	}

	(void) tcsetattr(STDIN_FILENO, TCSANOW, &tty);

#endif
}

/*!
 * @brief Remove trailing newline character.
 *
 * @param[in,out] buf Null-terminated string buffer.
 */
static
void remove_trailing_newline(char *buf)
{
	if (Q_UNLIKELY(NULL == buf)) {
		return;
	}

	char *pos = strchr(buf, '\n');
	if (pos != NULL) {
		*pos = '\0';
	} else {
		/* Input too long for buffer? */
	}
}

#define STDIN stdin
#define STDOUT stdout
#define PIN_BUF_LEN 256

/*!
 * @brief Read terminal/console input without echoing characters.
 *
 * @brief Returns null string on error.
 */
static
QString readInputNoEcho(void)
{
	char pin_buf[PIN_BUF_LEN];

	/*
	 * Using getch() or _getch() causes troubles inside MinGW terminal.
	 */

	enableStdinEcho(false);
	const char *ret = fgets(pin_buf, PIN_BUF_LEN, STDIN);
	enableStdinEcho(true);
	remove_trailing_newline(pin_buf);

	if (NULL != ret) {
		return QString((const char *)pin_buf);
	} else {
		return QString();
	}
}

bool CLIPin::queryPin(PinSettings &sett, int repeatNum)
{
	if (Q_UNLIKELY(!sett.pinConfigured())) {
		Q_ASSERT(0);
		return false;
	}

	if (repeatNum < 0) {
		repeatNum = 0;
	}

	bool properlySet = false;

	do {
		fprintf(STDOUT, "%s: ",
		    tr("Enter PIN code").toUtf8().constData());
		fflush(STDOUT);

		QString pinVal(readInputNoEcho());

		fputc('\n', STDOUT);
		fflush(STDOUT);

		if (!PinSettings::verifyPin(sett, pinVal)) {
			logWarningNL("%s",
			    "Could not verify entered PIN value.");
		} else {
			properlySet = true;
		}

		--repeatNum;
	} while (!properlySet && ((repeatNum + 1) > 0));

	return properlySet;
}
