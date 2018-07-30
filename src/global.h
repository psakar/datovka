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

/*
 * Forward class declaration.
 * These classes must be declared before the following namespace.
 */
class SingleInstanceEmitter;

class MessageProcessingEmitter;
class WorkerPool;

class Preferences;
class ProxiesSettings;
class PinSettings;
class RecordsManagementSettings;

class IsdsSessions;

class AccountDb;
class DbContainer;
class TagDb;
class RecordsManagementDb;

class AccountsMap;

/*!
 * @brief The namespace holds pointers to all globally accessible structures.
 */
namespace GlobInstcs {

	extern
	class SingleInstanceEmitter *snglInstEmitterPtr; /*!< Single instance message emitter. */

	extern
	class MessageProcessingEmitter *msgProcEmitterPtr; /*!< Task message emitter. */
	extern
	class WorkerPool *workPoolPtr; /*!< Worker pool. */

	extern
	class Preferences *prefsPtr; /*!< Preferences. */
	extern
	class ProxiesSettings *proxSetPtr; /*!< Proxy settings. */
	extern
	class PinSettings *pinSetPtr; /*!< PIN settings. */
	extern
	class RecordsManagementSettings *recMgmtSetPtr; /*!< Records management settings. */

	extern
	class IsdsSessions *isdsSessionsPtr; /*!< ISDS session container. */

	extern
	class AccountDb *accntDbPtr; /*!< Account database. */
	extern
	class DbContainer *msgDbsPtr; /*!< Message database container. */
	extern
	class TagDb *tagDbPtr; /*!< Tag database. */
	extern
	class RecordsManagementDb *recMgmtDbPtr; /*!< Records management database. */

	/*!
	 * @brief Holds account data related to account.
	 *
	 * @note Key is userName. The user name is held by the user name list.
	 */
	extern
	class AccountsMap *acntMapPtr;

}
