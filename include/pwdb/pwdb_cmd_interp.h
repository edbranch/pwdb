/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef pwdb_pwdb_cmd_interp_h_included
#define pwdb_pwdb_cmd_interp_h_included

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

#include "cmd_interp/cmd_interp.h"
#include "pwdb/db.h"

namespace pwdb {

class pwdb_cmd_interp
{
    bool modified_{false};
    pwdb::db &cdb_;
    cmd_interp::interp interp_;

    auto def_interp(void)->cmd_interp::interp;
public:
    pwdb_cmd_interp(void) = delete;
    pwdb_cmd_interp(const pwdb_cmd_interp&) = delete;
    pwdb_cmd_interp(pwdb_cmd_interp&&) = default;
    pwdb_cmd_interp(pwdb::db &cdb);
    auto operator=(const pwdb_cmd_interp&)->pwdb_cmd_interp& = delete;
    auto operator=(pwdb_cmd_interp&&)->pwdb_cmd_interp& = default;

    void run(std::string prompt) { interp_.run(prompt); }
    bool modified(void) const { return modified_; }
};

class rcd_cmd_interp
{
    bool modified_{false};
    pwdb::pb::Store store_;
    cmd_interp::interp interp_;

    auto def_interp(void)->cmd_interp::interp;
    template<typename InputIt>
    static void print_columns(InputIt begin, InputIt end) {
        cmd_interp::print_columns(std::cout, begin, end, " : ", "  ");
    }
public:
    rcd_cmd_interp(void);
    rcd_cmd_interp(const rcd_cmd_interp&) = delete;
    rcd_cmd_interp(rcd_cmd_interp&&) = default;
    rcd_cmd_interp(const pwdb::pb::Store &store);
    auto operator=(const rcd_cmd_interp&)->rcd_cmd_interp& = delete;
    auto operator=(rcd_cmd_interp&&)->rcd_cmd_interp& = default;

    void run(std::string prompt) { interp_.run(prompt); }
    bool handle(const std::string &cmdline) const
        { return interp_.handle(cmdline); };
    auto store(void) const->const pwdb::pb::Store & { return store_; }
    bool modified(void) const { return modified_; }
    void print(void) const;
};

} // namespace pwdb
#endif // pwdb_pwdb_cmd_interp_h_included
