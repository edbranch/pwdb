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

#include "pwdb/db.h"
#include <iostream>
#include <vector>
#include <functional>
#include <algorithm>

constexpr const char progname[] = "db_test";

bool tassert(bool pass, std::string desc)
{
    if(!pass) {
        std::cerr << "FAILED: " << desc << std::endl;
    }
    return !pass;
}

pwdb::db
gen_test_recordv(void)
{
    pwdb::db cdb{};

    pwdb::pb::Record rcd{};
    rcd.set_comment("www.record_one.com");
    *rcd.mutable_data() = "A heavily encrypted message";
    cdb.add("one", rcd);
    cdb.entag("one", "one two");

    rcd = pwdb::pb::Record{};
    rcd.set_comment("https://rcord_two.org");
    *rcd.mutable_data() = "I am an encrypted byte array, really";
    cdb.add("two", rcd);
    cdb.entag("two", "one two");
    cdb.entag("two", "two three");

    rcd = pwdb::pb::Record{};
    rcd.set_comment("ftp://ftp.record_three.net");
    *rcd.mutable_data() = "I am very sensitive information. Very!";
    cdb.add("three", rcd);
    cdb.entag("three", "two three");

    rcd = pwdb::pb::Record{};
    rcd.set_comment("ssh://ssh.terminal.co.uk");
    *rcd.mutable_data() = "If you can read this you will be RICH!";
    cdb.add("four", rcd);

    return cdb;
}

int
add_remove_test(void)
{
    bool ret = 0;
    // Add
    pwdb::db cdb{gen_test_recordv()};
    ret |= tassert(cdb.size() == 4, "Record add");
    ret |= tassert(cdb.at("one").comment() == "www.record_one.com", "at()");

    // Remove
    ret |= tassert(cdb.remove("three") == 1, "Call remove");
    ret |= tassert(cdb.size() == 3, "Size on remove");
    ret |= tassert(cdb.count("three") == 0, "Removed accessible");

    // Remove All
    for(auto size = cdb.size(); size != 0; --size) {
        ret |= tassert(cdb.remove(cdb.begin()->first), "Pop");
    }
    ret |= tassert(cdb.size() == 0, "Pop all");

    return ret;
}

int
tags_test(void)
{
    bool ret = 0;
    pwdb::db cdb{gen_test_recordv()};

    ret |= tassert(cdb.tags().size() > 0, "Add all");
    ret |= tassert(cdb.at_tag("nonexist").size() == 0, "Nonexistant tag");
    ret |= tassert(cdb.at_tag("one two").size() == 2, "Add tag to index");
    {
        auto t23 = cdb.at_tag("two three");
        std::set<std::string> names;
        for(auto rcd_name: t23)
            names.emplace(rcd_name);
        ret |= tassert(names.count("two") == 1 && names.count("three") == 1 &&
                names.count("one") == 0, "Tag index contents");
    }

    cdb.entag("two", "dynamic");
    cdb.entag("four", "dynamic");
    ret |= tassert(cdb.tags("two").count("dynamic") == 1, "Entag");
    ret |= tassert(cdb.at_tag("dynamic").size() == 2, "Entag index");
    cdb.detag("two", "dynamic");
    ret |= tassert(cdb.tags("two").count("dynamic") == 0, "Detag");
    ret |= tassert(cdb.at_tag("dynamic").size() == 1, "Detag index");

    cdb.remove("two");
    ret |= tassert(cdb.at_tag("one two").size() == 1 &&
            cdb.at_tag("two three").size() == 1 , "Remove tag from index");

    while(cdb.size())
        cdb.remove(cdb.begin()->first);
    ret |= tassert(cdb.tags().size() == 0, "Remove all");

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

    if(test_name == "add_remove")
        return add_remove_test();
    if(test_name == "tags")
        return tags_test();

    return 0;
}
