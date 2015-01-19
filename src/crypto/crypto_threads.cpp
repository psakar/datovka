/*
 * Copyright (C) 2014-2015 CZ.NIC
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


#include <openssl/crypto.h>
#include <openssl/err.h>
#include <QMutex>
#include <QThread>
#include <QVector>

#include "src/crypto/crypto_threads.h"


/*!
 * @brief Mutexes to be used by OpenSSL.
 */
static QVector<QMutex *> openssl_mutexes;


/*!
 * @brief Callback for OpenSSL mutex locking and unlocking.
 *
 * @see CRYPTO_set_locking_callback() in OpenSSL documentation.
 *
 * @param mode  Locking mode.
 * @param n     Mutex number.
 * @param file  Source file where locking occurs (for debugging).
 * @param line  Line number where locking occurs (for debugging).
 */
static
void openssl_mutex_cb(int mode, int n, const char *file, int line);


/*
 * @brief Initialise mutexes for OpenSSL usage.
 */
static
void openssl_mutexes_init(void);


/*
 * @brief Destroy mutexes for OpenSSL usage.
 */
static
void openssl_mutexes_destroy(void);


/*!
 * @brief Callback for thread identification for purpose of OpenSSL.
 *
 * @see CRYPTO_THREADID_set_callback() in OpenSSL documentation.
 *
 * @param openssl_id  Thread identifier in OpenSSL.
 */
static
void openssl_threadid_cb(CRYPTO_THREADID *openssl_id);


/* ========================================================================= */
/*
 * Initialise cryptographic back-end for multi-threaded use.
 */
void crypto_init_threads(void)
/* ========================================================================= */
{
	/* Locking. */
	if (openssl_mutexes.isEmpty()) {
		openssl_mutexes_init();
	}

	/* Thread identification. */
	CRYPTO_THREADID_set_callback(openssl_threadid_cb);
}


/* ========================================================================= */
/*
 * Perform clean-up.
 */
void crypto_cleanup_threads(void)
/* ========================================================================= */
{
	if (!openssl_mutexes.isEmpty()) {
		openssl_mutexes_destroy();
	}
}


/* ========================================================================= */
/* Static function definitions below here. */
/* ========================================================================= */


/* ========================================================================= */
/*
 * Callback for OpenSSL mutex locking and unlocking.
 */
void openssl_mutex_cb(int mode, int n, const char *file, int line)
/* ========================================================================= */
{
	(void) file;
	(void) line;

	Q_ASSERT(!openssl_mutexes.isEmpty());
	Q_ASSERT(n < openssl_mutexes.size());

	QMutex *mutex = openssl_mutexes[n];

	if (mode & CRYPTO_LOCK) {
		mutex->lock();
	} else {
		mutex->unlock();
	}
}


/* ========================================================================= */
/*
 * Initialise mutexes for OpenSSL usage.
 */
static
void openssl_mutexes_init(void)
/* ========================================================================= */
{
	int openssl_mutex_count;
	int i;
	QMutex *mutex;

	Q_ASSERT(openssl_mutexes.isEmpty());

	openssl_mutex_count = CRYPTO_num_locks();
	if (0 == openssl_mutex_count) {
		return;
	}

	for (i = 0; i < openssl_mutex_count; ++i) {
		mutex = new(std::nothrow) QMutex(QMutex::NonRecursive);
		openssl_mutexes.append(mutex);
	}

	CRYPTO_set_locking_callback(openssl_mutex_cb);
}


/* ========================================================================= */
/*
 * Destroy mutexes for OpenSSL usage.
 */
static
void openssl_mutexes_destroy(void)
/* ========================================================================= */
{
	int i;

	Q_ASSERT(!openssl_mutexes.isEmpty());

	for (i = 0; i < openssl_mutexes.size(); ++i) {
		delete openssl_mutexes[i];
	}

	openssl_mutexes.clear();
}


/* ========================================================================= */
/*
 * Callback for thread identification for purpose of OpenSSL.
 */
static
void openssl_threadid_cb(CRYPTO_THREADID *openssl_id)
/* ========================================================================= */
{
	QThread *id = QThread::currentThread();
	CRYPTO_THREADID_set_pointer(openssl_id, (void *)id);
}
