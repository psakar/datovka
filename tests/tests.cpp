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

#include <QTest>

#include "tests/test_crypto_message.h"
#include "tests/test_crypto_pin_pwd.h"
#include "tests/test_crypto_pin_token.h"
#include "tests/test_db_container.h"
#include "tests/test_isds_login.h"
#include "tests/test_message_db_set.h"
#include "tests/test_task_downloads.h"
#include "tests/test_task_send_message.h"
#include "tests/test_version.h"

static
int testThisClassAndDelete(QObject *testClassObj, int argc, char *argv[])
{
	int status;

	if (testClassObj == Q_NULLPTR) {
		return 1;
	}

	status = QTest::qExec(testClassObj, argc, argv);

	delete testClassObj;

	return status;
}

int main(int argc, char *argv[])
{
	int status = 0;
	qint64 msgId = 0;

#if defined (TEST_CRYPTO_MESSAGE)
	status |= testThisClassAndDelete(newTestCryptoMessage(), argc, argv);
#endif /* defined (TEST_CRYPTO_MESSAGE) */

#if defined (TEST_CRYPTO_PIN_PWD)
	status |= testThisClassAndDelete(newTestCryptoPinPwd(), argc, argv);
#endif /* defined (TEST_CRYPTO_PIN_PWD) */

#if defined (TEST_CRYPTO_PIN_TOKEN)
	status |= testThisClassAndDelete(newTestCryptoPinToken(), argc, argv);
#endif /* defined (TEST_CRYPTO_PIN_TOKEN) */

#if defined (TEST_DB_CONTAINER)
	status |= testThisClassAndDelete(newTestDbContainer(), argc, argv);
#endif /* defined (TEST_DB_CONTAINER) */

#if defined (TEST_MESSAGE_DB_SET)
	status |= testThisClassAndDelete(newTestMessageDbSet(), argc, argv);
#endif /* defined (TEST_MESSAGE_DB_SET) */

#if defined (TEST_ISDS_LOGIN)
	status |= testThisClassAndDelete(newTestIsdsLogin(), argc, argv);
#endif /* defined (TEST_ISDS_LOGIN) */

#if defined (TEST_TASK_SEND_MESSAGE)
	status |= testThisClassAndDelete(newTestTaskSendMessage(msgId), argc, argv);
#endif /* defined (TEST_TASK_SEND_MESSAGE) */

#if defined TEST_TASK_DOWNLOADS
	status |= testThisClassAndDelete(newTestTaskDownloads(msgId), argc, argv);
#endif /* defined TEST_TASK_DOWNLOADS */

#if defined TEST_VERSION
	status |= testThisClassAndDelete(newTestVersion(), argc, argv);
#endif /* defined TEST_VERSION */

	return status;
}
