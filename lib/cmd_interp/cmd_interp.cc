/* SPDX-License-Identifier: LGPL-3.0-or-later */
/***
    This file is part of pwdb.

    Copyright (C) 2018 Edward Branch

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
    more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program. If not, see <https://www.gnu.org/licenses/>.
***/

#include "cmd_interp/cmd_interp.h"
#include <boost/program_options/parsers.hpp>
#include <iostream>
#include <iomanip>
#include <algorithm>
extern "C" {
#include <readline/readline.h>
#include <readline/history.h>
} // extern "C"

namespace cmd_interp {

std::vector<std::string>
split_args(const std::string &cmdline)
{
    return boost::program_options::split_unix(cmdline);
}

std::string
assemble(const std::vector<std::string> &args)
{
    return assemble(args.begin(), args.end());
}

//----------------------------------------------------------------------------
// GNU Readline support
//----------------------------------------------------------------------------
ops
readline_ops(void)
{
    return ops {
        [](const std::string &line)->void {
            ::add_history(line.c_str());
        },
        [](const std::string &prompt)->std::optional<std::string> {
            std::optional<std::string> cmd{};
            if(char *lp = readline(prompt.c_str()); lp != nullptr) {
                cmd = lp;
                free(lp);
            }
            return std::move(cmd);
        }
    };
}

//----------------------------------------------------------------------------
// std::getline command line source support
//----------------------------------------------------------------------------
ops
istream_ops(std::istream &in)
{
    return ops{
        [](const std::string&) { ; },
        [&in](const std::string &prompt)->std::optional<std::string> {
            std::cout << prompt << std::flush;
            std::string line;
            return std::getline(in, line).good() ? line :
                std::optional<std::string>{};
        }
    };
}

//----------------------------------------------------------------------------
// class interp
//----------------------------------------------------------------------------

bool interp::
handle(const std::string &cmdline) const
{
    auto args = cmd_interp::split_args(cmdline);
    if(args.empty())
        return true;
    auto cmd = args.front();
    if(cmd == "help") {
        args.size() == 1 ? help(std::cout) : help(std::cout, args[1]);
        add_history(cmdline);
        return true;
    }
    auto cmd_def = interp_.find(cmd);
    if(cmd_def == interp_.end()) {
        std::cout << "Command Not Found: \"" << cmd << "\"\n";
        std::cout << "\tEnter \"help\" for available commands" << std::endl;
        add_history(cmdline);
        return true;
    }
    auto rv = cmd_def->second.handle(args);
    if((rv & result_add_history).any()) {
        add_history(cmdline);
    }
    return !(rv & result_exit).any();
}

void interp::
run(const std::string &prompt) const
{
    while(true) {
        auto cmd = ops_.get(prompt);
        if(!cmd)
            break;
        if(cmd->empty())
            continue;
        if(!this->handle(*cmd)) {
            break;
        }
    }
}

void interp::
help(std::ostream &out, const std::string &cmd) const
{
    out << "Help: " << cmd << " " << interp_.at(cmd).help << std::endl;
}

void interp::
help(std::ostream &out) const
{
    out << "Commands:\n";
    auto prefix = "    ";
    size_t cmd_width = 0;
    for(const auto cdef: interp_)
        cmd_width = std::max(cmd_width, cdef.first.size());
    for(const auto cdef: interp_)
        out << prefix << std::setw(cmd_width) << std::left << cdef.first <<
            " - " << cdef.second.help << '\n';
    out << std::flush;
}

} // namespace cmd_interp
