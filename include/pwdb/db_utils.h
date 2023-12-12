/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef pwdb_db_utils_h_included
#define pwdb_db_utils_h_included

/***
    This file is part of pwdb.

    Copyright (C) 2018 Edward Branch

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
    more details.

    You should have received a copy of the GNU General Public License along
    with this program. If not, see <https://www.gnu.org/licenses/>.

***/

#include "pwdb/db.h"
#include "gpgh/gpg_helper.h"

namespace pwdb {

auto db_open_rcd_store(gpgh::context &ctx, const db &cdb,
        pwdb::db::rcd_citer_t i)->pwdb::pb::Store;
void db_save_rcd_store(gpgh::context &ctx, db &cdb, const std::string &name,
        const pwdb::pb::Store &pb_store);
void db_recrypt_rcd_stores(gpgh::context &ctx, db &cdb);
void db_decrypt_all_rcd_stores(gpgh::context &ctx, db &cdb);

} // namespace pwdb
#endif // pwdb_db_utils_h_included
