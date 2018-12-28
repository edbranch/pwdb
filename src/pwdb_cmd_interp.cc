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

#include "pwdb/pwdb_cmd_interp.h"
#include "pwdb/pb_json.h"
#include "pwdb/util.h"
#include "pwdb/db_utils.h"
#include <iostream>
#include <deque>
#include <array>

namespace pwdb {

//-----------------------------------------------------------------------------
// pwdb_cmd_interp
//-----------------------------------------------------------------------------
cmd_interp::interp pwdb_cmd_interp::
def_interp(const cmd_interp::ops &ops)
{
    using A = const std::vector<std::string>;
    using cmd_interp::interp;

    cmd_interp::interp d{ops};
    d["exit"] = { "Exit the program",
        [](A &args)->interp::result_t {
            return interp::result_exit;
        }
    };
    d["echo"] = { "Echo command arguments",
        [this](A &args)->interp::result_t {
            std::cout << cmd_interp::assemble(args) << std::endl;
            return interp::result_add_history;
        }
    };
    d["list"] = { "([<TAG>]) Lists records optionally filtered by <TAG>",
        [this](A &args)->interp::result_t {
            if(cdb_.size() == 0)
                return interp::result_add_history;
            std::deque<std::array<std::string, 2>> das{};
            if(args.size() == 1) {
                for(const auto &entry: cdb_)
                    das.push_back({entry.first, entry.second.comment()});
            }
            else {
                auto names = cdb_.at_tag(args[1]);
                if(names.empty()) {
                    std::cerr << args[1] << ": No such tag" << std::endl;
                    return interp::result_add_history;
                }
                for(const auto &n: names) {
                    if(cdb_.count(n) == 0) {
                        std::cerr << "ERROR: Index corruption at tag: " <<
                            args[1] << " record: " << n << std::endl;
                        continue;
                    }
                    das.push_back({n, cdb_.at(n).comment()});
                }
            }
            std::sort(das.begin(), das.end());
            cmd_interp::print_columns(std::cout, das.cbegin(), das.cend(),
                    "  ", "  ");
            return interp::result_add_history;
        }
    };
    d["add"] = { "(<NAME> [COMMENT]) Add new record NAME and set COMMENT",
        [this](A &args)->interp::result_t {
            if(args.size() < 2) {
                std::cerr << "Missing required argument" << std::endl;
                interp_.help(std::cerr, args.at(0));
                return interp::result_add_history;
            }
            const auto &rcd_name(args.at(1));
            if(cdb_.count(rcd_name) != 0) {
                std::cerr << "Record exists" << std::endl;
                return interp::result_add_history;
            }
            pb::Record rcd;
            *rcd.mutable_comment() = cmd_interp::assemble(args.begin()+2,
                    args.end());
            cdb_.add(rcd_name, std::move(rcd));
            modified_ = true;
            return interp::result_add_history;
        }
    };
    d["remove"] = { "(<NAME>) Remove record NAME",
        [this](A &args)->interp::result_t {
            if(args.size() != 2) {
                std::cerr << "Incorrect number of arguments" << std::endl;
                interp_.help(std::cerr, args.at(0));
                return interp::result_add_history;
            }
            if(cdb_.remove(args.at(1))) {
                modified_ = true;
            } else {
                std::cerr << "No such record" << std::endl;
            }
            return interp::result_add_history;
        }
    };
    d["open"] = { "(<NAME>) Open the data store of record NAME",
        [this](A &args)->interp::result_t {
            if(args.size() < 2) {
                std::cerr << "Missing required argumant <NAME>" << std::endl;
                interp_.help(std::cerr, args.at(0));
                return interp::result_add_history;
            }
            const auto &name = args.at(1);
            auto rcd_iter = cdb_.find(name);
            if(cdb_.end() == rcd_iter) {
                std::cerr << "No such record" << std::endl;
                return interp::result_add_history;
            }
            gpgh::context ctx{};
            rcd_cmd_interp rcd_interp{db_open_rcd_store(ctx, cdb_, rcd_iter),
                interp_.ops()};
            {
                // Use alternate terminal buffer when record is open
                pwdb::term_mode tmode{};
                rcd_interp.print();
                rcd_interp.run(name + "> ");
            }
            if(rcd_interp.modified()) {
                std::cout << "Encrypting and closing " << name << std::endl;
                db_save_rcd_store(ctx, cdb_, name, rcd_interp.store());
                modified_ = true;
            } else {
                std::cout << "No modification, closing " << name << std::endl;
            }
            return interp::result_add_history;
        }
    };
    d["comment"] = { "(<NAME> [<COMMENT>]) Set COMMENT of record NAME",
        [this](A &args)->interp::result_t {
            if(args.size() < 2) {
                std::cerr << "Missing required argumant <NAME>" << std::endl;
                interp_.help(std::cerr, args.at(0));
                return interp::result_add_history;
            }
            std::string comment = cmd_interp::assemble(args.begin()+2,
                    args.end());
            if(cdb_.comment(args[1], comment)) {
                modified_ = true;
            } else {
                std::cerr << "No such record" << std::endl;
            }
            return interp::result_add_history;
        }
    };
    d["tag"] = { "(<NAME> <TAG>) Tag record <NAME> with <TAG>",
        [this](A &args)->interp::result_t {
            if(args.size() != 3) {
                std::cerr << "Incorrect number of arguments" << std::endl;
                interp_.help(std::cerr, args.at(0));
                return interp::result_add_history;
            }
            if(cdb_.entag(args[1], args[2])) {
                modified_ = true;
            } else {
                std::cerr << "No such record" << std::endl;
            }
            return interp::result_add_history;
        }
    };
    d["detag"] = { "(<NAME> <TAG>) Remove <TAG> from record <NAME>",
        [this](A &args)->interp::result_t {
            if(args.size() != 3) {
                std::cerr << "Incorrect number of arguments" << std::endl;
                interp_.help(std::cerr, args.at(0));
                return interp::result_add_history;
            }
            if(cdb_.detag(args[1], args[2])) {
                modified_ = true;
            } else {
                std::cerr << "No such record" << std::endl;
            }
            return interp::result_add_history;
        }
    };
    d["tags"] = { "Print all known tags",
        [this](A &args)->interp::result_t {
            if(args.size() != 1) {
                std::cerr << "Incorrect number of arguments" << std::endl;
                interp_.help(std::cerr, args.at(0));
                return interp::result_add_history;
            }
            auto tags = cdb_.tags();
            for(auto ti = tags.begin(); ti != tags.end(); ++ti) {
                if(ti != tags.begin())
                    std::cout << ", ";
                std::cout << *ti;
            }
            std::cout << std::endl;
            return interp::result_add_history;
        }};
    d["dump"] = { "([<NAME> ...] Dump database or records to terminal",
        [this](A &args)->interp::result_t {
            if(args.size() == 1) {
                cdb_.stream_out(std::cout, 4);
                return interp::result_add_history;
            }
            for(size_t i = 1; i < args.size(); ++i) {
                constexpr unsigned indent = 4;
                std::string prefix(indent, ' ');
                auto rcd_name = args.at(i);
                std::cout << prefix << rcd_name << ": ";
                if(cdb_.count(rcd_name) == 0)
                    std::cout << "NULL" << std::endl;
                else {
                    std::cout << "{\n";
                    stream_out(cdb_.at(rcd_name), std::cout, indent+4);
                    std::cout << prefix << "}" << std::endl;
                }
            }
            return interp::result_add_history;
        }
    };

    return std::move(d);
}

pwdb_cmd_interp::
pwdb_cmd_interp(pwdb::db &cdb, const cmd_interp::ops &ops) :
    cdb_{cdb},
    interp_{def_interp(ops)}
{ ; }

//-----------------------------------------------------------------------------
// rcd_cmd_interp
//-----------------------------------------------------------------------------
cmd_interp::interp rcd_cmd_interp::
def_interp(const cmd_interp::ops &ops)
{
    using A = const std::vector<std::string>;
    using cmd_interp::interp;

    cmd_interp::interp d{ops};
    d["exit"] = { "Exit the program",
        [](A &args)->interp::result_t {
            return interp::result_exit;
        }
    };
    d["echo"] = { "Echo command arguments",
        [this](A &args)->interp::result_t {
            std::cout << cmd_interp::assemble(args) << std::endl;
            return interp::result_add_history;
        }
    };
    d["set"] = { "(<KEY> [<VALUE>]) Set record key/value",
        [this](A &args)->interp::result_t {
            if(args.size() < 2) {
                std::cerr << "Missing required argumant <KEY>" << std::endl;
                interp_.help(std::cerr, args.at(0));
                return interp::result_add_history;
            }
            modified_ = true;
            (*store_.mutable_values())[args[1]] =
                cmd_interp::assemble(args.begin()+2, args.end());
            return interp::result_none;
        }
    };
    d["unset"] = { "(<KEY>) Unset record key",
        [this](A &args)->interp::result_t {
            if(args.size() < 2) {
                std::cerr << "Missing required argumant <KEY>" << std::endl;
                interp_.help(std::cerr, args.at(0));
                return interp::result_add_history;
            }
            auto i = store_.mutable_values()->find(args[1]);
            if(i == store_.mutable_values()->end()) {
                std::cerr << "Key " << args[1] << " is not set" << std::endl;
                return interp::result_add_history;
            }
            modified_ = true;
            store_.mutable_values()->erase(i);
            return interp::result_add_history;
        }
    };
    d["print"] = { "([<KEY>]...) Print key/values filtered by <KEY>s",
        [this](A &args)->interp::result_t {
            if(args.size() == 1) {
                this->print();
            } else {
                std::deque<std::array<std::string, 2>> das{};
                for(auto ki = ++args.begin(); ki != args.end(); ++ki) {
                    auto i = store_.values().find(*ki);
                    if(i == store_.values().end()) {
                        std::cerr << "Key " << *ki << " is not set\n";
                    } else {
                        das.push_back({*ki, i->second});
                    }
                }
                this->print_columns(das.cbegin(), das.cend());
            }
            return interp::result_add_history;
        }
    };

    return std::move(d);
}

rcd_cmd_interp::
rcd_cmd_interp(const pwdb::pb::Store &store, const cmd_interp::ops &ops) :
    store_{store}, interp_{def_interp(ops)}
{ ; }

void rcd_cmd_interp::
print(void) const
{
    std::deque<std::array<std::string, 2>> das{};
    for(const auto &entry: store_.values())
        das.push_back({entry.first, entry.second});
    std::sort(das.begin(), das.end());
    this->print_columns(das.cbegin(), das.cend());
}

} // namespace pwdb
