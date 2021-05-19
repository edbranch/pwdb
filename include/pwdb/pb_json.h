/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef pwdb_pb_json_h_included
#define pwdb_pb_json_h_included

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

#include <stdexcept>
#include <string>
#include <google/protobuf/util/json_util.h>

namespace pwdb {

//=============================================================================
// JSON Conversion
//=============================================================================

template <typename PB_T>
auto json2pb(const std::string &json)->PB_T
{
    google::protobuf::util::JsonParseOptions jopts;
    jopts.ignore_unknown_fields = false;
    PB_T msg;
    auto stat = google::protobuf::util::JsonStringToMessage(json, &msg, jopts);
    if(!stat.ok())
        throw std::runtime_error(std::string(stat.message()));
    return msg;
}

template <typename PB_T>
auto pb2json(const PB_T &msg)->std::string
{
    google::protobuf::util::JsonOptions jopts;
    jopts.preserve_proto_field_names = true;
    jopts.add_whitespace = true;
    std::string json;
    auto stat = google::protobuf::util::MessageToJsonString(msg, &json, jopts);
    if(!stat.ok())
        throw std::runtime_error(std::string(stat.message()));
    return json;
}

} // namespace pwdb
#endif //  pwdb_pb_json_h_included
