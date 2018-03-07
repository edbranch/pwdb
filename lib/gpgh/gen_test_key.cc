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

#include "gpgh/gpg_helper.h"
#include <iostream>

namespace gpgh {

gpgh::keylist gen_test_key(gpgh::context &context, const std::string &recipient)
{
    // FIXME: existence of the key is a not proper mutex
    // try to get the testing key
    auto key_filter = [](gpgme_key_t k)->bool
        { return !k->revoked && !k->expired && k->can_encrypt; };
    auto keys = context.get_keys(recipient, false, key_filter);
    // create the testing key if it was not found and then get it
    if(keys.size() == 0) {
        std::cout << "Generating test key" << std::endl;
        auto gerr = gpgme_op_createkey(context.get(), recipient.c_str(),
                "default", 0, 0, NULL, GPGME_CREATE_NOPASSWD);
        gpgh::gerr_check(gerr, __func__);
        keys = context.get_keys(recipient, false, key_filter);
    }

    if(keys.size() == 0) {
        throw gpgh::error(std::string("No suitable key found for ") + recipient);
    }
    // check and print the testing key
    for(auto k: keys) {
        std::cout << "Recipient:\n";
        gpgh::print_key(std::cout, *k, "\t");
    }

    return keys;
}

} // namespace gpgh
