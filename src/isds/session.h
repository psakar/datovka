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

#pragma once

#include <QMutex>
#include <QObject> /* Q_DISABLE_COPY */

extern "C" {
	struct isds_ctx;
}

namespace Isds {

	/*!
	 * @brief Communication context together with mutex lock.
	 */
	class Session {

	private:
		Q_DISABLE_COPY(Session)

	private:
		/*!
		 * @brief Private constructor.
		 */
		Session(void);

	public:
		/*!
		 * @brief Destructor.
		 */
		virtual
		~Session(void);

		/*!
		 * @brief Access libisds context.
		 *
		 * @return Pointer to context structure.
		 */
		struct isds_ctx *ctx(void);

		/*!
		 * @brief Access mutex.
		 *
		 * @return Pointer to mutex.
		 */
		QMutex *mutex(void);

		/*!
		 * @brief Set time-out in milliseconds.
		 *
		 * @param[in] timeoutMs Connection timeout in milliseconds.
		 * @return True on success.
		 */
		bool setTimeout(unsigned int timeoutMs);

		/*!
		 * @brief Creates a session. Session is guaranteed to be fully
		 *     created.
		 *
		 * @param[in] connectionTimeoutMs Connection timeout in milliseconds.
		 * @return Pointer to newly created instance, Q_NULLPTR on failure.
		 */
		static
		Session *createSession(unsigned int connectionTimeoutMs);

	private:
		struct isds_ctx *m_ctx; /*!< Communication context. */
		QMutex m_mutex; /*!< Mutex. */
	};

}
