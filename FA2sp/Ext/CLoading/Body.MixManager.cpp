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

namespace MinInfo
{
    struct MixFile {
        std::vector<MixEntry> files;
        uint32_t flags{};
        uint32_t size{};
        uint32_t offset{};
    };

    // ===== helpers =====
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

    // ===== byteswap (reverse entire buffer) =====
    static void byteswap(std::vector<uint8_t>& buf) {
        std::reverse(buf.begin(), buf.end());
    }
    static void byteswap(uint8_t* p, size_t n) {
        for (size_t i = 0, j = n ? n - 1 : 0; i < j; ++i, --j) {
            std::swap(p[i], p[j]);
        }
    }

    // ===== RSA public key (no padding), equivalent to Go's big.Int Exp(x, e, n) =====
    static std::vector<uint8_t> rsa_transform_raw(const uint8_t* block, size_t block_len,
        const uint8_t* mod, size_t mod_len,
        uint32_t e) {
        std::vector<uint8_t> out;

        BIGNUM* x = BN_bin2bn(block, static_cast<int>(block_len), nullptr);
        BIGNUM* n = BN_bin2bn(mod, static_cast<int>(mod_len), nullptr);
        BIGNUM* y = BN_new(); // result
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

    // ===== Blowfish ECB reader (like Go's ecbReader.Read) =====
    class BlowfishECBReader {
    public:
        BlowfishECBReader(std::istream& in, const uint8_t* key, size_t key_len)
            : in_(in) {
            BF_set_key(&key_, static_cast<int>(key_len), key);
        }

        // Read 'len' bytes into dst; will read & decrypt ceil(len/8)*8 from underlying stream
        void read(uint8_t* dst, size_t len) {
            ensure(len);
            // drain from buffer
            size_t n = std::min(len, buffer_.size() - rdpos_);
            std::memcpy(dst, buffer_.data() + rdpos_, n);
            rdpos_ += n;
            if (n != len) throw std::runtime_error("ECB short read");
            // compact if consumed a lot
            if (rdpos_ > 4096) {
                buffer_.erase(buffer_.begin(), buffer_.begin() + rdpos_);
                rdpos_ = 0;
            }
        }

    private:
        void ensure(size_t need) {
            size_t avail = buffer_.size() - rdpos_;
            if (avail >= need) return;

            const size_t blksz = 8; // Blowfish block
            size_t missing = need - avail;
            size_t nbytes = ((missing + blksz - 1) / blksz) * blksz;

            std::vector<uint8_t> enc(nbytes, 0);
            if (!read_exact(in_, enc.data(), nbytes)) {
                throw std::runtime_error("ECB underlying stream EOF");
            }

            // decrypt block-by-block in place
            for (size_t off = 0; off < nbytes; off += blksz) {
                BF_ecb_encrypt(enc.data() + off, enc.data() + off, &key_, BF_DECRYPT);
            }

            // append to buffer
            if (rdpos_ == 0) {
                buffer_.insert(buffer_.end(), enc.begin(), enc.end());
            }
            else {
                // if there is unread bytes, keep them and append new
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

    // read helpers working on BlowfishECBReader
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

    // ===== public key from Go =====
    static const uint8_t RSA_MODULUS[] = {
        0x51, 0xbc, 0xda, 0x08, 0x6d, 0x39, 0xfc, 0xe4,
        0x56, 0x51, 0x60, 0xd6, 0x51, 0x71, 0x3f, 0xa2,
        0xe8, 0xaa, 0x54, 0xfa, 0x66, 0x82, 0xb0, 0x4a,
        0xab, 0xdd, 0x0e, 0x6a, 0xf8, 0xb0, 0xc1, 0xe6,
        0xd1, 0xfb, 0x4f, 0x3d, 0xaa, 0x43, 0x7f, 0x15,
    };
    static const uint32_t RSA_E = 0x10001; // 65537

    // ===== derive Blowfish key from 80-byte keySource (Go's blowfishKeyFromKeySource) =====
    static std::vector<uint8_t> blowfish_key_from_keysource(const uint8_t keySource[80]) {
        // ks = keySource (80 bytes), then reverse whole buffer
        uint8_t ks[80];
        std::memcpy(ks, keySource, 80);
        byteswap(ks, 80);

        // split 40 + 40
        const uint8_t* left = ks;
        const uint8_t* right = ks + 40;

        // RSA raw transform on each half
        std::vector<uint8_t> s0 = rsa_transform_raw(left, 40, RSA_MODULUS, sizeof(RSA_MODULUS), RSA_E);
        std::vector<uint8_t> s1 = rsa_transform_raw(right, 40, RSA_MODULUS, sizeof(RSA_MODULUS), RSA_E);

        // Convert to big integers a = s0, b = s1
        BIGNUM* a = BN_bin2bn(s0.data(), (int)s0.size(), nullptr);
        BIGNUM* b = BN_bin2bn(s1.data(), (int)s1.size(), nullptr);
        BIGNUM* c = BN_new();
        BIGNUM* d = BN_new();
        BN_CTX* ctx = BN_CTX_new();

        // c = a << 312
        if (!a || !b || !c || !d || !ctx) {
            if (a) BN_free(a); if (b) BN_free(b); if (c) BN_free(c); if (d) BN_free(d);
            if (ctx) BN_CTX_free(ctx);
            return {};
        }
        if (!BN_lshift(c, a, 312)) {
            BN_free(a); BN_free(b); BN_free(c); BN_free(d); BN_CTX_free(ctx);
            return {};
        }
        // d = b + c
        if (!BN_add(d, b, c)) {
            BN_free(a); BN_free(b); BN_free(c); BN_free(d); BN_CTX_free(ctx);
            return {};
        }

        // key = d.Bytes() (minimal big-endian), then reverse entire key
        int n = BN_num_bytes(d);
        std::vector<uint8_t> key(n);
        BN_bn2bin(d, key.data());
        byteswap(key);

        BN_free(a); BN_free(b); BN_free(c); BN_free(d); BN_CTX_free(ctx);

        // Blowfish key must be 4..56 bytes. If longer, Blowfish will use only up to 56.
        if (key.size() > 56) key.resize(56);
        // If shorter than 4 (extremely unlikely), pad with zeros to 4
        if (key.size() < 4) key.resize(4, 0);

        return key;
    }

    // ===== core: unpack mix file from a stream with known total size =====
    static std::unique_ptr<MixFile> unpack_mix(std::istream& in, uint64_t total_size) {
        // Read first uint16
        uint16_t count_or_zero = read_u16(in);

        if (count_or_zero != 0) {
            uint16_t count = count_or_zero;
            (void)read_u32(in); // size (unused)
            auto entries = read_entries(in, count);

            auto mf = std::make_unique<MixFile>();
            mf->files = std::move(entries);
            mf->flags = 0;
            mf->offset = 6u + 12u * static_cast<uint32_t>(count);
            mf->size = static_cast<uint32_t>(total_size) - mf->offset;
            return mf;
        }

        // New format
        uint16_t flags16 = read_u16(in);
        uint32_t flags = uint32_t(flags16) << 16;

        constexpr uint32_t flagEncrypted = 0x00020000u;

        if (flags & flagEncrypted) {
            // read 80-byte keySource
            uint8_t keySource[80];
            if (!read_exact(in, keySource, 80)) throw std::runtime_error("read keySource failed");

            // derive Blowfish key
            std::vector<uint8_t> bf_key = blowfish_key_from_keysource(keySource);
            if (bf_key.empty()) throw std::runtime_error("derive blowfish key failed");

            // Wrap remaining stream with ECB reader
            BlowfishECBReader ecb(in, bf_key.data(), bf_key.size());

            // Encrypted index: read count, then [size][entries...]
            uint16_t count = read_u16_ecb(ecb);
            (void)read_u32_ecb(ecb); // size (unused)
            auto entries = read_entries_ecb(ecb, count);

            auto mf = std::make_unique<MixFile>();
            mf->files = std::move(entries);
            mf->flags = flags;

            // offset = 84 + align8(6 + 12*count)
            uint32_t index_len = 6u + 12u * static_cast<uint32_t>(count);
            mf->offset = 84u + align_up(index_len, 8u);
            mf->size = static_cast<uint32_t>(total_size) - mf->offset;
            return mf;
        }
        else {
            // not encrypted: read count + [size][entries...]
            uint16_t count = read_u16(in);
            (void)read_u32(in); // size (unused)
            auto entries = read_entries(in, count);

            auto mf = std::make_unique<MixFile>();
            mf->files = std::move(entries);
            mf->flags = flags;
            mf->offset = 10u + 12u * static_cast<uint32_t>(count);
            mf->size = static_cast<uint32_t>(total_size) - mf->offset;
            return mf;
        }
    }

    // write int64 LE
    static void write_i64_le(std::string& out, int64_t v) {
        uint8_t b[8];
        b[0] = uint8_t(v & 0xff);
        b[1] = uint8_t((v >> 8) & 0xff);
        b[2] = uint8_t((v >> 16) & 0xff);
        b[3] = uint8_t((v >> 24) & 0xff);
        b[4] = uint8_t((uint64_t(v) >> 32) & 0xff);
        b[5] = uint8_t((uint64_t(v) >> 40) & 0xff);
        b[6] = uint8_t((uint64_t(v) >> 48) & 0xff);
        b[7] = uint8_t((uint64_t(v) >> 56) & 0xff);
        out.append(reinterpret_cast<char*>(b), 8);
    }

    static char* build_result_buffer(const MixFile& mix, int64_t base_offset, int* outLen) {
        // total bytes = files.size() * 24
        std::string buf;
        buf.reserve(mix.files.size() * 24);

        for (const auto& e : mix.files) {
            write_i64_le(buf, static_cast<int64_t>(e.id));
            write_i64_le(buf, base_offset + static_cast<int64_t>(mix.offset) + static_cast<int64_t>(e.offset));
            write_i64_le(buf, static_cast<int64_t>(e.size));
        }

        *outLen = static_cast<int>(buf.size());
        if (*outLen == 0) return nullptr;

        char* ptr = (char*)std::malloc(buf.size());
        if (!ptr) {
            *outLen = 0;
            return nullptr;
        }
        std::memcpy(ptr, buf.data(), buf.size());
        return ptr;
    }

    // file size helper
    static uint64_t file_size(std::ifstream& f) {
        auto pos = f.tellg();
        f.seekg(0, std::ios::end);
        std::streamoff end = f.tellg();
        f.seekg(pos, std::ios::beg);
        return static_cast<uint64_t>(end);
    }

    char* GetMixInfo(const char* cPath, int* outLen) {
        // cGame is parsed in Go to validate, but does not affect parsing logic.
        // 为与 Go 行为一致：若 game 无效，返回空（这里仅做最小化兼容：空指针或空串都继续）
        try {
            if (!cPath || !outLen) return nullptr;

            std::ifstream f(cPath, std::ios::binary);
            if (!f) { *outLen = 0; return nullptr; }

            uint64_t total = file_size(f);
            f.seekg(0, std::ios::beg);

            auto mix = unpack_mix(f, total);
            return build_result_buffer(*mix, /*base_offset*/0, outLen);
        }
        catch (...) {
            if (outLen) *outLen = 0;
            return nullptr;
        }
    }

    char* GetMixInfoFromRange(const char* cPath, long long offset, long long size, int* outLen) {
        try {
            if (!cPath || !outLen) return nullptr;
            if (offset < 0 || size <= 0) { *outLen = 0; return nullptr; }

            std::ifstream f(cPath, std::ios::binary);
            if (!f) { *outLen = 0; return nullptr; }

            // Read the specified range into memory and parse from it
            f.seekg(0, std::ios::end);
            uint64_t file_end = static_cast<uint64_t>(f.tellg());
            uint64_t start = static_cast<uint64_t>(offset);
            uint64_t length = static_cast<uint64_t>(size);
            if (start > file_end || start + length > file_end) { *outLen = 0; return nullptr; }

            std::vector<char> buf(length);
            f.seekg(static_cast<std::streamoff>(start), std::ios::beg);
            if (!read_exact(f, buf.data(), buf.size())) { *outLen = 0; return nullptr; }

            std::istringstream sub(std::string(buf.data(), buf.size()));
            auto mix = unpack_mix(sub, length);
            return build_result_buffer(*mix, static_cast<int64_t>(start), outLen);
        }
        catch (...) {
            if (outLen) *outLen = 0;
            return nullptr;
        }
    }

    void FreeMixMem(char* ptr) {
        if (ptr) std::free(ptr);
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

std::vector<MixEntry> parseMixInfoRaw(const char* raw, int rawLen) {
    std::vector<MixEntry> out;
    size_t pos = 0;
    while (pos + sizeof(int64_t) * 3 <= (size_t)rawLen) {
        int64_t id = 0, off = 0, sz = 0;
        std::memcpy(&id, raw + pos, sizeof(id)); pos += sizeof(id);
        std::memcpy(&off, raw + pos, sizeof(off)); pos += sizeof(off);
        std::memcpy(&sz, raw + pos, sizeof(sz));  pos += sizeof(sz);
        out.push_back(MixEntry{ (uint32_t)id, off, sz });
    }
    return out;
}

MixLoader& MixLoader::Instance() {
    static MixLoader inst;
    return inst;
}

bool MixLoader::LoadTopMix(const std::string& path) {
    int outLen = 0;
    char* raw = MinInfo::GetMixInfo(const_cast<char*>(path.c_str()), &outLen);
    if (!raw || outLen <= 0) {
        if (raw) MinInfo::FreeMixMem(raw);
        return false;
    }

    auto entries = parseMixInfoRaw(raw, outLen);
    MinInfo::FreeMixMem(raw);

    std::ifstream fin(path, std::ios::binary);
    if (!fin.is_open()) return false;

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
    int outLen = 0;
    char* raw = MinInfo::GetMixInfoFromRange(const_cast<char*>(parent.path.c_str()),
        (long long)entry.offset,
        (long long)entry.size,
        &outLen);
    if (!raw || outLen <= 0) {
        if (raw) MinInfo::FreeMixMem(raw);
        return false;
    }

    auto entries = parseMixInfoRaw(raw, outLen);
    MinInfo::FreeMixMem(raw);

    MixFile mf;
    mf.path = parent.path;
    mf.stream.open(parent.path, std::ios::binary);
    if (!mf.stream.is_open()) return false;

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