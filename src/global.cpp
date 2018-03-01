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

#include <QtCore> /* Q_NULLPTR */

#include "src/global.h"

class LogDevice *GlobInstcs::logPtr = Q_NULLPTR;

class SingleInstanceEmitter *GlobInstcs::snglInstEmitterPtr = Q_NULLPTR;

class MessageProcessingEmitter *GlobInstcs::msgProcEmitterPtr = Q_NULLPTR;
class WorkerPool *GlobInstcs::workPoolPtr = Q_NULLPTR;

class GlobPreferences *GlobInstcs::prefsPtr = Q_NULLPTR;
class ProxiesSettings *GlobInstcs::proxSetPtr = Q_NULLPTR;
class PinSettings *GlobInstcs::pinSetPtr = Q_NULLPTR;
class RecordsManagementSettings *GlobInstcs::recMgmtSetPtr = Q_NULLPTR;

class IsdsSessions *GlobInstcs::isdsSessionsPtr = Q_NULLPTR;

class AccountDb *GlobInstcs::accntDbPtr = Q_NULLPTR;
class DbContainer *GlobInstcs::msgDbsPtr = Q_NULLPTR;
class TagDb *GlobInstcs::tagDbPtr = Q_NULLPTR;
class RecordsManagementDb *GlobInstcs::recMgmtDbPtr = Q_NULLPTR;

class AccountsMap *GlobInstcs::acntMapPtr = Q_NULLPTR;
