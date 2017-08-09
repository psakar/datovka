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

#ifndef _CLI_H_
#define _CLI_H_

#include <QSet>
#include <QString>

#define CLI_PREFIX "D-CLI: "
#define PARSER_PREFIX "Parser error: "

// Login method
#define L_USER "user" //username
#define L_CERT "cert" //certificate
#define L_HOTP "hotp" //secutiry code
#define L_TOTP "totp" //sms code

// Message type
#define MT_SENT "sent"
#define MT_RECEIVED "received"
#define MT_SENT_RECEIVED "all"

// Databox type
#define DB_OVM "OVM"
#define DB_PO "PO"
#define DB_PFO "PFO"
#define DB_FO "FO"

// Define services names
#define SER_LOGIN "login"
#define SER_GET_MSG_LIST "get-msg-list"
#define SER_SEND_MSG "send-msg"
#define SER_GET_MSG "get-msg"
#define SER_GET_DEL_INFO "get-delivery-info"
#define SER_GET_USER_INFO "get-user-info"
#define SER_GET_OWNER_INFO "get-owner-info"
#define SER_CHECK_ATTACHMENT "check-attachment"
#define SER_FIND_DATABOX "find-databox"


// set of return error values
enum cli_error {
	CLI_SUCCESS = 0,
	CLI_ERROR,
	CLI_DB_ERR,
	CLI_CONNECT_ERR,
	CLI_UNKNOWN_SER,
	CLI_UNKNOWN_ATR,
	CLI_ATR_NAME_ERR,
	CLI_ATR_VAL_ERR,
	CLI_REQ_ATR_ERR
};

// IMORTANT: if any another service is/ will be defined,
// it must be added into this service list
extern const QSet<QString> serviceSet;

/*
 * Run login + service
 * [in] lParam = login parameter string
 * [in] service = name of service
 * [in] sParam = service parameter string
 * @return status code of operation (EXIT_SUCCESS or EXIT_FAILURE)
*/
int runService(const QString &lParam,
    const QString &service, const QString &sParam);

#endif /* _CLI_H_ */
