/* SPDX-License-Identifier: LGPL-3.0-or-later */
#ifndef gpgh_gpg_helper_h_included
#define gpgh_gpg_helper_h_included

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

extern "C" {
#include <gpgme.h>
} // extern "C"
#include <stdexcept>
#include <string>
#include <deque>
#include <functional>
#include <memory>
#include <iostream>

namespace gpgh {

//=============================================================================
// error type to throw on GpgME errors
//=============================================================================
class error : public std::runtime_error
{
public:
    error(const std::string &w) : std::runtime_error(w) { ; }
};

//=============================================================================
// gpgme_key_t RAII wrapper
//=============================================================================
using _key_up_t = std::unique_ptr<std::remove_pointer<gpgme_key_t>::type,
      decltype(&gpgme_key_unref)>;
class key : public _key_up_t
{
public:
    key(void) : _key_up_t(nullptr, gpgme_key_unref) { ; }
    key(gpgme_key_t k) : _key_up_t(k, gpgme_key_unref) { ; }
};
using keylist = std::deque<std::shared_ptr<key>>;

//=============================================================================
// A std::streambuf backed gpgme_data_t
//=============================================================================
using data_up_t = std::unique_ptr<std::remove_pointer<gpgme_data_t>::type,
    decltype(&gpgme_data_release)>;

class data_stream
{
    data_up_t data_{nullptr, gpgme_data_release};
    std::streambuf *sb_{nullptr};
public:
    data_stream(void);
    data_stream(std::streambuf *sb) : data_stream{} { sb_ = sb; }
    auto get(void) const noexcept->gpgme_data_t { return data_.get(); }
    auto rdbuf(void) const noexcept->std::streambuf* { return sb_; }
    auto rdbuf(std::streambuf *sb) noexcept->std::streambuf*
        { return sb_ = sb; }
    virtual ~data_stream() = default;
};

//=============================================================================
// std::streambuf backed gpgme input and output data objects
//=============================================================================

class idata : public data_stream
{
public:
    idata(void) = default;
    idata(std::ostream &out) : data_stream{out.rdbuf()} { ; }
    auto to_string(void) const->std::string;
};
inline std::ostream &operator<<(std::ostream &out, const idata &d)
    { return out << d.rdbuf(); }

class odata : public data_stream
{
public:
    odata(void) = default;
    odata(std::istream &in) : data_stream{in.rdbuf()} { ; }
};
inline std::istream &operator>>(std::istream &in, const odata &d)
    { return in >> d.rdbuf(); }

//=============================================================================
// Utility functions
//=============================================================================
// check and throw on gpgme_error_t
void gerr_check(gpgme_error_t gerr, const char *pstr = nullptr);

// print basic key info - for full info use operator <<(ostream, Key)
void print_key(std::ostream &out, gpgh::key &k,
        const std::string prefix = "");

// create non-owning null terminated vector of gpgme_key_t from keylist
auto keylist2kvec(const keylist &kl)->std::vector<gpgme_key_t>;

// generate passwordless testing key
class context;
auto gen_test_key(gpgh::context &context, const std::string &recipient) ->
    gpgh::keylist;

//=============================================================================
// gpgme_context wrapper
//=============================================================================
class context
{
    std::unique_ptr<gpgme_context, decltype(&gpgme_release)> _ctx;

    static const char *gpg_version;
    static void gpg_init(void);
    static bool filt_true(gpgme_key_t) noexcept { return true; }

public:
    context(void);
    context(const std::string &gpg_homedir);
    auto get(void)->gpgme_ctx_t { return _ctx.get(); }
    auto get_keys(const std::string &recipient, bool secret_only = false,
            std::function<bool(gpgme_key_t)> filter = filt_true)
        -> gpgh::keylist;
    auto get_keys(const std::vector<std::string> &recipients,
            bool secret_only = false,
            std::function<bool(gpgme_key_t)> filter = filt_true)
        -> gpgh::keylist;
    void clear_signers(void) { gpgme_signers_clear(_ctx.get()); }
    void add_signer(const std::string &uid);

    // encrypt
    auto encrypt(const gpgh::keylist &recipients, const std::string &src,
            bool sign = false,
            gpgme_encrypt_flags_t flags = (gpgme_encrypt_flags_t)0)
        -> std::string;
    auto encrypt(const gpgh::keylist &recipients, std::istream &src,
            bool sign = false,
            gpgme_encrypt_flags_t flags = (gpgme_encrypt_flags_t)0)
        -> std::string;
    void encrypt(const gpgh::keylist &recipients,
            const std::string &src, std::ostream &dest,
            bool sign = false,
            gpgme_encrypt_flags_t flags = (gpgme_encrypt_flags_t)0);
    void encrypt(const gpgh::keylist &recipients,
            std::istream &src, std::ostream &dest,
            bool sign = false,
            gpgme_encrypt_flags_t flags = (gpgme_encrypt_flags_t)0);

    // decrypt
    auto decrypt(const std::string &src, gpgme_decrypt_flags_t flags =
            GPGME_DECRYPT_VERIFY) -> std::string;
    auto decrypt(std::istream &src, gpgme_decrypt_flags_t flags =
            GPGME_DECRYPT_VERIFY) -> std::string;
    void decrypt(const std::string &src, std::ostream &dest,
            gpgme_decrypt_flags_t flags = GPGME_DECRYPT_VERIFY);
    void decrypt(std::istream &src, std::ostream &dest,
            gpgme_decrypt_flags_t flags = GPGME_DECRYPT_VERIFY);
};

} // namespace gpgh
#endif // gpgh_gpg_helper_h_included
