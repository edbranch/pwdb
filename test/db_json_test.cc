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

#include "pwdb/pb_json.h"
#include "pwdb/db.h"
#include <iostream>
#include <sstream>
#include <string>
#include <functional>

static constexpr char progname[] = "db_json_test";

static constexpr char proto_db1_json[] = R"json(
{
    "uid": "default_r1@mail.com",
    "records": {
        "empty": { },
        "data_only": {
            "data": "QSByZWFsbHkgcmVhbGx5IGJpZyBzZWNyZXQhIgo="
        },
        "complete_a_b": {
            "comment": "www.complete_a_b.com",
            "recipient": ["a_b_r1@mail.com", "a_b_r2@mail.net"],
            "data":
                "U29tZW9uZSBtdXN0IGhhdmUgZm91bmQgYSBjb2xsaXNpb24gYWxnb3JpdGhtIQo="
        },
        "complete_b_c": {
            "comment": "www.complete_b_c.org",
            "recipient": ["b_c_r1@mail.com"],
            "data": "SSBndWVzcyBJIGFtIG5vdCBmb29saW5nIGFueW9uZSwgYW0gST8K"
        }
    },
    "tags": {
        "a": { "str": ["complete_a_b"] },
        "b": { "str": ["complete_a_b", "complete_b_c"] },
        "c": { "str": ["complete_b_c"] }
    }
})json";

bool tassert(std::function<bool(void)> test, std::string desc)
{
    bool pass;
    try {
        pass = test();
    } catch(const std::exception &e) {
        std::cerr << "EXCEPTION: " << desc << std::endl;
        throw;
    }
    if(!pass) {
        std::cerr << "FAILED: " << desc << std::endl;
    }
    return !pass;
}

bool basic_test(void)
{
    bool ret = 0;

    // proto_db1 tests
    pwdb::db cdb;
    ret |= tassert([&cdb]()->bool {
            cdb = pwdb::json2pb<pwdb::pb::DB>(proto_db1_json);
            return true;
        }, "Import db from JSON");
    ret |= tassert([&cdb]()->bool {
            std::cout << pwdb::pb2json(cdb.pb());
            return true;
        }, "Export db to JSON");
    ret |= tassert([&cdb]()->bool { return cdb.size() == 4; } ,
            "Number of records");
    ret |= tassert([&cdb]()->bool {
            return cdb.count("complete_a_b") != 0;
        }, "Access \"complete_a_b\"");
    ret |= tassert([&cdb]()->bool {
            return cdb.at("complete_a_b").comment() == "www.complete_a_b.com";
        }, "\"complete_a_b\" comment");
    ret |= tassert([&cdb]()->bool {
            return cdb.tags("complete_a_b").size() == 2;
        }, "\"complete_a_b\" tags size");

    return ret;
}

int main(int argc, const char *argv[])
{
    if(argc < 2) {
        std::cerr << progname << ": No test to run" << std::endl;
        return 1;
    }
    std::string test_name(argv[1]);

    if(test_name == "basic")
        return basic_test();

    return 0;
}
