/* SPDX-License-Identifier: GPL-3.0-or-later */
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

#include "pwdb/util.h"
#include <filesystem>
#include <fstream>
#include <system_error>
#include <cerrno>
#include <cstring>

extern "C" {
#include <curses.h>
#include <term.h>
}

namespace pwdb {

namespace fs = ::std::filesystem;

std::string
xdg_data_dir(void)
{
    const char *xdg_data_home = getenv("XDG_DATA_HOME");
    fs::path data_dir;
    if(xdg_data_home) {
        data_dir = xdg_data_home;
    }
    if(data_dir.empty()) {
        data_dir = getenv("HOME");
        data_dir /= ".local/share";
    }
    return data_dir.string();
}

void
overwrite_file(const std::string &path,
        std::function<void(std::ostream&)> writer)
{
    fs::path tmpfile{path + ".tmp"};
    std::ofstream out(tmpfile.string(),
            std::ios::binary | std::ios::trunc | std::ios::out);
    if(!out) {
        throw std::system_error(errno, std::generic_category());
    }
    writer(out);
    fs::rename(tmpfile, path);
}

//----------------------------------------------------------------------------
// term_mode
//----------------------------------------------------------------------------
term_mode::
term_mode(void)
{
    if(ERR == setupterm(NULL, 1, NULL))
        throw std::runtime_error("terminal info could not be initialized");
    smcup = tigetstr("smcup");
    rmcup = tigetstr("rmcup");
    if(smcup == 0 || smcup == (char*)(-1l) || rmcup == 0 ||
            rmcup == (char*)(-1l))
        throw std::runtime_error("terminal could not be initialized\n");
    putp(smcup);
}

term_mode::
~term_mode()
{
    putp(rmcup);
}

} // namespace pwdb
