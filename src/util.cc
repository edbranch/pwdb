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
#include <unistd.h>
#include <fcntl.h>
#include <curses.h>
#include <term.h>
}

namespace pwdb {

using namespace std::literals::string_literals;
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


//----------------------------------------------------------------------------
// lock_overwrite_file
//----------------------------------------------------------------------------

lock_overwrite_file::
lock_overwrite_file(const std::filesystem::path &file) :
    file_{fs::weakly_canonical(file)},
    tmp_file_{fs::path{file_.string() + ".tmp"}}
{
    std::filesystem::create_directories(file_.parent_path());
    int fd = ::open(tmp_file_.c_str(), O_WRONLY | O_CREAT | O_EXCL, S_IRWXU);
    if(fd < 0) {
        if(errno == EEXIST) {
            throw std::runtime_error("File in use: "s + tmp_file_.string());
        } else {
            throw std::system_error(errno, std::generic_category(),
                    "Creating: "s + tmp_file_.string());
        }
    } else {
        ::close(fd);
    }
}

lock_overwrite_file::
lock_overwrite_file(lock_overwrite_file &&other) noexcept
{
    swap(*this, other);
}

lock_overwrite_file &lock_overwrite_file::
operator=(lock_overwrite_file &&other) noexcept
{
    lock_overwrite_file tmp{std::move(other)};
    swap(*this, tmp);
    return *this;
}

lock_overwrite_file::
~lock_overwrite_file()
{
    if(!tmp_file_.empty()) {
        std::error_code ec; // remove can throw unless we pass ec
        fs::remove(tmp_file_, ec);
    }
}

void lock_overwrite_file::
overwrite(std::function<void(std::ostream&)> writer)
{
    std::ofstream out(tmp_file_,
            std::ios::binary | std::ios::trunc | std::ios::out);
    out.exceptions(std::ios::badbit | std::ios::failbit);
    writer(out);
    out.close();
    fs::rename(tmp_file_, file_);
    tmp_file_.clear();
}

void swap(lock_overwrite_file &lhs, lock_overwrite_file &rhs) noexcept
{
    using std::swap;
    swap(lhs.file_, rhs.file_);
    swap(lhs.tmp_file_, rhs.tmp_file_);
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
