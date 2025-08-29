#include "Body.h"
#include "../../FA2sp/Logger.h"
#include <iostream>
#include <cstring>
#include <unordered_set>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <CFinalSunApp.h>
#include <XCC/mix_file.h>
#include <openssl/bn.h>
#include <openssl/blowfish.h>
#include "../../FA2sp.h"

namespace MinInfo
{
    struct MixFileStruct {
        std::vector<MixEntry> files;
        uint32_t flags{};
        uint32_t size{};
        uint32_t offset{};
    };

    static inline uint32_t align_up(uint32_t x, uint32_t a) {
        return (x + (a - 1)) & ~(a - 1);
    }

    static bool read_exact(std::istream& in, void* dst, size_t len) {
        in.read(reinterpret_cast<char*>(dst), static_cast<std::streamsize>(len));
        return static_cast<size_t>(in.gcount()) == len;
    }

    static uint16_t read_u16(std::istream& in) {
        uint8_t b[2];
        if (!read_exact(in, b, 2)) throw std::runtime_error("read_u16 failed");
        return (uint16_t)(b[0] | (uint16_t(b[1]) << 8));
    }

    static uint32_t read_u32(std::istream& in) {
        uint8_t b[4];
        if (!read_exact(in, b, 4)) throw std::runtime_error("read_u32 failed");
        return (uint32_t)(b[0] | (uint32_t(b[1]) << 8) | (uint32_t(b[2]) << 16) | (uint32_t(b[3]) << 24));
    }

    static std::vector<MixEntry> read_entries(std::istream& in, uint16_t count) {
        std::vector<MixEntry> v;
        v.reserve(count);
        for (uint16_t i = 0; i < count; ++i) {
            MixEntry e;
            e.id = read_u32(in);
            e.offset = read_u32(in);
            e.size = read_u32(in);
            v.push_back(e);
        }
        return v;
    }

    static void byteswap(std::vector<uint8_t>& buf) {
        std::reverse(buf.begin(), buf.end());
    }

    static void byteswap(uint8_t* p, size_t n) {
        for (size_t i = 0, j = n ? n - 1 : 0; i < j; ++i, --j) {
            std::swap(p[i], p[j]);
        }
    }

    static std::vector<uint8_t> rsa_transform_raw(const uint8_t* block, size_t block_len,
        const uint8_t* mod, size_t mod_len,
        uint32_t e) {
        std::vector<uint8_t> out;

        BIGNUM* x = BN_bin2bn(block, static_cast<int>(block_len), nullptr);
        BIGNUM* n = BN_bin2bn(mod, static_cast<int>(mod_len), nullptr);
        BIGNUM* y = BN_new();
        BIGNUM* be = BN_new();

        BN_CTX* ctx = BN_CTX_new();
        if (!x || !n || !y || !be || !ctx) goto cleanup;

        if (!BN_set_word(be, e)) goto cleanup;
        if (!BN_mod_exp(y, x, be, n, ctx)) goto cleanup;

        {
            int nbytes = BN_num_bytes(y);
            out.resize(nbytes);
            BN_bn2bin(y, out.data());
        }

    cleanup:
        if (ctx) BN_CTX_free(ctx);
        if (x) BN_free(x);
        if (n) BN_free(n);
        if (y) BN_free(y);
        if (be) BN_free(be);
        return out;
    }

    class BlowfishECBReader {
    public:
        BlowfishECBReader(std::istream& in, const uint8_t* key, size_t key_len)
            : in_(in) {
            BF_set_key(&key_, static_cast<int>(key_len), key);
        }

        void read(uint8_t* dst, size_t len) {
            ensure(len);
            size_t n = std::min(len, buffer_.size() - rdpos_);
            std::memcpy(dst, buffer_.data() + rdpos_, n);
            rdpos_ += n;
            if (n != len) throw std::runtime_error("ECB short read");
            if (rdpos_ > 4096) {
                buffer_.erase(buffer_.begin(), buffer_.begin() + rdpos_);
                rdpos_ = 0;
            }
        }

    private:
        void ensure(size_t need) {
            size_t avail = buffer_.size() - rdpos_;
            if (avail >= need) return;

            const size_t blksz = 8;
            size_t missing = need - avail;
            size_t nbytes = ((missing + blksz - 1) / blksz) * blksz;

            std::vector<uint8_t> enc(nbytes, 0);
            if (!read_exact(in_, enc.data(), nbytes)) {
                throw std::runtime_error("ECB underlying stream EOF");
            }

            for (size_t off = 0; off < nbytes; off += blksz) {
                BF_ecb_encrypt(enc.data() + off, enc.data() + off, &key_, BF_DECRYPT);
            }

            if (rdpos_ == 0) {
                buffer_.insert(buffer_.end(), enc.begin(), enc.end());
            }
            else {
                buffer_.erase(buffer_.begin(), buffer_.begin() + rdpos_);
                rdpos_ = 0;
                buffer_.insert(buffer_.end(), enc.begin(), enc.end());
            }
        }

        std::istream& in_;
        BF_KEY key_;
        std::vector<uint8_t> buffer_;
        size_t rdpos_ = 0;
    };

    static uint16_t read_u16_ecb(BlowfishECBReader& r) {
        uint8_t b[2];
        r.read(b, 2);
        return (uint16_t)(b[0] | (uint16_t(b[1]) << 8));
    }

    static uint32_t read_u32_ecb(BlowfishECBReader& r) {
        uint8_t b[4];
        r.read(b, 4);
        return (uint32_t)(b[0] | (uint32_t(b[1]) << 8) | (uint32_t(b[2]) << 16) | (uint32_t(b[3]) << 24));
    }

    static std::vector<MixEntry> read_entries_ecb(BlowfishECBReader& r, uint16_t count) {
        std::vector<MixEntry> v;
        v.reserve(count);
        for (uint16_t i = 0; i < count; ++i) {
            MixEntry e;
            e.id = read_u32_ecb(r);
            e.offset = read_u32_ecb(r);
            e.size = read_u32_ecb(r);
            v.push_back(e);
        }
        return v;
    }

    static const uint8_t RSA_MODULUS[] = {
        0x51, 0xbc, 0xda, 0x08, 0x6d, 0x39, 0xfc, 0xe4,
        0x56, 0x51, 0x60, 0xd6, 0x51, 0x71, 0x3f, 0xa2,
        0xe8, 0xaa, 0x54, 0xfa, 0x66, 0x82, 0xb0, 0x4a,
        0xab, 0xdd, 0x0e, 0x6a, 0xf8, 0xb0, 0xc1, 0xe6,
        0xd1, 0xfb, 0x4f, 0x3d, 0xaa, 0x43, 0x7f, 0x15,
    };
    static const uint32_t RSA_E = 0x10001;

    static std::vector<uint8_t> blowfish_key_from_keysource(const uint8_t keySource[80]) {
        uint8_t ks[80];
        std::memcpy(ks, keySource, 80);
        byteswap(ks, 80);

        const uint8_t* left = ks;
        const uint8_t* right = ks + 40;

        std::vector<uint8_t> s0 = rsa_transform_raw(left, 40, RSA_MODULUS, sizeof(RSA_MODULUS), RSA_E);
        std::vector<uint8_t> s1 = rsa_transform_raw(right, 40, RSA_MODULUS, sizeof(RSA_MODULUS), RSA_E);

        BIGNUM* a = BN_bin2bn(s0.data(), (int)s0.size(), nullptr);
        BIGNUM* b = BN_bin2bn(s1.data(), (int)s1.size(), nullptr);
        BIGNUM* c = BN_new();
        BIGNUM* d = BN_new();
        BN_CTX* ctx = BN_CTX_new();

        if (!a || !b || !c || !d || !ctx) {
            if (a) BN_free(a); if (b) BN_free(b); if (c) BN_free(c); if (d) BN_free(d);
            if (ctx) BN_CTX_free(ctx);
            return {};
        }
        if (!BN_lshift(c, a, 312)) {
            BN_free(a); BN_free(b); BN_free(c); BN_free(d); BN_CTX_free(ctx);
            return {};
        }
        if (!BN_add(d, b, c)) {
            BN_free(a); BN_free(b); BN_free(c); BN_free(d); BN_CTX_free(ctx);
            return {};
        }

        int n = BN_num_bytes(d);
        std::vector<uint8_t> key(n);
        BN_bn2bin(d, key.data());
        byteswap(key);

        BN_free(a); BN_free(b); BN_free(c); BN_free(d); BN_CTX_free(ctx);

        if (key.size() > 56) key.resize(56);
        if (key.size() < 4) key.resize(4, 0);

        return key;
    }

    static std::unique_ptr<MixFileStruct> unpack_mix(std::istream& in, uint32_t total_size) {
        uint16_t count_or_zero = read_u16(in);

        if (count_or_zero != 0) {
            uint16_t count = count_or_zero;
            (void)read_u32(in);
            auto entries = read_entries(in, count);

            auto mf = std::make_unique<MixFileStruct>();
            mf->files = std::move(entries);
            mf->flags = 0;
            mf->offset = 6u + 12u * static_cast<uint32_t>(count);
            mf->size = static_cast<uint32_t>(total_size) - mf->offset;
            return mf;
        }

        uint16_t flags16 = read_u16(in);
        uint32_t flags = uint32_t(flags16) << 16;

        constexpr uint32_t flagEncrypted = 0x00020000u;

        if (flags & flagEncrypted) {
            uint8_t keySource[80];
            if (!read_exact(in, keySource, 80)) throw std::runtime_error("read keySource failed");

            std::vector<uint8_t> bf_key = blowfish_key_from_keysource(keySource);
            if (bf_key.empty()) throw std::runtime_error("derive blowfish key failed");

            BlowfishECBReader ecb(in, bf_key.data(), bf_key.size());

            uint16_t count = read_u16_ecb(ecb);
            (void)read_u32_ecb(ecb);
            auto entries = read_entries_ecb(ecb, count);

            auto mf = std::make_unique<MixFileStruct>();
            mf->files = std::move(entries);
            mf->flags = flags;

            uint32_t index_len = 6u + 12u * static_cast<uint32_t>(count);
            mf->offset = 84u + align_up(index_len, 8u);
            mf->size = static_cast<uint32_t>(total_size) - mf->offset;
            return mf;
        }
        else {
            uint16_t count = read_u16(in);
            (void)read_u32(in);
            auto entries = read_entries(in, count);

            auto mf = std::make_unique<MixFileStruct>();
            mf->files = std::move(entries);
            mf->flags = flags;
            mf->offset = 10u + 12u * static_cast<uint32_t>(count);
            mf->size = static_cast<uint32_t>(total_size) - mf->offset;
            return mf;
        }
    }

    static uint32_t file_size(std::ifstream& f) {
        auto pos = f.tellg();
        f.seekg(0, std::ios::end);
        std::streamoff end = f.tellg();
        f.seekg(pos, std::ios::beg);
        return static_cast<uint32_t>(end);
    }

    static std::vector<MixEntry> GetMixInfo(const std::string& cPath) {
        VEHGuard guard(false);
        try {
            std::ifstream f(cPath, std::ios::binary);
            if (!f)  return {};

            uint32_t total = file_size(f);
            f.seekg(0, std::ios::beg);

            auto mix = unpack_mix(f, total);
            for (auto& e : mix->files) {
                e.offset += mix->offset;
            }
            return std::move(mix->files);
        }
        catch (...) {
            return {};
        }
    }

    static std::vector<MixEntry> GetMixInfoFromRange(const std::string& cPath, uint32_t offset, uint32_t size) {
        VEHGuard guard(false);
        try {
            if (offset < 0 || size <= 0)  return {};

            std::ifstream f(cPath, std::ios::binary);
            if (!f)  return {};

            f.seekg(0, std::ios::end);
            uint32_t file_end = static_cast<uint32_t>(f.tellg());
            uint32_t start = static_cast<uint32_t>(offset);
            uint32_t length = static_cast<uint32_t>(size);
            if (start > file_end || start + length > file_end)  return {};

            std::vector<char> buf(length);
            f.seekg(static_cast<std::streamoff>(start), std::ios::beg);
            if (!read_exact(f, buf.data(), buf.size()))  return {};

            std::istringstream sub(std::string(buf.data(), buf.size()));
            auto mix = unpack_mix(sub, length);
            for (auto& e : mix->files) {
                e.offset += start + mix->offset;
            }
            return std::move(mix->files);
        }
        catch (...) {
            return {};
        }
    }
} 

inline int _stricmp(const char* a, const char* b) {
    std::string sa(a), sb(b);
    std::transform(sa.begin(), sa.end(), sa.begin(), ::tolower);
    std::transform(sb.begin(), sb.end(), sb.begin(), ::tolower);
    return sa.compare(sb);
}

uint32_t MixLoader::GetFileID(const char* lpFileName)
{
    xcc_string name(lpFileName);
    return (uint32_t)Cmix_file::get_id(t_game::game_ra2, name);
}

MixLoader& MixLoader::Instance() {
    static MixLoader inst;
    return inst;
}

bool MixLoader::LoadTopMix(const std::string& path) {
    std::ifstream fin(path, std::ios::binary);
    if (!fin.is_open()) return false;
    auto entries = MinInfo::GetMixInfo(path);

    MixFile mf;
    mf.path = path;
    mf.stream = std::move(fin);
    mf.entries = std::move(entries);
    mf.isNested = false;
    mf.baseOffset = 0;

    for (const auto& e : mf.entries) {
        if (fileMap.find(e.id) == fileMap.end())
            fileMap[e.id] = { mixFiles.size(), &e};
    }

    mixFiles.push_back(std::move(mf));
    return true;
}

bool MixLoader::LoadNestedMix(MixFile& parent, const MixEntry& entry) {
    MixFile mf;
    mf.path = parent.path;
    mf.stream.open(parent.path, std::ios::binary);
    if (!mf.stream.is_open()) return false;

    auto entries = MinInfo::GetMixInfoFromRange(parent.path, entry.offset, entry.size);
    mf.entries = std::move(entries);
    mf.isNested = true;
    mf.baseOffset = entry.offset;

    for (const auto& e : mf.entries) {
        if (fileMap.find(e.id) == fileMap.end())
            fileMap[e.id] = { mixFiles.size(), &e };
    }

    mixFiles.push_back(std::move(mf));
    return true;
}

bool MixLoader::LoadMixFile(const std::string& path, int* parentIndex) {
    if (std::filesystem::exists(path)) {
        if (parentIndex) *parentIndex = -1;
        return LoadTopMix(path);
    }

    auto name = std::filesystem::path(path).filename().string();
    int id = GetFileID(name.c_str());
    for (size_t i = 0; i < mixFiles.size(); ++i) {
        auto& mf = mixFiles[i];
        for (const auto& e : mf.entries) {
            if (e.id == id) {
                if (parentIndex) *parentIndex = i;
                return LoadNestedMix(mf, e);
            }
        }
    }

    return false;
}

int MixLoader::QueryFileIndex(const std::string& fileName, int mixIdx) {
    if (mixFiles.empty()) return -1;
    int id = GetFileID(fileName.c_str());
    if (mixIdx >= 0 && mixIdx < (int)mixFiles.size()) {
        auto& mix = mixFiles[mixIdx];
        for (const auto& e: mix.entries) {
            if (e.id == id)
                return mixIdx;
        }
        return -1;
    }

    auto itr = fileMap.find(id);
    if (itr != fileMap.end())
        return static_cast<int>(itr->second.mixIndex);

    return -1;
}

std::unique_ptr<uint8_t[]> MixLoader::LoadFile(const std::string& fileName, size_t* outSize, int mixIdx) {
    if (mixFiles.empty()) return nullptr;
    if (outSize) *outSize = 0;
    int id = GetFileID(fileName.c_str());
    if (mixIdx >= 0 && mixIdx < (int)mixFiles.size()) {
        auto& mf = mixFiles[mixIdx];
        for (auto& e : mf.entries) {
            if (e.id == id) {
                if (!mf.stream.is_open()) {
                    mf.stream.open(mf.path, std::ios::binary);
                    if (!mf.stream.is_open()) return nullptr;
                }

                mf.stream.seekg(e.offset, std::ios::beg);
                if (!mf.stream.good()) return nullptr;

                auto buf = std::make_unique<uint8_t[]>((size_t)e.size);
                mf.stream.read(reinterpret_cast<char*>(buf.get()), e.size);
                std::streamsize got = mf.stream.gcount();
                if (got != (std::streamsize)e.size) {
                    return nullptr;
                }
                if (outSize) *outSize = (size_t)e.size;
                return buf;
            }
        }
        return nullptr;
    }

    auto itr = fileMap.find(id);
    if (itr != fileMap.end())
    {
        auto& mf = mixFiles[itr->second.mixIndex];
        const auto& e = itr->second.entry;
        if (!mf.stream.is_open()) {
            mf.stream.open(mf.path, std::ios::binary);
            if (!mf.stream.is_open()) return nullptr;
        }

        mf.stream.seekg(e->offset, std::ios::beg);
        if (!mf.stream.good()) return nullptr;

        auto buf = std::make_unique<uint8_t[]>((size_t)e->size);
        mf.stream.read(reinterpret_cast<char*>(buf.get()), e->size);
        std::streamsize got = mf.stream.gcount();
        if (got != (std::streamsize)e->size) {
            return nullptr;
        }
        if (outSize) *outSize = (size_t)e->size;
        return buf;
    }

    return nullptr;
}

bool MixLoader::ExtractFile(const std::string& fileName, const std::string& outPath, int mixIdx)
{
    size_t size = 0;
    if (auto file = LoadFile(fileName, &size, mixIdx))
    {
        std::ofstream fout(outPath, std::ios::binary);
        if (!fout) {
            return false;
        }

        fout.write((const char*)file.get(), size);
        return fout.good();
    }
    return false;
}

void MixLoader::Clear() {
    mixFiles.clear();
    fileMap.clear();
}