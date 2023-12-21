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

#include <string>
#include <ostream>
#include <functional>
#include <filesystem>

namespace pwdb {

auto xdg_data_dir(void)->std::string;

//-----------------------------------------------------------------------------
class lock_overwrite_file
// Scope-guard class to safely handle modifying a file by read, modify in-memory
// representation, write to temp file, then replace input file with temp file.
// For mutex, the successfully opened temp file **is** the lock. The flock()
// and fcntl() locking mechanisms are inherently racey for this pattern as one
// process could replace the file between another process openning and
// locking. So we use exclusive creation of the tempfile as the lock since it
// closes this race window. Tradoff is if the program abnormally terminates it
// will leave the temp file in place, which must then be manually deleted.
//-----------------------------------------------------------------------------
{
    std::filesystem::path file_;
    std::filesystem::path tmp_file_;
    bool need_unlock_ = false;
public:
    lock_overwrite_file(const std::filesystem::path &file);
    lock_overwrite_file(const lock_overwrite_file&) = delete;
    lock_overwrite_file(lock_overwrite_file&&) = default;
    lock_overwrite_file &operator=(const lock_overwrite_file&) = delete;
    lock_overwrite_file &operator=(lock_overwrite_file&&) = default;
    ~lock_overwrite_file();

    auto file(void) const-> const std::filesystem::path& {return file_;}
    auto tmp_file(void) const-> const std::filesystem::path& {return tmp_file_;}
    void overwrite(std::function<void(std::ostream&)> writer);
};

//----------------------------------------------------------------------------
class term_mode
// Scope-guard class to set terminal to use the alt buffer
//----------------------------------------------------------------------------
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
