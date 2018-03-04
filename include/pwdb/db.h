/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef _pwdb_db_h_
#define _pwdb_db_h_

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

#include "pwdb/pwdb.pb.h"
#include <map>
#include <set>
#include <vector>
#include <string>
#include <iostream>

namespace pwdb {

void stream_out(const pb::Record &rcd, std::ostream &out, unsigned indent=0);

class db
{
public:
    using rcd_iter_t = decltype(pb::DB{}.mutable_records()->begin());
    using rcd_citer_t = decltype(pb::DB{}.records().begin());

private:
    pb::DB pb_db;

    auto &records(void) { return *pb_db.mutable_records(); }
    const auto &crecords(void) const { return pb_db.records(); }
    bool detag(const std::string &name,
            decltype(pb_db.mutable_tags()->begin()) tag_iter);

public:
    db(void) = default;
    db(pb::DB &&p) : pb_db(p) { ; }
    db(std::istream &in) { pb_db.ParseFromIstream(&in); }
    db(std::istream &&in) : db{in} {}
    db(const db &) = delete;
    db(db &&) = default;
    db &operator=(const db &) = delete;
    db &operator=(db &&) = default;
    db &operator=(pb::DB &&p) { pb_db = std::move(p); return *this; }

    auto uid(void) const->std::string
        { return pb_db.uid(); }
    void uid(const std::string &id)
        { *pb_db.mutable_uid() = id; }
    void add(const std::string &name, const pb::Record &rcd)
        { records()[name] = rcd; }
    void add(const std::string &name, pb::Record &&rcd = pb::Record{})
        { records()[name] = std::move(rcd); }
    auto remove(const std::string &name)->unsigned;
    auto count(const std::string &name) const
        { return pb_db.records().count(name); }
    auto find(const std::string &name) const->rcd_citer_t
        { return crecords().find(name); }
    auto at(const std::string &name) const->const pb::Record&;
    auto get_data(const std::string &name) const->const std::string&
        { return pb_db.records().at(name).data(); }
    void set_data(const std::string &name, const std::string &data) {
        pb_db.mutable_records()->at(name).set_data(data);
    }
    void set_data(const std::string &name, std::string &&data) {
        pb_db.mutable_records()->at(name).set_data(std::move(data));
    }
    bool entag(const std::string &name, const std::string &tag);
    bool detag(const std::string &name, const std::string &tag);
    auto at_tag(const std::string &tag) const->std::vector<std::string>;
    bool comment(const std::string &name, const std::string &cmt);
    auto begin(void) const { return pb_db.records().cbegin(); }
    auto end(void) const { return pb_db.records().cend(); }
    auto size(void) const { return pb_db.records_size(); }
    auto tags(void) const->std::set<std::string>;
    auto tags(const std::string &name) const->std::set<std::string>;
    auto pb(void) const->const pwdb::pb::DB& { return pb_db; }
    void stream_out(std::ostream &out, unsigned indent=0) const;
};

} // namespace pwdb
#endif //  _pwdb_db_h_
