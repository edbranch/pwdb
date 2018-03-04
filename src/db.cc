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
#include <algorithm>

namespace pwdb {

//=============================================================================
// pwdb::pb::Record
//=============================================================================
void
stream_out(const pb::Record &rcd, std::ostream &out, unsigned indent)
{
    std::string prefix(indent, ' ');
    out << prefix << "comment: " << rcd.comment() << '\n';
    if(rcd.recipient_size()) {
        out << prefix << "recipients: ";
        for(int i=0; i != rcd.recipient_size(); ++i) {
            if(i != 0)
                out << ", ";
            out << rcd.recipient(i);
        }
        out << std::endl;
    }
}

//=============================================================================
// pwdb::db
//=============================================================================

unsigned db::
remove(const std::string &name)
{
    auto rcd_iter{records().find(name)};
    if(rcd_iter == records().end())
        return 0;
    for(auto tag_iter = pb_db.mutable_tags()->begin();
            tag_iter != pb_db.mutable_tags()->end(); ++tag_iter) {
        detag(name, tag_iter->first);
    }
    records().erase(rcd_iter);
    // NOTE: std::remove_if does not work on associative types
    for(auto i = pb_db.mutable_tags()->begin();
            i != pb_db.mutable_tags()->end(); ) {
        if(i->second.str().empty()) {
            i = pb_db.mutable_tags()->erase(i);
        } else {
            ++i;
        }
    }
    return 1;
}

const pb::Record &db::
at(const std::string &name) const
{
    auto rcd_iter(find(name));
    if(rcd_iter == crecords().cend()) {
        throw std::logic_error(std::string(__func__) + ": no such record: " +
                name);
    }
    return rcd_iter->second;
}

bool db::
entag(const std::string &name, const std::string &tag)
{
    auto rcd_count(records().count(name));
    if(!rcd_count)
        return false;
    auto tag_iter(pb_db.mutable_tags()->find(tag));
    if(tag_iter == pb_db.tags().end()) {
        (*pb_db.mutable_tags())[tag] = pb::Strlist{};
        tag_iter = pb_db.mutable_tags()->find(tag);
    }
    tag_iter->second.add_str(name);
    return true;
}

bool db::
detag(const std::string &name, const std::string &tag)
{
    auto tag_iter(pb_db.mutable_tags()->find(tag));
    if(tag_iter == pb_db.mutable_tags()->end())
        return false;
    if(!detag(name, tag_iter))
        return false;
    auto slp = tag_iter->second.mutable_str();
    if(slp->empty())
        pb_db.mutable_tags()->erase(tag_iter);
    return true;
}

std::vector<std::string> db::
at_tag(const std::string &tag) const
{
    auto tag_iter(pb_db.tags().find(tag));
    if(tag_iter == pb_db.tags().end())
        return std::vector<std::string>{};
    auto slp = tag_iter->second.str();
    return std::vector<std::string>(slp.begin(), slp.end());
}

bool db::
comment(const std::string &name, const std::string &cmt)
{
    auto rcd_iter(records().find(name));
    if(rcd_iter == records().end())
        return false;
    *rcd_iter->second.mutable_comment() = cmt;
    return true;
}

std::set<std::string> db::
tags(void) const
{
    std::set<std::string> ret;
    for(const auto &tag_val: pb_db.tags())
        ret.emplace(tag_val.first);
    return ret;
}

std::set<std::string> db::
tags(const std::string &name) const
{
    std::set<std::string> ret;
    for(const auto &tag_val: pb_db.tags()) {
        auto sl = tag_val.second.str();
        if(std::find(sl.begin(), sl.end(), name) != sl.end()) {
            ret.emplace(tag_val.first);
        }
    }
    return ret;
}

void db::
stream_out(std::ostream &out, unsigned indent) const
{
    std::string prefix(indent, ' ');
    out << prefix << "UID: " << pb_db.uid();
    out << std::endl;
    for(const auto &v: crecords()) {
        out << prefix << v.first << ": {\n";
        pwdb::stream_out(v.second, out, indent+4);
        out << prefix << '}' << std::endl;
    }
    out << prefix << "tags:\n";
    auto tag_prefix = prefix + prefix;
    for(auto i=pb_db.tags().begin(); i != pb_db.tags().end(); ++i) {
        out << tag_prefix << i->first << ": ";
        auto &sl = i->second;
        for(int r=0; r != sl.str_size(); ++r) {
            if(r != 0)
                out << ", ";
            out << sl.str(r);
        }
        out << std::endl;
    }
}

//-----------------------------------------------------------------------------
// pwdb::db private
//-----------------------------------------------------------------------------

bool db::
detag(const std::string &name, decltype(pb_db.mutable_tags()->begin()) tag_iter)
{
    auto slp = tag_iter->second.mutable_str();
    auto str_iter = std::find(slp->begin(), slp->end(), name);
    if(str_iter == slp->end())
        return false;
    slp->erase(str_iter);
    // NOTE: DO NOT DELETE EMPTY TAGS HERE!
    // It would invalidate tag_iter. The caller must be responsible for that.
    return true;
}

} // namespace pwdb
