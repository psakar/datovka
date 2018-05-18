/*
 * Copyright (C) 2014-2018 CZ.NIC
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

#if defined(__APPLE__) || defined(__clang__)
#  define __USE_C99_MATH
#  define _Bool bool
#else /* !__APPLE__ */
#  include <cstdbool>
#endif /* __APPLE__ */

#include <isds.h>
#include <QFile>

#include "src/isds/message_conversion.h"
#include "src/isds/message_functions.h"
#include "src/log/log.h"

Isds::Message Isds::messageFromData(const QByteArray &rawMsgData,
    enum LoadType zfoType)
{
	isds_error status;
	isds_raw_type rawType;
	struct isds_ctx *iDummySession = NULL;
	struct isds_message *iMessage = NULL;
	bool isMessage = false;
	Message message;
	bool iOk = false;

	iDummySession = isds_ctx_create();
	if (Q_UNLIKELY(NULL == iDummySession)) {
		logErrorNL("%s", "Cannot create dummy session.");
		goto fail;
	}

	status = isds_guess_raw_type(iDummySession, &rawType,
	    rawMsgData.constData(), rawMsgData.size());
	if (IE_SUCCESS != status) {
		logErrorNL("%s", "Cannot guess message content type.");
		goto fail;
	}

	if (zfoType != LT_DELIVERY) {
		status = isds_load_message(iDummySession, rawType,
		    rawMsgData.constData(), rawMsgData.size(), &iMessage,
		    BUFFER_COPY);
		isMessage = (IE_SUCCESS == status);
	}
	if ((!isMessage) && (zfoType != LT_MESSAGE)) {
		status = isds_load_delivery_info(iDummySession, rawType,
		    rawMsgData.constData(), rawMsgData.size(), &iMessage,
		    BUFFER_COPY);
	}
	if (IE_SUCCESS != status) {
		logErrorNL("%s", "Cannot load message content.");
		goto fail;
	}

	message = libisds2message(iMessage, &iOk);
	if (!iOk) {
		logErrorNL("%s", "Cannot create message object.");
		/* Message is null, fail follows. */
	}

fail:
	if (NULL != iMessage) {
		isds_message_free(&iMessage);
	}
	if (NULL != iDummySession) {
		isds_ctx_free(&iDummySession);
	}
	return message;
}

Isds::Message Isds::messageFromFile(const QString &fName, enum LoadType zfoType)
{
	if (Q_UNLIKELY(fName.isEmpty())) {
		Q_ASSERT(0);
		return Message();
	}

	QFile file(fName);
	if (!file.open(QIODevice::ReadOnly)) {
		logErrorNL("Cannot open file '%s'.",
		    fName.toUtf8().constData());
		return Message();
	}

	QByteArray content(file.readAll());
	file.close();

	return messageFromData(content, zfoType);
}
