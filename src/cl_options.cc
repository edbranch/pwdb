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
#include <boost/program_options.hpp>
#include <string>
#include <map>
#include <filesystem>
#include <sstream>
#include <iostream>
#include <format>
#include <memory>

namespace pwdb {

namespace fs = ::std::filesystem;
namespace po = ::boost::program_options;

using namespace std::literals::string_literals;

struct cmd_entry {
    po::options_description vis_opts;
    po::options_description all_opts;
    po::positional_options_description args;

    cmd_entry() = default;
    cmd_entry(const std::string &vis_name) : vis_opts(vis_name) {;}
    cmd_entry(const std::string &vis_name,
            const po::options_description &all_base) :
        vis_opts(vis_name),
        all_opts(all_base)
    {;}
    cmd_entry(const std::string &vis_name,
            const po::positional_options_description &args_base) :
        vis_opts(vis_name),
        args(args_base)
    {;}
    cmd_entry(const std::string &vis_name,
            const po::options_description &all_base,
            const po::positional_options_description &args_base) :
        vis_opts(vis_name),
        all_opts(all_base),
        args(args_base)
    {;}
};

static cl_options
make_cl_options(const std::string &usage, const std::string &subcmd,
        const po::variables_map &opts)
{
    auto opt_as_string_or_empty = [&opts](const char *name)->std::string {
        return opts.count(name) ? opts[name].as<std::string>() : ""s;
    };
    return cl_options{
        .help = !!opts.count("help"),
        .version = !!opts.count("version"),
        .usage_msg = usage,
        .subcmd = subcmd,
        .pwdb_file = opt_as_string_or_empty("file"),
        .uid = opt_as_string_or_empty("uid"),
        .gpg_homedir = opt_as_string_or_empty("gpg-homedir"),
        .infile = opt_as_string_or_empty("infile"),
        .outfile = opt_as_string_or_empty("outfile"),
    };
}

cl_options
cl_handle(int argc, const char *argv[])
{
    // Info options
    // First, the options that show in help
    po::options_description info_opts_vis("Info Options");
    info_opts_vis.add_options()
        ("help,h", "print help and exit")
        ("version,v", "print version and exit")
    ;
    // Base Info options for Sub-Commands to add must include the Sub-Command
    po::options_description info_opts_base("Info Options");
    info_opts_base.add(info_opts_vis).add_options()
        ("subcmd", po::value<std::string>())
    ;
    po::positional_options_description info_args_base;
    info_args_base.add("subcmd", 1);
    // First-stage parser needs to handle unknown Sub-Command positional args
    po::options_description info_opts_desc("Info Options");
    info_opts_desc.add(info_opts_base).add_options()
        ("extras", po::value<std::vector<std::string>>())
    ;
    auto info_args_desc = info_args_base;
    info_args_desc.add("extras", -1);

    // Common options
    po::options_description common_opts_desc("Common Options");
    common_opts_desc.add_options()
        ("file,f", po::value<std::string>()->
                default_value(pwdb::xdg_data_dir() + "/pwdb/pwdb.gpg"),
            "Password database file")
        ("uid,u", po::value<std::string>(),
            "GnuPG UID of signer and primary encryption recipient")
        ("gpg-homedir", po::value<std::string>(), "GnuPG home directory")
    ;

    // commands map
    std::map<std::string, cmd_entry> cmds_map;

    { // open
        [[maybe_unused]] auto &entry = cmds_map.try_emplace("open",
                "open Options", common_opts_desc).first->second;
    }

    { // recrypt
        [[maybe_unused]] auto &entry = cmds_map.try_emplace("recrypt",
                "recrypt Options", common_opts_desc).first->second;
    }

    { // import
        auto &entry = cmds_map.try_emplace("import", "import Options",
                common_opts_desc).first->second;
        entry.vis_opts.add_options()
            ("infile", po::value<std::string>()->required(),
                "Input file for import")
        ;
        entry.all_opts.add(entry.vis_opts);
        entry.args.add("infile", 1);
    }

    { // export
        auto &entry = cmds_map.try_emplace("export", "export Options",
                common_opts_desc).first->second;
        entry.vis_opts.add_options()
            ("outfile", po::value<std::string>()->required(),
                "Output file for export")
        ;
        entry.all_opts.add(entry.vis_opts);
        entry.args.add("outfile", 1);
    }

    // Usage message
    auto progname = fs::path{argv[0]}.filename().string();
    auto usage = [&](void)->std::string {
        std::stringstream ss;
        ss << "Usage:\n";
        ss << std::format("  {} -h | -v\n", progname);
        ss << std::format("  {} [open] [Common Options] [open Options]\n",
                progname);
        ss << std::format("  {} recrypt [Common Options]\n", progname);
        ss << std::format("  {} import [Common Options] {{infile}}\n",
                progname);
        ss << std::format("  {} export [Common Options] {{outfile}}\n",
                progname);
        // We want a specific order, cmds_map.keys() would be alphabetical
        const char *subcmds[] = {"open", "recrypt", "import", "export"};
        ss << info_opts_vis << common_opts_desc;
        for(const auto &i: subcmds)
            ss << cmds_map.at(i).vis_opts;
        return ss.str();
    };

    std::string subcmd{"open"};
    po::variables_map opts_map;
    try {
        // First pass, get Info Options (ie. -h, -v) and the subcmd if given
        auto gen_opts_parsed = po::command_line_parser(argc, argv)
            .options(info_opts_desc)
            .positional(info_args_desc)
            .allow_unregistered()
            .run();
        po::store(gen_opts_parsed, opts_map);
        auto deferred_opts = po::collect_unrecognized(gen_opts_parsed.options,
                po::include_positional);
        // Determine the Sub-Command. It may be missing (ok, infer "open") but
        // if missing, an argument to an unknown option could have been
        // misinterpreted (unavoidably) as the Sub-Command, so detect and fix.
        if(opts_map.count("subcmd")) {
            const auto maybe_subcmd = opts_map["subcmd"].as<std::string>();
            if(!deferred_opts.empty() && maybe_subcmd == deferred_opts.front())
            {
                subcmd = maybe_subcmd;
                deferred_opts.erase(deferred_opts.begin());
            }
        }

        // Second pass with full options description
        auto subcmd_iter = cmds_map.find(subcmd);
        if(subcmd_iter == cmds_map.end()) {
            throw std::runtime_error(std::format(
                        "Invalid Sub-Command: {}\n{}", subcmd, usage()));
        }
        auto parsed_opts = po::command_line_parser(deferred_opts)
            .options(subcmd_iter->second.all_opts)
            .positional(subcmd_iter->second.args)
            .run();
        po::store(parsed_opts, opts_map);
        po::notify(opts_map);
    }
    catch(const po::error &e) {
        throw std::runtime_error(std::format("{}\n{}", e.what(), usage()));
    }

    return make_cl_options(usage(), subcmd, opts_map);
}

} // namespace pwdb
