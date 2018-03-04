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
#include <iostream>
#include <sstream>

constexpr const char progname[] = "cmd_interp_test";

using namespace std::string_literals;
using vs = std::vector<std::string>;

bool tassert(bool pass, std::string desc)
{
    if(!pass) {
        std::cerr << "FAILED: " << desc << std::endl;
    }
    return !pass;
}

int
utils_test(void)
{
    using namespace cmd_interp;
    bool ret = 0;

    // split_args()
    ret |= tassert(split_args(""s) == vs{}, "split_args empty"s);
    ret |= tassert(split_args("foo"s) == vs{"foo"s}, "split_args single"s);
    ret |= tassert(split_args("foo bar \tbaz"s) == vs{"foo"s, "bar"s, "baz"s},
            "split_args whitespace"s);
    ret |= tassert(split_args("foo \"bar baz\""s) == vs{"foo"s, "bar baz"s},
            "split_args quoted"s);

    // assemble()
    ret |= tassert(assemble(vs{}) == ""s, "assemble empty"s);
    ret |= tassert(assemble(vs{"foo"s}) == "foo"s, "assemble single"s);
    ret |= tassert(assemble(vs{"foo"s, "bar"s, "baz"s}) == "foo bar baz"s,
            "assemble multi"s);

    return ret;
}

int
interp_test(void)
{
    using A = const std::vector<std::string>;
    using cmd_interp::interp;

    // interpreter definition
    std::stringstream cmds;
    auto interp = cmd_interp::interp{cmd_interp::istream_ops(cmds)};
    bool exit_res = false;
    interp["exit"] = { "Exit the interpreter",
        [&exit_res](A &args)->interp::result_t {
            exit_res = true;
            return interp::result_exit;
        }};
    size_t count_args_res = 0;
    interp["count_args"] = { "Count arguments",
        [&count_args_res](A &args)->interp::result_t {
            count_args_res = args.size();
            return interp::result_none;
        }};
    std::string echo_res{};
    interp["echo"] = { "Echo arguments",
        [&echo_res](A &args)->interp::result_t {
            echo_res = args.size() < 2 ? std::string{} :
                cmd_interp::assemble(++args.begin(), args.end());
            return interp::result_none;
        }};

    // tests
    bool ret = 0;

    cmds << "exit"s << std::endl;
    interp.run();
    ret |= tassert(exit_res, "exit"s);

    interp.handle("count_args foo \"bar baz\" bar baz"s);
    ret |= tassert(count_args_res == 5, "count args"s);

    {
        std::string line{"a line of text"s};
        cmds << "echo " << line << '\n' << "exit" << std::endl;
        interp.run();
        ret |= tassert(echo_res == line, "echo"s);
    }

    exit_res = false;
    cmds << "echo foo bar" << std::endl;
    cmds << "count_args foo bar baz" << std::endl;
    cmds << "exit" << std::endl;
    interp.run();
    ret |= tassert(exit_res && (echo_res == "foo bar"s) &&
            (count_args_res == 4), "sequence"s);

    return ret;
}

int
main(int argc, const char *argv[])
{
    if(argc < 2) {
        std::cerr << progname << ": No test to run" << std::endl;
        return 1;
    }
    std::string test_name(argv[1]);

    if(test_name == "utils_test")
        return utils_test();
    if(test_name == "interp_test")
        return interp_test();

    std::cerr << progname << ": No such test: " << test_name << std::endl;
    return 1;
}
