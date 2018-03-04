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
#include "pwdb/util.h"
#include "pwdb/config.h"
#include <boost/program_options.hpp>
#include <experimental/filesystem>
#include <sstream>
#include <iostream>

namespace pwdb {

namespace fs = ::std::experimental::filesystem;
namespace po = ::boost::program_options;

static std::optional<po::variables_map>
clopts_parse(int argc, const char *argv[])
{
    po::options_description opts_desc("Options");
    opts_desc.add_options()
        ("help,h", "print help and exit")
        ("version,v", "print version and exit")
        ("file,f", po::value<std::string>()->
                default_value(pwdb::xdg_data_dir() + "/pwdb/pwdb.gpg"),
            "Data file to load and save to")
        ("create,c", "Create new data file")
        ("uid,u", po::value<std::string>(),
            "GnuPG UID of signer and primary encryption recipient")
        // ("record", po::value<std::string>(), "Record to open") // TODO
        ("gpg-homedir", po::value<std::string>(), "GnuPG home directory")
        ("recrypt", "Re-encrypt all record data stores")
    ;
    auto usage = [&opts_desc](void)->std::string {
        std::stringstream ss;
        ss << "Usage:\n" << opts_desc;
        return ss.str();
    };
    po::variables_map opts;
    std::optional<po::variables_map> ret;
    try {
        po::store(po::parse_command_line(argc, argv, opts_desc), opts);
        if(opts.count("help")) {
            std::cout << usage() << std::endl;
            return ret;
        }
        if(opts.count("version")) {
            std::cout << "pwdb version " << pwdb::version << std::endl;
            return ret;
        }
        po::notify(opts);    
    }
    catch(po::error &e) {
        std::stringstream ss;
        ss << e.what() << '\n' << usage();
        throw std::runtime_error(ss.str());
    }
    ret = std::move(opts);
    return std::move(ret);
}

std::optional<cl_options>
cl_handle(int argc, const char *argv[])
{
    std::optional<cl_options> ret;
    const auto optopts = clopts_parse(argc, argv);
    if(!optopts)
        return ret;
    ret = cl_options{};
    const auto &opts = *optopts;

    // Data file location and directory creation
    fs::path data_file(opts["file"].as<std::string>());
    ret->create = false;
    if(fs::exists(data_file)) {
        data_file = fs::canonical(data_file);
        if(!fs::is_regular_file(data_file)) {
            std::stringstream ss;
            ss << data_file << " is not a file";
            throw std::runtime_error(ss.str());
        }
    } else if(opts.count("create")) {
        /* FIXME: GCC 7 does not have weakly_canonical()
        data_file = fs::weakly_canonical(data_file);
        */
        ret->create = true;
        auto dir = data_file.parent_path();
        if(!dir.empty()) {
            fs::create_directories(dir);
        }
    } else {
        std::stringstream ss;
        ss << data_file << " does not exist";
        throw std::runtime_error(ss.str());
    }
    ret->file = data_file.string();

    // GnuPG sign/encrypt uid
    if(opts.count("uid"))
        ret->uid = opts["uid"].as<std::string>();

    // Record to open
    if(opts.count("record"))
        ret->record = opts["record"].as<std::string>();

    // GnuPG home directory
    if(opts.count("gpg-homedir"))
        ret->gpg_homedir = opts["gpg-homedir"].as<std::string>();

    // Recrypt
    if(opts.count("recrypt"))
        ret->recrypt = true;

    return std::move(ret);
}


} // namespace pwdb
