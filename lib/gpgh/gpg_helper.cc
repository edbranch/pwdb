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
#include <sstream>
#include <fstream>
#include <locale>
#include <system_error>

namespace gpgh {

using namespace std::literals;

void gerr_check(gpgme_error_t gerr, const char *pstr)
{
    if(gpgme_err_code(gerr) != GPG_ERR_NO_ERROR) {
        if(pstr)
            std::cerr << "GPGH ERROR: " << pstr << std::endl;
        throw gpgh::error(gpgme_strerror(gerr));
    }
}

bool gerr_show(gpgme_error_t gerr, const char *pstr)
{
    bool success = gpgme_err_code(gerr) == GPG_ERR_NO_ERROR;
    if(!success) {
        if(pstr)
            std::cerr << "GPGH ERROR: " << pstr << std::endl;
        std::cerr << gpgme_strerror(gerr) << std::endl;
    }
    return success;
}

// RAII / Exception safety for gpgme keylist operation
class gpgme_op_keylist
{
    gpgme_context *ctx_;
public:
    gpgme_op_keylist(void) = delete;
    gpgme_op_keylist(gpgme_context *ctx, const std::string &recip,
            bool secret_only) : ctx_{ctx}
    {
        auto gerr = gpgme_op_keylist_start(ctx, recip.c_str(), secret_only);
        gerr_check(gerr, __func__);
    }
    gpgme_op_keylist(const gpgme_op_keylist&) = delete;
    gpgme_op_keylist &operator=(const gpgme_op_keylist&) = delete;
    ~gpgme_op_keylist()
    {
        auto gerr = gpgme_op_keylist_end(ctx_);
        gerr_show(gerr, __func__);
    }
};

//=============================================================================
// data_stream
//=============================================================================

// NOTE: None of these data_stream_cbs_* functions can throw as threy are all
// called back from C

ssize_t data_stream_cbs_read(void *handle, void *buffer, size_t size)
{
    auto ds = reinterpret_cast<data_stream*>(handle);
    std::streambuf *sbuf = ds->rdbuf();
    if(nullptr == sbuf) {
        errno = ENOBUFS;
        return -1;
    }
    // TODO - error handling?
    return sbuf->sgetn(reinterpret_cast<char*>(buffer), size);
}

ssize_t data_stream_cbs_write(void *handle, const void *buffer, size_t size)
{
    auto ds = reinterpret_cast<data_stream*>(handle);
    std::streambuf *sbuf = ds->rdbuf();
    if(nullptr == sbuf) {
        errno = ENOBUFS;
        return -1;
    }
    // TODO - error handling?
    return sbuf->sputn(reinterpret_cast<const char*>(buffer), size);
}

off_t data_stream_cbs_seek(void *handle, off_t offset, int whence)
{
    auto ds = reinterpret_cast<data_stream*>(handle);
    std::streambuf *sbuf = ds->rdbuf();
    if(nullptr == sbuf) {
        errno = ENOBUFS;
        return -1;
    }
    std::ios_base::seekdir way;
    switch(whence) {
        case SEEK_SET:
            way = std::ios_base::beg;
            break;
        case SEEK_CUR:
            way = std::ios_base::cur;
            break;
        case SEEK_END:
            way = std::ios_base::end;
            break;
        default:
            errno = -EINVAL;
            return -1;
    }
    return sbuf->pubseekoff(offset, way);
}

class data_stream_cbs
{
    gpgme_data_cbs cbs = {nullptr};
public:
    data_stream_cbs() {
        cbs.read = data_stream_cbs_read;
        cbs.write = data_stream_cbs_write;
        cbs.seek = data_stream_cbs_seek;
    }
    auto get(void)->gpgme_data_cbs* { return &cbs; }
};

static gpgme_data_cbs *get_data_stream_cbs(void)
{
    static data_stream_cbs cbs{};
    return cbs.get();
}

data_stream::
data_stream(void)
{
    gpgme_data_t dt = nullptr;
    auto gerr = gpgme_data_new_from_cbs(&dt, get_data_stream_cbs(), this);
    gerr_check(gerr, __func__);
    data_.reset(dt);
}

//=============================================================================
// Utility functions
//=============================================================================

// print basic key info - for full info use operator <<(ostream, Key)
void
print_key(std::ostream &out, key &k, const std::string prefix)
{
    out << prefix << "fpr:\t" << k->fpr << '\n';
    out << prefix << "id:\t" << k->uids->uid << '\n';
    out << prefix << "cmt:\t" << k->uids->comment << std::endl;
}

// create non-owning null terminated vector of gpgme_key_t from keylist
std::vector<gpgme_key_t>
keylist2kvec(const keylist &kl)
{
    std::vector<gpgme_key_t> rkv;
    rkv.reserve(kl.size() + 1);
    for(const auto &sk: kl)
        rkv.push_back(sk->get());
    rkv.push_back(nullptr);
    return rkv;
}

//=============================================================================
// context
// GpgME::Context wrapper
//=============================================================================
const char *context::gpg_version = nullptr;

context::
context(void) : _ctx(nullptr, gpgme_release)
{
    context::gpg_init();
    // create context
    gpgme_ctx_t ctxp = nullptr;
    auto gerr = gpgme_new(&ctxp);
    gerr_check(gerr, __func__);
    _ctx = std::unique_ptr<gpgme_context, decltype(&gpgme_release)>
        (ctxp, gpgme_release);
    // set protocol
    gerr = gpgme_set_protocol(ctxp, GPGME_PROTOCOL_OpenPGP);
    gerr_check(gerr, __func__);
}

context::
context(const std::string &gpg_homedir) : context{}
{
    if(gpg_homedir.empty())
        return;
    auto gerr = gpgme_ctx_set_engine_info(_ctx.get(), GPGME_PROTOCOL_OpenPGP,
            nullptr, gpg_homedir.c_str());
    gpgh::gerr_check(gerr, __func__);
}

gpgh::keylist context::
get_keys(const std::string &recipient, bool secret_only,
        std::function<bool(gpgme_key_t)> filter)
{
    keylist keys;
    gpgh::gpgme_op_keylist op_keylist(_ctx.get(), recipient, secret_only);
    do {
        gpgme_key_t kt;
        auto gerr = gpgme_op_keylist_next(_ctx.get(), &kt);
        if(gerr) {
            if(gpg_err_code(gerr) == GPG_ERR_EOF)
                break;
            else
                gerr_check(gerr, __func__);
        }
        if(filter(kt))
            keys.push_back(std::make_shared<key>(kt));
    } while (true);
    return keys;
}

gpgh::keylist context::
get_keys(const std::vector<std::string> &recipients, bool secret_only,
        std::function<bool(gpgme_key_t)> filter)
{
    gpgh::keylist kl;
    for(const auto &r: recipients) {
        auto ikl = get_keys(r, secret_only, filter);
        kl.insert(kl.end(), ikl.cbegin(), ikl.cend());
    }
    return kl;
}

void context::
add_signer(const std::string &uid)
{
    auto kl = get_keys(uid, true, [](gpgme_key_t k) {
            return k->can_encrypt && k->can_sign;
    });
    if(kl.empty())
        throw std::runtime_error("gpg uid "s + uid + " not found"s);
    if(kl.size() > 1)
        throw std::runtime_error("gpg uid "s + uid + " is not unique"s);
    auto gerr = gpgme_signers_add(_ctx.get(), kl[0]->get());
    gerr_check(gerr, __func__);
}

std::string context::
encrypt(const gpgh::keylist &recipients, const std::string &src, bool sign,
        gpgme_encrypt_flags_t flags)
{
    std::istringstream src_strm{src};
    std::ostringstream dest_strm{};
    this->encrypt(recipients, src_strm, dest_strm, sign, flags);
    return dest_strm.str();
}

std::string context::
encrypt(const gpgh::keylist &recipients, std::istream &src, bool sign,
        gpgme_encrypt_flags_t flags)
{
    std::ostringstream dest_strm{};
    this->encrypt(recipients, src, dest_strm, sign, flags);
    return dest_strm.str();
}

void context::
encrypt(const gpgh::keylist &recipients, const std::string &src,
        std::ostream &dest, bool sign, gpgme_encrypt_flags_t flags)
{
    std::istringstream src_strm{src};
    this->encrypt(recipients, src_strm, dest, sign, flags);
}

void context::
encrypt(const gpgh::keylist &recipients, std::istream &src, std::ostream &dest,
        bool sign, gpgme_encrypt_flags_t flags)
{
    gpgh::odata src_strm{src};
    gpgh::idata dest_strm{dest};
    auto rkv = keylist2kvec(recipients);
    auto encrypt_fn = sign ? gpgme_op_encrypt_sign : gpgme_op_encrypt;
    auto gerr = encrypt_fn(_ctx.get(), rkv.data(), flags, src_strm.get(),
            dest_strm.get());
    gerr_check(gerr, __func__);
}

std::string context::
decrypt(const std::string &src, gpgme_decrypt_flags_t flags)
{
    std::istringstream src_strm{src};
    std::ostringstream dest_strm{};
    this->decrypt(src_strm, dest_strm, flags);
    return dest_strm.str();
}

std::string context::
decrypt(std::istream &src, gpgme_decrypt_flags_t flags)
{
    std::ostringstream dest_strm{};
    this->decrypt(src, dest_strm, flags);
    return dest_strm.str();
}

void context::
decrypt(const std::string &src, std::ostream &dest, gpgme_decrypt_flags_t flags)
{
    std::istringstream src_strm{src};
    this->decrypt(src_strm, dest, flags);
}

void context::
decrypt(std::istream &src, std::ostream &dest, gpgme_decrypt_flags_t flags)
{
    gpgh::odata src_data{src};
    gpgh::idata dest_data{dest};
    auto gerr = gpgme_op_decrypt_ext(_ctx.get(), flags, src_data.get(),
            dest_data.get());
    gerr_check(gerr, __func__);
}

void context::
gpg_init(void)
{
    // run once
    if(gpg_version)
        return;
    gpgme_set_global_flag("require-gnupg", "2.1.13");
    // initialize library (yes, check_version initializes the library)
    gpg_version = gpgme_check_version("1.8.0");
    if(!gpg_version)
        throw gpgh::error("GnuPG version 1.8.0 required");
    // check OpenPGP engine installation
    auto gerr = gpgme_engine_check_version(GPGME_PROTOCOL_OpenPGP);
    gerr_check(gerr, __func__);
    // set locale
    setlocale(LC_ALL, "");
    gerr = gpgme_set_locale(NULL, LC_ALL, setlocale(LC_ALL, NULL));
    gerr_check(gerr, __func__);
}

} // namespace gpgh
