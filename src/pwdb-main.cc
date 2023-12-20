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
#include <list>
#include <system_error>
#include <filesystem>
#include <cerrno>

extern "C" {
#include <fcntl.h>
}

void check_uid(gpgh::context &ctx, const std::string &uid)
{
    auto keys = ctx.get_keys(uid, false,
            [](gpgme_key_t k)->bool {return k->can_encrypt && k->can_sign;});
    if(keys.empty()) {
        std::cerr << std::format("WARNING: No suitable key found for uid {}\n",
                uid);
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

        // Canonicalize path (in main because we need it later)
        auto db_file = std::filesystem::weakly_canonical(opts->file);
        // Create parent directories
        std::filesystem::create_directories(db_file.parent_path());
        // The tmp file **is** the lock. The flock() and fcntl() locking
        // mechanisms are inherently racey WRT the write-to-temp-move semantics
        // of a modified database. We can use exclusive creation of the emp
        // file to close this race window.
        std::filesystem::path tmp_file{db_file.string() + ".tmp"};
        int fd = ::open(tmp_file.c_str(), O_WRONLY | O_CREAT | O_EXCL, S_IRWXU);
        if(errno == EEXIST)
            throw std::runtime_error(std::format("File in use: {}",
                        tmp_file.string()));
        else if(fd < 0) {
            throw std::system_error(errno, std::generic_category());
        }
        else
            ::close(fd);
        // If we get here, it is guaranteed that this process created the
        // tmp_file.
        // Abuse unique_ptr as a scope guard to remove tmp_file in any execution
        // path except when it's explicitly released.
        auto tf_sg = std::unique_ptr<int, std::function<void(int*)>> (&fd,
                [&tmp_file](int*){ std::filesystem::remove(tmp_file); });

        // Check opts->create vs db_file existence
        bool db_file_exists = std::filesystem::exists(db_file);
        if(opts->create && db_file_exists) {
            throw std::runtime_error(std::format("File exists: {}",
                        db_file.string()));
        } else if(!opts->create && !db_file_exists) {
            throw std::runtime_error(std::format("File does not exist: {}",
                        db_file.string()));
        }
        std::cerr << std::format("{} {}\n", opts->create ? "Creating" : "Using",
                db_file.string());

        // Create and initialize the database
        pwdb::db cdb{};
        if(opts->import_file.empty()) {
            // Read in the database
            if(std::filesystem::exists(db_file)) {
                if(std::ifstream ifs(db_file); ifs.good()) {
                    gpgh::context ctx{opts->gpg_homedir};
                    cdb = pwdb::decode_data<pwdb::pb::DB>(ctx, ifs);
                    check_gpg_verify_result(ctx);
                } else {
                    throw std::system_error(errno, std::generic_category());
                }
            }
        } else {
            if(std::ifstream ifs(opts->import_file); ifs.good()) {
                std::cerr << std::format("Importing {}\n", opts->import_file);
                gpgh::context ctx{opts->gpg_homedir};
                cdb = pwdb::json2pb<pwdb::pb::DB>(ctx.decrypt(ifs));
                check_gpg_verify_result(ctx);
                do_recrypt = true;
            } else {
                throw std::system_error(errno, std::generic_category());
            }
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
            std::cerr << std::format("Exporting {}\n", opts->export_file);
            if(std::ofstream ofs(opts->export_file); ofs.good()) {
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
            pwdb::overwrite_file(db_file, tmp_file, encode);
            tf_sg.release();
        }

    } catch(const std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    std::cout << "Exiting" << std::endl;
    return 0;
}
