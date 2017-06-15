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

#ifndef _DB_TABLES_H_
#define _DB_TABLES_H_

#include "src/io/sqlite/table.h"

/*
 * Account database.
 */
extern SQLiteTbl accntinfTbl; /*!< Table 'account_info'. */
extern SQLiteTbl userinfTbl; /*!< Table 'user_info'. */
extern SQLiteTbl pwdexpdtTbl; /*!< Table 'password_expiration_date'. */

/*
 * Message database.
 */
extern SQLiteTbl msgsTbl; /*!< Table 'messages'. */
extern SQLiteTbl flsTbl; /*!< Table 'files'. */
extern SQLiteTbl hshsTbl; /*!< Table 'hashes'. */
extern SQLiteTbl evntsTbl; /*!< Table 'events'. */
extern SQLiteTbl prcstTbl; /*!< Table 'process_state'. */
extern SQLiteTbl rwmsgdtTbl; /*!< Table 'raw_message_data'. */
extern SQLiteTbl rwdlvrinfdtTbl; /*!< Table 'raw_delivery_info_data'. */
extern SQLiteTbl smsgdtTbl; /*!< Table 'supplementary_message_data'. */
extern SQLiteTbl crtdtTbl; /*!< Table 'certificate_data'. */
extern SQLiteTbl msgcrtdtTbl; /*!< Table 'message_certificate_data'. */

/*
 * Tag database.
 */
extern SQLiteTbl tagTbl; /*!< Table 'tag'. */
extern SQLiteTbl msgtagsTbl; /*!< Table 'message_tags'. */

/*
 * Document service database.
 */
extern SQLiteTbl srvcInfTbl; /*!< Table 'service_info'. */
extern SQLiteTbl strdFlsMsgsTbl; /*!< Table 'stored_files_messages'. */

#endif /* _DB_TABLES_H_ */
