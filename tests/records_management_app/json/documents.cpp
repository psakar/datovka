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

#include "tests/records_management_app/json/documents.h"

const QByteArray uhResDoc(
"{ \"name\": null, \"id\": null, \"metadata\": [\"root\"], \"sub\":"
"    ["
"        { \"name\": \"NIX.CZ\", \"id\": null, \"metadata\": [\"Neutral Internet eXchange\", \"propojení sítí\", \"peeringový uzel\"], \"sub\":"
"            ["
"                { \"name\": \"obecné\", \"id\": \"af51ef52yx\", \"metadata\": [\"povšechné záležitosti\"], \"sub\": [] },"
"                { \"name\": \"FENIX\", \"id\": \"af51ef52yz\", \"metadata\": [\"reakce na DoS\"], \"sub\": [] }"
"            ]"
"        },"
"        { \"name\": \"CZ.NIC\", \"id\": null, \"metadata\": [\"Network Information Centre\", \"Domain Name Registry\", \"provozovatel domény .cz\"], \"sub\":"
"            ["
"                { \"name\": \"obecné\", \"id\": \"aabbccddee\", \"metadata\": [\"povšechné záležitosti\"], \"sub\": [] },"
"                { \"name\": \"Datovka\", \"id\": \"aabbccddef\", \"metadata\": [\"datové schránky\", \"ISDS\"], \"sub\": [] },"
"                { \"name\": \"Tablexia\", \"id\": \"aabbccddf0\", \"metadata\": [\"hry\", \"dyslexie\"], \"sub\": [] },"
"                { \"name\": \"Knot DNS\", \"id\": \"aabbccddf1\", \"metadata\": [\"authoritative name server\"], \"sub\": [] },"
"                { \"name\": \"Knot Resolver\", \"id\": \"aabbccddf2\", \"metadata\": [\"recursive name server\"], \"sub\": [] }"
"            ]"
"        }"
"    ]"
"}"
);
