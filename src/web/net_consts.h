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

#ifndef NET_CONST_H
#define NET_CONST_H

#define APP_NAME "Datovka"

/* HTTP request timeout, default 30s */
#define NET_REQUEST_TIMEOUT 30000

#define COOKIE_SESSION_ID "sessionid"
#define COOKIE_SESSION_MOJEID "mojeidsession"
#define COOKIE_CSRFTOKEN "csrftoken"

#define WEBDATOVKA_LOGIN_URL1 "https://datovka.labs.nic.cz/development/mojeid/login"
#define WEBDATOVKA_LOGIN_URL "https://datovka.labs.nic.cz/development/fakelogin/1"
#define WEBDATOVKA_SERVICE_URL "https://datovka.labs.nic.cz/development/"

#define MOJEID_BASE_URL0 "https://mojeid.cz/editor/"
#define MOJEID_BASE_URL1 "https://mojeid.cz/consumer/"
#define MOJEID_BASE_URL2 "https://mojeid.cz/endpoint/"
#define MOJEID_BASE_URL3 "https://mojeid.cz/endpoint/password/"

#endif // NET_CONST_H
