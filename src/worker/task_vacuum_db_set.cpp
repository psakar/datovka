/*
 * Copyright (C) 2014-2016 CZ.NIC
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

#include <QtGlobal> /* QT_VERSION */
#include <QObject>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
#include <QStorageInfo>
#else /* < Qt-5.4 */
#warning "Compiling against version < Qt-5.4." /* Doesn't have QStorageInfo. */
#include <QTemporaryFile>
#endif /* >= Qt-5.4 */
#include <QThread>

#include "src/log/log.h"
#include "src/worker/message_emitter.h"
#include "src/worker/task_vacuum_db_set.h"

TaskVacuumDbSet::TaskVacuumDbSet(MessageDbSet *dbSet)
    : m_success(false),
    m_error(),
    m_dbSet(dbSet)
{
}

void TaskVacuumDbSet::run(void)
{
	if (0 == m_dbSet) {
		Q_ASSERT(0);
		return;
	}

	logDebugLv0NL("Starting vacuum task in thread '%p'",
	    (void *) QThread::currentThreadId());

	/* ### Worker task begin. ### */

	m_success = vacuumDbSet(m_dbSet, m_error);

	emit globMsgProcEmitter.progressChange(PL_IDLE, 0);

	/* ### Worker task end. ### */

	logDebugLv0NL("Vacuum task finished in thread '%p'",
	    (void *) QThread::currentThreadId());
}

QString TaskVacuumDbSet::storagePlace(MessageDbSet *dbSet)
{
	Q_ASSERT(0 != dbSet);

	QString place;

	const QStringList fileNames(dbSet->fileNames());
	foreach (const QString &fileName, fileNames) {
		const QFileInfo fInfo(fileName);
		if (!fInfo.isFile()) {
			return QString();
		}
		if (!place.isEmpty()) {
			QString otherPlace(fInfo.absoluteDir().absolutePath());
			if (place != otherPlace) {
				return QString();
			}
		} else {
			place = fInfo.absoluteDir().absolutePath();
		}
	}

	return place;
}

#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
bool TaskVacuumDbSet::haveStorageSpace(const QString &storagePlace,
    qint64 spaceSize)
{
	Q_ASSERT(!storagePlace.isEmpty());
	Q_ASSERT(spaceSize > 0);

	QStorageInfo sInfo(storagePlace);
	if (!sInfo.isValid()) {
		logErrorNL("Cannot obtain storage info about '%s'.",
		    storagePlace.toUtf8().constData());
		return false;
	}

	return sInfo.bytesAvailable() > spaceSize;
}
#else /* < Qt-5.4 */
bool TaskVacuumDbSet::haveStorageSpace(const QString &storagePlace,
    qint64 spaceSize)
{
	Q_ASSERT(!storagePlace.isEmpty());
	Q_ASSERT(spaceSize > 0);

	/*
	 * Try creating a file of given file to check whether enough space
	 * available.
	 */

	QTemporaryFile tmpFile(storagePlace + QDir::separator() + QLatin1String("XXXXXX"));
	tmpFile.open();
	tmpFile.setAutoRemove(true);
	if (!tmpFile.exists()) {
		logErrorNL("Cannot create temporary file '%s' to check available space.",
		    tmpFile.fileName().toUtf8().constData());
		return false;
	}

	if (!tmpFile.resize(spaceSize)) {
		logErrorNL("Cannot acquire requested space in location '%s'.",
		    storagePlace.toUtf8().constData());
		return false;
	}

	return true;
}
#endif /* >= Qt-5.4 */

bool TaskVacuumDbSet::vacuumDbSet(MessageDbSet *dbSet, QString &error)
{
	Q_ASSERT(0 != dbSet);

	qint64 maxDbSize = dbSet->underlyingFileSize(MessageDbSet::SC_LARGEST);
	if (maxDbSize <= 0) {
		/* Nothing to do. */
		return true;
	}

	/* Small adjustment. */
	maxDbSize += (qint64)(maxDbSize * 0.02);

	const QString dbDir(storagePlace(dbSet));
	if (dbDir.isEmpty()) {
		error = QObject::tr("Could not determine database directory");
		return false;
	}

	/* Test for available space. */
	if (!haveStorageSpace(dbDir, maxDbSize)) {
		error = QObject::tr("Not enough space on device where '%1' resides.").arg(dbDir);
		return false;
	}

	bool ret = dbSet->vacuum();
	if (!ret) {
		error = QObject::tr("Calling vacuum on database set failed.");
	}

	return ret;
}
