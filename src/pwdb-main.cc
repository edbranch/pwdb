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

#include "pwdb/config.h"
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

static void
read_from_pwdb(pwdb::db &cdb, const std::string &pwdb_file,
        const std::string &gpg_homedir)
{
    std::ifstream ifs(pwdb_file, std::ios::in | std::ios::binary);
    ifs.exceptions(std::ios::badbit | std::ios::failbit);
    gpgh::context ctx{gpg_homedir};
    cdb = pwdb::decode_data<pwdb::pb::DB>(ctx, ifs);
    check_gpg_verify_result(ctx);
}

static void
subcmd_open(const pwdb::cl_options &opts)
{
    pwdb::lock_overwrite_file db_file_lock{fs::path(opts.pwdb_file)};
    auto db_file = db_file_lock.file().string();
    bool db_file_exists = fs::exists(db_file);
    std::cerr << (db_file_exists ? "Opening " : "Creating ") << db_file <<
        std::endl;
    pwdb::db cdb{};
    if(db_file_exists) {
        read_from_pwdb(cdb, db_file, opts.gpg_homedir);
    }

    // Set signing and primary encryption uid
    bool cdb_modified = false;
    if(!opts.uid.empty() && opts.uid != cdb.uid()) {
        if(!cdb.uid().empty()) {
            std::cerr <<
                "WARNING: uid has changed, recommend running recrypt\n";
        }
        cdb.uid(opts.uid);
        cdb_modified = true;
    }
    {
        gpgh::context ctx{opts.gpg_homedir};
        check_uid(ctx, cdb.uid());
    }

    // Run command interpreter
    pwdb::pwdb_cmd_interp cmd_interp(cdb);
    cmd_interp.run("pwdb> ");
    cdb_modified = cdb_modified || cmd_interp.modified();

    // Save database
    if(cdb_modified) {
        std::cerr << "Database modified, saving" << std::endl;
        auto encode = [&cdb, &opts](std::ostream& out) {
            gpgh::context ctx{opts.gpg_homedir};
            ctx.add_signer(cdb.uid());
            pwdb::encode_data(ctx, cdb.uid(), cdb.pb(), out, true);
        };
        db_file_lock.overwrite(encode);
    }
    std::cerr << "Closed " << db_file << std::endl;
}

static void
subcmd_recrypt(const pwdb::cl_options &opts)
{
    // Lock, check, and read in the pwdb file
    pwdb::lock_overwrite_file db_file_lock{fs::path(opts.pwdb_file)};
    auto db_file = db_file_lock.file().string();
    if(!fs::exists(db_file)) {
        throw std::runtime_error("File does not exist: "s + db_file);
    }
    std::cerr << "Re-encrypting " << db_file << std::endl;
    pwdb::db cdb{};
    read_from_pwdb(cdb, db_file, opts.gpg_homedir);

    // Set signing and primary encryption uid then re-encrypt
    if(!opts.uid.empty()) {
        cdb.uid(opts.uid);
    }
    {
        gpgh::context ctx{opts.gpg_homedir};
        check_uid(ctx, cdb.uid());
        pwdb::db_recrypt_rcd_stores(ctx, cdb);
    }

    // Save database
    auto encode = [&cdb, &opts](std::ostream& out) {
        gpgh::context ctx{opts.gpg_homedir};
        ctx.add_signer(cdb.uid());
        pwdb::encode_data(ctx, cdb.uid(), cdb.pb(), out, true);
    };
    db_file_lock.overwrite(encode);
}

static void
subcmd_import(const pwdb::cl_options &opts)
{
    pwdb::lock_overwrite_file db_file_lock{fs::path(opts.pwdb_file)};
    auto db_file = db_file_lock.file().string();
    if(fs::exists(db_file)) {
        throw std::runtime_error("File exists: "s + db_file);
    }
    std::cerr << "Importing " << opts.infile << " to " << db_file << std::endl;
    pwdb::db cdb{};
    {
        std::ifstream ifs(opts.infile, std::ios::in | std::ios::binary);
        ifs.exceptions(std::ios::badbit | std::ios::failbit);
        gpgh::context ctx{opts.gpg_homedir};
        cdb = pwdb::json2pb<pwdb::pb::DB>(ctx.decrypt(ifs));
        check_gpg_verify_result(ctx);
    }

    // Set signing and primary encryption uid, then encrypt record stores
    if(!opts.uid.empty()) {
        cdb.uid(opts.uid);
    }
    {
        gpgh::context ctx{opts.gpg_homedir};
        check_uid(ctx, cdb.uid());
        pwdb::db_recrypt_rcd_stores(ctx, cdb);
    }

    // Save database
    auto encode = [&cdb, &opts](std::ostream& out) {
        gpgh::context ctx{opts.gpg_homedir};
        ctx.add_signer(cdb.uid());
        pwdb::encode_data(ctx, cdb.uid(), cdb.pb(), out, true);
    };
    db_file_lock.overwrite(encode);
}

static void
subcmd_export(const pwdb::cl_options &opts)
{
    // Because writers use read-write-replace readers shouldn't need to lock
    auto db_file = fs::weakly_canonical(opts.pwdb_file).string();
    if(!fs::exists(db_file)) {
        throw std::runtime_error("File does not exist: "s + db_file);
    }
    std::cerr << "Exporting " << db_file << " to " << opts.outfile << std::endl;
    pwdb::db cdb{};
    read_from_pwdb(cdb, db_file, opts.gpg_homedir);

    // Set signing and primary encryption uid
    if(!opts.uid.empty()) {
        cdb.uid(opts.uid);
    }
    {
        gpgh::context ctx{opts.gpg_homedir};
        check_uid(ctx, cdb.uid());
    }

    // Export
    std::ofstream ofs(opts.outfile,
            std::ios::out | std::ios::binary);
    ofs.exceptions(std::ios::badbit | std::ios::failbit);
    fs::permissions(opts.outfile,
            fs::perms::owner_read | fs::perms::owner_write);
    gpgh::context ctx{opts.gpg_homedir};
    ctx.add_signer(cdb.uid());
    db_decrypt_all_rcd_stores(ctx, cdb);
    auto json = pwdb::pb2json(cdb.get_db());
    auto keyfilt = [](gpgme_key_t k)->bool {
        return !k->revoked && !k->expired && k->can_encrypt &&
            k->can_sign;
    };
    auto keys = ctx.get_keys(cdb.uid(), false, keyfilt);
    ctx.encrypt(keys, json, ofs, true);
}

int main(int argc, const char *argv[])
{
    try {
        const auto opts = pwdb::cl_handle(argc, argv);
        if(opts.help)
            std::cout << opts.usage_msg << std::endl;
        else if(opts.version)
            std::cout << "pwdb version " << pwdb::version << std::endl;
        else if(opts.subcmd == "open")
            subcmd_open(opts);
        else if(opts.subcmd == "recrypt")
            subcmd_recrypt(opts);
        else if(opts.subcmd == "import")
            subcmd_import(opts);
        else if(opts.subcmd == "export")
            subcmd_export(opts);
        else
            throw std::logic_error("Invalid commandline options structure");
    } catch(const std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
