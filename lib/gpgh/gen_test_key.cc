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
#include <system_error>
#include <iostream>
#include <string>
#include <memory>
#include <functional>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>

namespace gpgh {

using namespace std::literals::string_literals;

using sem_up_t = std::unique_ptr<sem_t, std::function<void(sem_t*)>>;

static sem_up_t
gen_test_key_lock(void)
{
    const std::string sem_name =
        "/pwdb_gen_test_key-"s + std::to_string(geteuid());
    auto sem_p = sem_open(sem_name.c_str(), O_CREAT, S_IRUSR | S_IWUSR, 1);
    if(SEM_FAILED == sem_p)
        throw std::system_error(errno, std::generic_category(),
                "Opening semaphore "s + sem_name);
    if(sem_wait(sem_p)) {
        auto e = errno;
        sem_close(sem_p);
        throw std::system_error(e, std::generic_category(),
                "Waiting on semaphore"s + sem_name);
    }
    return sem_up_t{sem_p, [sem_name](sem_t *sem_p) {
        sem_post(sem_p);
        sem_unlink(sem_name.c_str());
        sem_close(sem_p);
    }};
}

gpgh::keylist
gen_test_key(gpgh::context &context, const std::string &recipient)
{
    gpgh::keylist keys;
    auto key_filter = [](gpgme_key_t k)->bool {
        return !k->revoked && !k->expired && k->can_encrypt && k->can_sign;
    };

    { // lock
        auto lock = gen_test_key_lock();
        // try to get the testing key
        keys = context.get_keys(recipient, false, key_filter);
        // create the testing key if it was not found
        if(keys.size() == 0) {
            std::cout << "Generating test key" << std::endl;
            auto gerr = gpgme_op_createkey(context.get(), recipient.c_str(),
                    "default", 0, 0, NULL, GPGME_CREATE_NOPASSWD);
            gpgh::gerr_check(gerr, __func__);
        }
    }

    if(keys.size() == 0) {
        keys = context.get_keys(recipient, false, key_filter);
        if(keys.size() == 0) {
            throw gpgh::error("No suitable key found for "s + recipient);
        }
    }
    // print the testing key
    for(auto k: keys) {
        std::cout << "Recipient:\n";
        gpgh::print_key(std::cout, *k, "\t");
    }

    return keys;
}

} // namespace gpgh
