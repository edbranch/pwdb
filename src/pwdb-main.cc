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

#include "pwdb/cl_options.h"
#include "pwdb/pwdb_cmd_interp.h"
#include "pwdb/db_utils.h"
#include "pwdb/util.h"
#include "pwdb/pb_gpg.h"
#include <fstream>
#include <iostream>
#include <list>
#include <system_error>
#include <filesystem>
#include <cerrno>
#include <cstdio>

void check_uid(const std::string &uid)
{
    auto keys = gpgh::context{}.get_keys(uid, false,
            [](gpgme_key_t k)->bool {return k->can_encrypt && k->can_sign;});
    if(keys.empty()) {
        std::cerr << "WARNING: No suitable key found for uid " << uid <<
            std::endl;
        std::cerr << "\tChanges will NOT be saved!" << std::endl;
    }
}

void check_gpg_verify_result(gpgh::context &ctx)
{
    using key_stat_t = std::tuple<bool, std::string, std::string>;
    std::list<key_stat_t> key_stats;
    for(auto sig = gpgme_op_verify_result(ctx.get())->signatures; sig != NULL;
            sig = sig->next)
    {
        key_stats.emplace_back(false, std::string{}, std::string{});
        auto &kstat = key_stats.back();
        if(sig->key) {
            std::get<0>(kstat) = true;
            std::get<1>(kstat) = sig->key->uids->uid;
        } else {
            std::get<1>(kstat) = sig->fpr;
        }

        if(sig->summary & GPGME_SIGSUM_VALID)
            std::get<2>(kstat) = "Signature %s good\n";
        else if(sig->summary & GPGME_SIGSUM_GREEN)
            std::get<2>(kstat) = "Signature %s ok\n";
        else if(sig->summary & GPGME_SIGSUM_RED)
            std::get<2>(kstat) = "WARNING: Signature %s invalid\n";
        else
            std::get<2>(kstat) =
                "WARNING: Signature %s could not be verified\n";
        break;
    }
    for(const auto &kstat: key_stats) {
        std::string uid{std::get<1>(kstat)};
        if(!std::get<0>(kstat)) {
            auto keys = ctx.get_keys(uid, false);
            if(!keys.empty())
                uid = keys.front()->get()->uids->uid;
        }
        std::printf(std::get<2>(kstat).c_str(), uid.c_str());
    }
}

int main(int argc, const char *argv[])
{
    try {
        // Parse command line options
        const auto opts = pwdb::cl_handle(argc, argv);
        if(!opts) {
            return 0;
        }

        // Status
        if(!opts->uid.empty())
            check_uid(opts->uid);
        std::cerr << (opts->create ? "Creating " : "Using ");
        std::cerr << opts->file << std::endl;

        // Read in the database
        pwdb::db cdb{};
        if(std::filesystem::exists(opts->file)) {
            if(std::ifstream ifs(opts->file); ifs.good()) {
                gpgh::context ctx{};
                cdb = pwdb::decode_data<pwdb::pb::DB>(ctx, ifs);
                check_gpg_verify_result(ctx);
            } else {
                throw std::system_error(errno, std::generic_category());
            }
        }

        // Set signing and primary encryption uid
        bool cdb_modified = false;
        if(!opts->uid.empty() && opts->uid != cdb.uid()) {
            if(!cdb.uid().empty() && !opts->recrypt) {
                std::cerr << "WARNING: uid has changed, recommend running with"
                   " --recrypt\n";
            }
            cdb.uid(opts->uid);
            cdb_modified = true;
        }
        check_uid(cdb.uid());

        // Recrypt
        // NOTE: do this anytime requested as new subkeys could have been added
        if(opts->recrypt) {
            std::cout << "Re-encrypting all record data stores" << std::endl;
            cdb_modified = true;
            gpgh::context ctx{};
            pwdb::db_recrypt_rcd_stores(ctx, cdb);
        }

        // Run command interpreter
        pwdb::pwdb_cmd_interp cmd_interp(cdb);
        cmd_interp.run("pwdb> ");
        cdb_modified = cdb_modified || cmd_interp.modified();

        // Save database
        if(cdb_modified) {
            std::cout << "Database modified, saving" << std::endl;
            auto encode = [&cdb](std::ostream& out) {
                gpgh::context ctx{};
                ctx.add_signer(cdb.uid());
                // TODO - additional recipients
                pwdb::encode_data(ctx, cdb.uid(), cdb.pb(), out, true);
            };
            pwdb::overwrite_file(opts->file, encode);
        }

    } catch(std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    std::cout << "Exiting" << std::endl;
    return 0;
}
