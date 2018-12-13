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
#include <sstream>
#include <fstream>
#include <system_error>
#include <filesystem>

namespace fs = ::std::filesystem;

int main(int argc, const char *argv[])
{
    std::string test(argv[1]);
    std::string data_src, data_dest;

    // testing home directory and uid
    auto gpg_home = fs::current_path().append("gnupg");
    std::cout << "GPG Set to use homedir=" << gpg_home << std::endl;
    fs::create_directory(gpg_home);
    std::string recipient("ctest@testson.name");

    try {
        // create testing context pointed at testing home directory
        gpgme_error_t gerr;
        gpgh::context context{};
        gerr = gpgme_ctx_set_engine_info(context.get(), GPGME_PROTOCOL_OpenPGP,
                nullptr, gpg_home.c_str());
        gpgh::gerr_check(gerr, __func__);

        // try to get the testing key
        auto key_filter = [](gpgme_key_t k)->bool
            { return !k->revoked && !k->expired && k->can_encrypt; };
        auto keys = context.get_keys(recipient, false, key_filter);
        // create the testing key if it was not found and then get it
        if(keys.size() == 0) {
            std::cout << "Generating test key" << std::endl;
            gerr = gpgme_op_createkey(context.get(), recipient.c_str(),
                    "default", 0, 0, NULL, GPGME_CREATE_NOPASSWD);
            gpgh::gerr_check(gerr, __func__);
            keys = context.get_keys(recipient, false, key_filter);
        }

        // check and print the testing key
        for(auto k: keys) {
            std::cout << "Recipient:\n";
            gpgh::print_key(std::cout, *k, "\t");
        }
        if(keys.size() == 0) {
            std::cerr << "No suitable key found for " << recipient << std::endl;
            return 1;
        }

        //=====================================================================
        // Tests
        //=====================================================================
        if(test == "encrypt2file") {
            data_src = "encrypt2file content\n";
            std::string cipher_path{"cipher"};
            // encrypt file
            {
                std::ofstream dest_strm{cipher_path};
                if(!dest_strm) {
                    std::cerr << "Error opening " << cipher_path << std::endl;
                    throw std::system_error(errno, std::system_category());
                }
                context.encrypt(keys, data_src, dest_strm);
            }
            // decrypt the file back
            {
                std::ifstream src_strm{cipher_path};
                if(!src_strm) {
                    std::cerr << "Error opening " << cipher_path << std::endl;
                    throw std::system_error(errno, std::system_category());
                }
                data_dest = context.decrypt(src_strm);
            }
            std::cout << "encrypt2file decrypt: " << data_dest.size() <<
                " bytes read\n";
            std::cout << "Content:\n--------\n" << data_dest <<
                "\n--------" << std::endl;
        }
        else if(test == "encrypt2string") {
            data_src = "encrypt2string content\n";
            // encrypt string
            std::string cipher = context.encrypt(keys, data_src);
            // decrypt the string back
            data_dest = context.decrypt(cipher);
            std::cout << "encrypt2string decrypt: " << data_dest.size() <<
                " bytes read\n";
            std::cout << "Content:\n--------\n" << data_dest <<
                "\n--------" << std::endl;
        }
        else {
            std::cerr << "Unrecognized test" << std::endl;
        }

        if(data_dest != data_src) {
            std::cerr << "ROUNDTRIP ERROR - data mismatch" << std::endl;
            return 1;
        }

    } catch(const gpgh::error &e) {
        std::cerr << "GPG ERROR: " << e.what() << std::endl;
        return 1;
    } catch(const std::system_error &e) {
        std::cerr << "SYSTEM ERROR: " << e.what() << std::endl;
        return 1;
    } catch(const std::exception &e) {
        std::cerr << "UNKNOWN ERROR: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
