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
#include "pwdb/pb_json.h"
#include <fstream>
#include <iostream>
#include <format>
#include <system_error>
#include <filesystem>

using namespace std::literals::string_literals;
namespace fs = std::filesystem;

void check_uid(gpgh::context &ctx, const std::string &uid)
{
    auto keys = ctx.get_keys(uid, false,
            [](gpgme_key_t k)->bool {return k->can_encrypt && k->can_sign;});
    if(keys.empty()) {
        std::cerr << "WARNING: No suitable key found for uid " << uid <<
            std::endl;
        std::cerr << "\tChanges will NOT be saved!\n";
    }
}

void check_gpg_verify_result(gpgh::context &ctx)
{
    auto siglist = ctx.op_verify_result();
    for(const auto &sig: siglist) {
        std::string uid("<unknown>");
        if(sig.key != nullptr)
            uid = sig.key->uids->uid;

        if(sig.summary & GPGME_SIGSUM_VALID)
            std::cout << std::format("Signature {} good\n", uid);
        else if(sig.summary & GPGME_SIGSUM_GREEN)
            std::cout << std::format("Signature {} ok\n", uid);
        else if(sig.summary & GPGME_SIGSUM_RED)
            std::cout << std::format("WARNING: Signature {} invalid\n", uid);
        else
            std::cout << std::format("WARNING: Signature {} could not be "
                    "verified\n", uid);
        std::cout << std::flush;
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
        bool do_recrypt = opts->recrypt;

        // Status
        if(!opts->uid.empty()) {
            gpgh::context ctx{opts->gpg_homedir};
            check_uid(ctx, opts->uid);
        }

        pwdb::lock_overwrite_file db_file_lock{fs::path(opts->file)};
        auto db_file = db_file_lock.file().string();

        // Check opts->create vs db_file existence
        bool db_file_exists = fs::exists(db_file);
        if(opts->create && db_file_exists) {
            throw std::runtime_error("File exists: "s + db_file);
        } else if(!opts->create && !db_file_exists) {
            throw std::runtime_error("File does not exist: "s + db_file);
        }
        std::cout << (opts->create ? "Creating " : "Using ") << db_file <<
            std::endl;

        // Create and initialize the database
        pwdb::db cdb{};
        if(opts->import_file.empty()) {
            if(db_file_exists) {
                std::ifstream ifs(db_file, std::ios::in | std::ios::binary);
                ifs.exceptions(std::ios::badbit | std::ios::failbit);
                gpgh::context ctx{opts->gpg_homedir};
                cdb = pwdb::decode_data<pwdb::pb::DB>(ctx, ifs);
                check_gpg_verify_result(ctx);
            }
        } else {
            std::cout << "Importing " << opts->import_file << std::endl;
            std::ifstream ifs(opts->import_file,
                    std::ios::in | std::ios::binary);
            ifs.exceptions(std::ios::badbit | std::ios::failbit);
            gpgh::context ctx{opts->gpg_homedir};
            cdb = pwdb::json2pb<pwdb::pb::DB>(ctx.decrypt(ifs));
            check_gpg_verify_result(ctx);
            do_recrypt = true;
        }

        // Set signing and primary encryption uid
        bool cdb_modified = false;
        if(!opts->uid.empty() && opts->uid != cdb.uid()) {
            if(!cdb.uid().empty() && !do_recrypt) {
                std::cerr << "WARNING: uid has changed, recommend running with"
                   " --recrypt\n";
            }
            cdb.uid(opts->uid);
            cdb_modified = true;
        }
        {
            gpgh::context ctx{opts->gpg_homedir};
            check_uid(ctx, cdb.uid());
        }

        // Recrypt
        // NOTE: do this anytime requested as new subkeys could have been added
        if(do_recrypt) {
            std::cout << "Re-encrypting all record data stores" << std::endl;
            cdb_modified = true;
            gpgh::context ctx{opts->gpg_homedir};
            pwdb::db_recrypt_rcd_stores(ctx, cdb);
        }

        // Export
        // FIXME: if both --recrypt and --export-file are given, re-encrypted
        // database will not be saved, only exported.
        if(!opts->export_file.empty()) {
            std::cout << "Exporting " << opts->export_file << std::endl;
            std::ofstream ofs(opts->export_file,
                    std::ios::out | std::ios::binary);
            ofs.exceptions(std::ios::badbit | std::ios::failbit);
            fs::permissions(opts->export_file,
                    fs::perms::owner_read | fs::perms::owner_write);
            pwdb::db expdb{cdb.copy()};
            gpgh::context ctx{opts->gpg_homedir};
            ctx.add_signer(expdb.uid());
            db_decrypt_all_rcd_stores(ctx, expdb);
            auto json = pwdb::pb2json(expdb.get_db());
            auto keyfilt = [](gpgme_key_t k)->bool {
                return !k->revoked && !k->expired && k->can_encrypt &&
                    k->can_sign;
            };
            auto keys = ctx.get_keys(expdb.uid(), false, keyfilt);
            ctx.encrypt(keys, json, ofs, true);
            return 0;
        }

        // Run command interpreter
        pwdb::pwdb_cmd_interp cmd_interp(cdb);
        cmd_interp.run("pwdb> ");
        cdb_modified = cdb_modified || cmd_interp.modified();

        // Save database
        if(cdb_modified) {
            std::cout << "Database modified, saving" << std::endl;
            auto encode = [&cdb, &opts](std::ostream& out) {
                gpgh::context ctx{opts->gpg_homedir};
                ctx.add_signer(cdb.uid());
                // TODO - additional recipients
                pwdb::encode_data(ctx, cdb.uid(), cdb.pb(), out, true);
            };
            db_file_lock.overwrite(encode);
        }

    } catch(const std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    std::cout << "Exiting" << std::endl;
    return 0;
}
