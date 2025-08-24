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

MixEntrySet MixLoader::parseMixInfoRaw(const char* raw, int rawLen) {
    MixEntrySet out;
    size_t pos = 0;
    while (pos + sizeof(int64_t) * 3 <= (size_t)rawLen) {
        int64_t id = 0, off = 0, sz = 0;
        std::memcpy(&id, raw + pos, sizeof(id)); pos += sizeof(id);
        std::memcpy(&off, raw + pos, sizeof(off)); pos += sizeof(off);
        std::memcpy(&sz, raw + pos, sizeof(sz));  pos += sizeof(sz);
        out.insert(MixEntry{ (uint32_t)id, off, sz });
    }
    return out;
}

MixLoader& MixLoader::Instance() {
    static MixLoader inst;
    return inst;
}

void MixLoader::SetDLL(HMODULE h) {
    hDLL = h;
    if (!hDLL) throw std::runtime_error("Invalid DLL handle");

    GetMixInfo = (GetMixInfoFunc)GetProcAddress(hDLL, "GetMixInfo");
    GetMixInfoFromRange = (GetMixInfoFromRangeFunc)GetProcAddress(hDLL, "GetMixInfoFromRange");
    FreeMixMem = (FreeMixMemFunc)GetProcAddress(hDLL, "FreeMixMem");

    if (!GetMixInfo || !GetMixInfoFromRange || !FreeMixMem) {
        throw std::runtime_error("Failed to locate required exports in DLL");
    }
}

bool MixLoader::LoadTopMix(const std::string& path, const std::string& game) {
    if (!GetMixInfo || !FreeMixMem) return false;
    FString utf8Path = path;
    utf8Path.toUTF8();

    int outLen = 0;
    char* raw = GetMixInfo(const_cast<char*>(utf8Path.c_str()), const_cast<char*>(game.c_str()), &outLen);
    if (!raw || outLen <= 0) {
        if (raw) FreeMixMem(raw);
        return false;
    }

    auto entries = parseMixInfoRaw(raw, outLen);
    FreeMixMem(raw);

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

bool MixLoader::LoadNestedMix(MixFile& parent, const MixEntry& entry, const std::string& game) {
    if (!GetMixInfoFromRange || !FreeMixMem) return false;

    FString utf8Path = parent.path;
    utf8Path.toUTF8();

    int outLen = 0;

    char* raw = GetMixInfoFromRange(const_cast<char*>(utf8Path.c_str()),
        const_cast<char*>(game.c_str()),
        (long long)entry.offset,
        (long long)entry.size,
        &outLen);
    if (!raw || outLen <= 0) {
        if (raw) FreeMixMem(raw);
        return false;
    }

    auto entries = parseMixInfoRaw(raw, outLen);
    FreeMixMem(raw);

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

bool MixLoader::LoadMixFile(const std::string& path, const std::string& game, int* parentIndex) {
    if (std::filesystem::exists(path)) {
        if (parentIndex) *parentIndex = -1;
        return LoadTopMix(path, game);
    }

    auto name = std::filesystem::path(path).filename().string();
    int id = GetFileID(name.c_str());
    for (size_t i = 0; i < mixFiles.size(); ++i) {
        auto& mf = mixFiles[i];
        for (const auto& e : mf.entries) {
            if (e.id == id) {
                if (parentIndex) *parentIndex = i;
                return LoadNestedMix(mf, e, game);
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