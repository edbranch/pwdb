/* SPDX-License-Identifier: LGPL-3.0-or-later */
#ifndef cmd_interp_cmd_interp_h_included
#define cmd_interp_cmd_interp_h_included

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

#include <functional>
#include <map>
#include <vector>
#include <string>
#include <ostream>
#include <sstream>
#include <iomanip>
#include <optional>
#include <bitset>

namespace cmd_interp {

//-----------------------------------------------------------------------------
// Command line operations
//-----------------------------------------------------------------------------
struct ops
{
    using add_history_proto = void(const std::string &line);
    using get_proto = auto(const std::string &prompt)->
        std::optional<std::string>;

    std::function<add_history_proto> add_history;
    std::function<get_proto> get;
};

// readline command line source support
ops readline_ops(void);

// std::getline command line source support
ops istream_ops(std::istream &in);

//-----------------------------------------------------------------------------
// Command interpreter
//-----------------------------------------------------------------------------
class interp {
public:
    using result_t = std::bitset<2>;
    static constexpr result_t result_none = 0u;
    static constexpr result_t result_exit = 1u << 0;
    static constexpr result_t result_add_history = 1u << 1;

    struct cmd_def
    {
        std::string help;
        std::function<result_t(const std::vector<std::string> &)> handle;
    };
private:
    cmd_interp::ops ops_ = readline_ops();
    std::map<std::string, cmd_def> interp_;
public:
    interp(void) = default;
    interp(const cmd_interp::ops &ops) : ops_{ops} { ; }
    cmd_def &operator[](std::string cmd) { return interp_[cmd]; }
    bool handle(const std::string &cmdline) const;
    void run(const std::string &prompt=std::string{}) const;
    void add_history(const std::string &cmdline) const
        { ops_.add_history(cmdline); }
    void help(std::ostream &out, const std::string &cmd) const;
    void help(std::ostream &out) const;
};

//-----------------------------------------------------------------------------
// Utilities
//-----------------------------------------------------------------------------
auto split_args(const std::string &cmdline)->std::vector<std::string>;
template<typename inputIter>
auto assemble(const inputIter begin, const inputIter end)->std::string {
    std::stringstream cmdline;
    for(auto i = begin; i != end; ++i) {
        if(i != begin)
            cmdline << " ";
        cmdline << *i;
    }
    return cmdline.str();
}
auto assemble(const std::vector<std::string> &args)->std::string;

template<typename InputIterator>
void print_columns(std::ostream &out, InputIterator first, InputIterator last,
        std::string separator = " ", std::string prefix = "")
{
    std::vector<size_t> widths;
    for(auto r=first; r!=last; ++r) {
        size_t c = 0;
        for(auto ci = r->begin(); ci != r->end(); ++ci, ++c) {
            if(c == widths.size())
                widths.push_back(0);
            widths[c] = std::max(widths[c], ci->size());
        }
    }
    for(auto r=first; r!=last; ++r) {
        size_t c = 0;
        out << prefix;
        for(auto ci = r->begin(); ci != r->end(); ++ci, ++c) {
            if(c != 0)
                out << separator;
            out << std::setw(widths[c]) << std::left << *ci;
        }
        out << std::endl;
    }
}

} // namespace cmd_interp
#endif // cmd_interp_cmd_interp_h_included
