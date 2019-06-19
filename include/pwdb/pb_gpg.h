/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef pwdb_pb_gph_h_included
#define pwdb_pb_gph_h_included

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

#include "gpgh/gpg_helper.h"
#include <sstream>

namespace pwdb {

//=============================================================================
// Encrypt/Decrypt
//=============================================================================

template <typename PB_T>
auto decode_data(gpgh::context &ctx, std::istream &src)->PB_T
{
    PB_T ret;
    if(!ret.ParseFromString(ctx.decrypt(src))) {
        throw std::runtime_error(std::string("Failed to parse ") +
                typeid(PB_T).name());
    }
    return ret;
}

template <typename PB_T>
auto decode_data(gpgh::context &ctx, const std::string &src)->PB_T
{
    std::stringstream src_strm{src};
    return decode_data<PB_T>(ctx, src_strm);
}

template <typename PB_T>
void encode_data(gpgh::context &ctx,
        const std::vector<std::string> &recipients,
        const PB_T &msg, std::ostream &dest, bool sign=false)
{
    std::stringstream dec_data;
    if(!msg.SerializeToOstream(&dec_data)) {
        throw std::runtime_error(std::string("Failed to serialize ") +
                typeid(PB_T).name());
    }
    auto key_filter = [sign](gpgme_key_t k)->bool {
        return !k->revoked && !k->expired && k->can_encrypt &&
            (sign ? k->can_sign : true);
    };
    ctx.encrypt(ctx.get_keys(recipients, false, key_filter), dec_data, dest,
            sign);
}

template <typename PB_T>
void encode_data(gpgh::context &ctx,
        const std::string &recipient,
        const PB_T &msg, std::ostream &dest, bool sign=false)
{
    encode_data(ctx, std::vector<std::string>{recipient}, msg, dest, sign);
}

template <typename PB_T>
auto encode_data(gpgh::context &ctx,
        const std::vector<std::string> &recipients,
        const PB_T &msg, bool sign=false)->std::string
{
    std::stringstream dest;
    encode_data(ctx, recipients, msg, dest, sign);
    return dest.str();
}

template <typename PB_T>
auto encode_data(gpgh::context &ctx,
        const std::string &recipient, const PB_T &msg, bool sign=false)
{
    return encode_data(ctx, std::vector<std::string>{recipient}, msg, sign);
}

} // namespace pwdb
#endif //  pwdb_pb_gph_h_included
