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


#ifndef CLI_H
#define CLI_H

#include <QApplication>
#include <QDebug>

#define CLI_RET_ERROR_CODE -1
#define CLI_PREFIX "D-CLI: "
#define PARSER_PREFIX "Parser error: "
#define SERVICE_LABEL "service"
#define ATTACH_LABEL "dmAttachment"

// Login method
#define L_USER "user" //username
#define L_CERT "cert" //certificate
#define L_HOTP "hotp" //secutiry code
#define L_TOTP "totp" //sms code

// Message type
#define MT_SENT "sent"
#define MT_RECEIVED "received"
#define MT_SENT_RECEIVED "all"

// Define services names
#define SER_CONNECT "connect"
#define SER_GET_MSG_LIST "get-msg-list"
#define SER_SEND_MSG "send-msg"
#define SER_DWNLD_MSG "download-msg"
#define SER_DWNLD_DEL_INFO "download-delivery-info"
#define SER_GET_USER_INFO "get-user-info"
#define SER_GET_OWNER_INFO "get-owner-info"

int runService(const QString &service, const QString &paramString);

#endif // CLI_H
