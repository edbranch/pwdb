/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef pwdb_util_h_included
#define pwdb_util_h_included

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

#include <functional>
#include <string>
#include <ostream>

namespace pwdb {

auto xdg_data_dir(void)->std::string;
void overwrite_file(const std::string &db_file, const std::string &tmp_file,
        std::function<void(std::ostream&)> writer);

class term_mode
{
public:
    term_mode(void);
    term_mode(const term_mode &) = delete;
    term_mode &operator=(const term_mode &) = delete;
    ~term_mode();
private:
    const char *smcup;  // control code enter term alternate buffer
    const char *rmcup;  // control code exit term aternate buffer
};

} // namespace pwdb
#endif // pwdb_util_h_included
