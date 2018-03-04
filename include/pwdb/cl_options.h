/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef _pwdb_cl_options_h_
#define _pwdb_cl_options_h_

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

#include <string>
#include <optional>

namespace pwdb {

struct cl_options
{
    bool create;
    bool recrypt;
    std::string file;
    std::string uid;
    std::string record;
    std::string gpg_homedir;
};

std::optional<cl_options> cl_handle(int argc, const char *argv[]);

} // namespace pwdb
#endif // _pwdb_cl_options_h_
