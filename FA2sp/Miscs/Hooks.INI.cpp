#include "Hooks.INI.h"
#include "../Ext/CMapData/Body.h"
#include "../Ext/CLoading/Body.h"


/*
* Codes imported from Ares
*/

using std::map;
using std::vector;

int INIIncludes::LastReadIndex = -1;
bool INIIncludes::IsFirstINI = true;
bool INIIncludes::IsMapINI = false;
bool INIIncludes::MapINIWarn = false;
vector<CINI*> INIIncludes::LoadedINIs;
vector<char*> INIIncludes::LoadedINIFiles;
vector<char*> INIIncludes::RulesIncludeFiles;
map<FString, unsigned int> INIIncludes::CurrentINIIdxHelper;
std::unordered_map<FString, std::unordered_map<FString, FString>> INIIncludes::MapIncludedKeys;
std::unordered_map<CINI*, CINIExt> CINIManager::propertyMap;
bool INIIncludes::SkipBracketFix = false;

static void Trim(char* str) {
    if (!str) return;
    char* start = str;
    while (*start && isspace(static_cast<unsigned char>(*start))) {
        start++;
    }
    if (*start == '\0') {
        *str = '\0';
        return;
    }
    char* end = start + strlen(start) - 1;
    while (end > start && isspace(static_cast<unsigned char>(*end))) {
        end--;
    }
    *(end + 1) = '\0';
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

DEFINE_HOOK(452EFB, CLoading_ParseINI_BracketFix_inheritSupport, 7)
{
    if (INIIncludes::SkipBracketFix)
        return 0;

    LEA_STACK(char*, lpLine, STACK_OFFS(0x22FC, 0x200C));

    const char* lb1 = strchr(lpLine, '[');
    const char* rb1 = strchr(lpLine, ']');
    const char* eq = strchr(lpLine, '=');

    if (!lb1 || !rb1 || rb1 < lb1 || (eq && eq < lb1)) {
        // illegal section name
        return 0x453012;
    }

    // ares mode
    if (ExtConfigs::AllowInherits && !ExtConfigs::InheritType)
    {
        if (rb1[1] == ':' && rb1[2] == '[') {
            const char* lb2 = rb1 + 2;
            const char* rb2 = strchr(lb2, ']');
            if (rb2) {

                char sectionA[128] = { 0 };
                char sectionB[128] = { 0 };

                size_t lenA = rb1 - (lb1 + 1);
                size_t lenB = rb2 - (lb2 + 1); 

                if (lenA < sizeof(sectionA) && lenB < sizeof(sectionB)) {
                    GET_STACK(CINI*, pINI, STACK_OFFS(0x22FC, 0x22D0));

                    memcpy(sectionA, lb1 + 1, lenA);
                    sectionA[lenA] = '\0';

                    memcpy(sectionB, lb2 + 1, lenB);
                    sectionB[lenB] = '\0';

                    if (auto pSectionB = pINI->GetSection(sectionB))
                    {
                        for (const auto& [key, value] : pSectionB->GetEntities())
                        {
                            pINI->WriteString(sectionA, key, value);
                        }
                    }
                }
            }
        }
    }

    // normal [Section]
    return 0x452F56;
}

// some maps use '[' for encryption
DEFINE_HOOK(49D64F, CMapData_LoadMap_SkipBracketFix, 6)
{
    INIIncludes::SkipBracketFix = false;
    return 0;
}

DEFINE_HOOK(4530F7, CLoading_ParseINI_PlusSupport, 8)
{
    LEA_STACK(char*, lpKey, STACK_OFFS(0x22FC, 0x200C));
    LEA_STACK(char*, lpValue, STACK_OFFS(0x22FC, 0x100C));

    Trim(lpKey);
    Trim(lpValue);

    if (ExtConfigs::AllowPlusEqual)
    {
        GET_STACK(CINI*, pINI, STACK_OFFS(0x22FC, 0x22D0));
        LEA_STACK(const char*, lpSection, STACK_OFFS(0x22FC, 0x210C));
        if (strcmp(lpKey, "+") == 0)
        {
            unsigned int& attempt = INIIncludes::CurrentINIIdxHelper[lpSection];
            while (true)
            {
                sprintf_s(lpKey, 0x1000, "FA2sp%u", attempt);
                if (!pINI->KeyExists(lpSection, lpKey))
                    break;
                ++attempt;
                if (attempt >= 10000)
                {
                    sprintf_s(lpKey, 0x1000, "+");
                    break;
                }
            }
        }
    }
    return 0;
}

DEFINE_HOOK(47FFB0, CLoading_LoadTSINI_IncludeSupport_1, 7)
{

    GET_STACK(const char*, pFile, 0x4);
    GET_STACK(CINI*, pINI, 0x8);

    INIIncludes::LoadedINIs.push_back(pINI);
    INIIncludes::LoadedINIFiles.push_back(_strdup(pFile));
    CINIManager::GetInstance().SetProperty(pINI, { pFile });

    return 0;
}

DEFINE_HOOK(48028D, CLoading_LoadTSINI_PackageSupport, 5)
{
    CINI* xINI = INIIncludes::LoadedINIs.back();
    if (!xINI)
        return 0;

    FString filename = CINIManager::GetInstance().GetProperty(xINI).Name;

    size_t size = 0;
    auto data = ResourcePackManager::instance().getFileData(filename, &size);
    if (data && size > 0)
    {
        FString out_path = CFinalSunApp::Instance->FilePath();
        out_path += "\\FinalSun";
        out_path += filename;

        std::ofstream fout(out_path, std::ios::binary);
        if (fout.is_open()) 
        {
            fout.write(reinterpret_cast<const char*>(data.get()), static_cast<std::streamsize>(size));
            fout.close();

            // load as a mix ini
            return 0x480337;
        }
    }

    return 0;
}

DEFINE_HOOK(480880, INIClass_LoadTSINI_IncludeSupport_2, 5)
{
    if (INIIncludes::LoadedINIs.size() == 0)
        return 0;
    CINI* xINI = INIIncludes::LoadedINIs.back();
    if (!xINI)
        return 0;

    bool readExtra = false;
    if (INIIncludes::IsFirstINI)
    {
        readExtra = true;
        INIIncludes::IsFirstINI = false;
    }

    auto toLower = [&](const FString& input) {
        FString result = input;
        std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
            return std::tolower(c);
            });
        return result;
        };
    auto toLowerC = [&](const char* input) {
        FString result = input;
        std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
            return std::tolower(c);
            });
        return result;
        };

    char buffer[0x80]{0};

    FString fullPath = CINIManager::GetInstance().GetProperty(xINI).Name;
    std::vector<FString> folders;
    for (auto& f : STDHelpers::SplitString(fullPath, "\\")) {
        if (f != "") {
            folders.push_back(f);
        }
    }
    if (folders.empty())
        return 0;
    auto fileName = FString(folders.back());
    folders.pop_back();

    // to fit some mods that change these names
    FString extraName = toLower(fileName);
    int theaterIniType = -1;
    bool isPartOfRulesIni = false;

    auto getOriTileSetName = [xINI](int type)
        {
            FString sName = "";
            for (int index = 0; index < 10000; index++)
            {
                sName.Format("TileSet%04d", index);
                if (xINI->SectionExists(sName))
                {
                    auto setName = xINI->GetString(sName, "SetName");
                    auto& set = CMapDataExt::TileSetOriginSetNames[type];
                    if (set.find(index) == set.end())
                    {
                        set[index] = setName;
                    }
                }
                else break;
            }
        };

    if (extraName == toLowerC(CINI::FAData->GetString("Filenames", "TemperateYR", "TemperatMd.ini")))
    {
        theaterIniType = 0;
    }
    else if (extraName == toLowerC(CINI::FAData->GetString("Filenames", "SnowYR", "SnowMd.ini")))
    {
        theaterIniType = 1;
    }
    else if (extraName == toLowerC(CINI::FAData->GetString("Filenames", "UrbanYR", "UrbanMd.ini")))
    {
        theaterIniType = 2;
    }
    else if (extraName == toLowerC(CINI::FAData->GetString("Filenames", "UrbanNYR", "UrbanNMd.ini")))
    {
        theaterIniType = 3;
    }
    else if (extraName == toLowerC(CINI::FAData->GetString("Filenames", "LunarYR", "lunarmd.ini")))
    {
        theaterIniType = 4;
    }
    else if (extraName == toLowerC(CINI::FAData->GetString("Filenames", "DesertYR", "desertmd.ini")))
    {
        theaterIniType = 5;
    }
    else if (extraName == toLowerC(CINI::FAData->GetString("Filenames", "RulesYR", "rulesmd.ini"))) {
        isPartOfRulesIni = true;
        INIIncludes::RulesIncludeFiles.clear();
        Variables::OrderedRulesIndicies.clear();
        Variables::OrderedRulesMapIndicies.clear();
    }
    for (size_t j = 0; j < INIIncludes::RulesIncludeFiles.size(); ++j) {
        FString name1 = toLower(INIIncludes::RulesIncludeFiles[j]);
        FString name2 = toLower(FString(fullPath));

        if (name1 == name2) {
            isPartOfRulesIni = true;
            break;
        }
    }

    if (theaterIniType > -1)
        getOriTileSetName(theaterIniType);

    if (fileName.find("fa2extra_") == FString::npos)
        extraName = "fa2extra_" + extraName;

    if (ExtConfigs::AllowIncludes || isPartOfRulesIni)
    {
        const char* includeSection = ExtConfigs::IncludeType ? "$Include" : "#include";

        int nMix = CLoading::Instance->SearchFile(fileName.c_str());
        CINI ini;
        FString path = CFinalSunApp::Instance->FilePath();
        FString externalPath = CFinalSunApp::Instance->FilePath();
        for (auto& f : folders) {
            path += "\\" + f;
            externalPath += "\\" + f;
        }
        path += "\\FinalSun";
        externalPath += "\\";
        path += fileName;
        externalPath += fileName;

        bool copied = false;
        std::ifstream fin;
        fin.open(externalPath, std::ios::in | std::ios::binary);
        if (fin.is_open())  {
            fin.close();
            CopyFile(externalPath.c_str(), path.c_str(), FALSE);
            copied = true;
        }
        if (!copied)
        {
            size_t size = 0;
            auto data = ResourcePackManager::instance().getFileData(fileName, &size);
            if (data && size > 0)
            {
                std::ofstream fout(path, std::ios::binary);
                if (fout.is_open())
                {
                    fout.write(reinterpret_cast<const char*>(data.get()), static_cast<std::streamsize>(size));
                    fout.close();
                    copied = true;
                }
            }
        }
        if (!copied)
        {
            if (CLoading::Instance->HasFile(fileName.c_str(), nMix)) {
                CMixFile::ExtractFile(fileName.c_str(), path.c_str(), nMix);
                copied = true;
            }
        }

        ini.ClearAndLoad(path.c_str());
        if (isPartOfRulesIni) {
            for (const auto& section : ini.Dict) {
                auto&& cur = ini.ParseIndiciesData(section.first);
                auto& Indicies = Variables::OrderedRulesIndicies[section.first];
                for (int i = 0; i < cur.size(); i++)
                {
                    auto& key = cur[i];
                    FString value = ini.GetString(section.first, key, "");
                    // the include of Ares will delete the same key in registries
                    // and then add it to the bottom
                    // it will ignore empty values
                    if (value != "") {
                        Indicies.erase(
                            std::remove_if(Indicies.begin(), Indicies.end(),
                                [&key](const std::pair<FString, FString>& item) {
                                    return item.first == key;
                                }),
                            Indicies.end()
                        );

                        bool same = false;
                        for (auto& checkSame : Indicies) {
                            if (checkSame.second == value) {
                                same = true;
                                break;
                            }
                        }
                        if (!same) {
                            Indicies.push_back(std::make_pair(key, value));
                        }
                    }
                }
            }
            Variables::OrderedRulesMapIndicies = Variables::OrderedRulesIndicies;
        }

        if (ExtConfigs::AllowIncludes)
        {
            if (auto pSection = ini.GetSection(includeSection)) {
                for (auto& pair : pSection->GetEntities()) {
                    const ppmfc::CString& includeFile = pair.second;

                    if (includeFile && strlen(includeFile) > 0) {
                        bool canLoad = true;
                        for (size_t j = 0; j < INIIncludes::LoadedINIFiles.size(); ++j) {
                            if (!strcmp(INIIncludes::LoadedINIFiles[j], includeFile)) {
                                canLoad = false;
                                break;
                            }
                        }
                        if (isPartOfRulesIni) {
                            INIIncludes::RulesIncludeFiles.push_back(_strdup(includeFile));
                        }

                        if (canLoad) {
                            if (!INIIncludes::IsMapINI)
                                Logger::Debug("Include Ext Loaded File: %s\n", includeFile);
                            else
                                Logger::Debug("Include Ext Loaded File in Map: %s\n", includeFile);
                            CLoading::Instance->LoadTSINI(
                                includeFile, (CINI*)xINI, TRUE
                            );
                        }
                    }
                }
            }
        }
        DeleteFile(path.c_str());
    }

    ++INIIncludes::LastReadIndex;
    buffer[0] = '\0';

    if (readExtra)
    {
        INIIncludes::IsFirstINI = true;
        // only load extra file in the main INI
        strcpy_s(buffer, extraName.c_str());
        if (buffer && strlen(buffer) > 0) {
            bool canLoad = true;
            for (size_t j = 0; j < INIIncludes::LoadedINIFiles.size(); ++j) {
                if (!strcmp(INIIncludes::LoadedINIFiles[j], buffer)) {
                    canLoad = false;
                    break;
                }

            }

            if (canLoad) {
                if (!INIIncludes::IsMapINI)
                    Logger::Debug("Include Ext Loaded File: %s\n", buffer);
                else
                    Logger::Debug("Include Ext Loaded File in Map: %s\n", buffer);
                CLoading::Instance->LoadTSINI(
                    buffer, (CINI*)xINI, TRUE
                );
            }
        }
    } 

    if (theaterIniType > -1)
    {
        int index;
        std::map<int, FString> newMarbles;
        for (index = 0; index < 10000; index++)
        {
            FString sectionName;
            sectionName.Format("TileSet%04d", index);
            if (!xINI->SectionExists(sectionName))
            {
                break;
            }
            else
            {
                auto nmm = xINI->GetString(sectionName, "NewMarbleMadness");
                if (nmm != "")
                {
                    newMarbles[index] = nmm;
                }

            }
        }
        if (index > 0 && newMarbles.size() > 0)
        {
            for (auto& [i, secName] : newMarbles) {

                FString oldSectionName;
                FString newSectionName;
                oldSectionName.Format("TileSet%04d", i);
                newSectionName.Format("TileSet%04d", index);
                xINI->WriteString(oldSectionName, "MarbleMadness", std::to_string(index).c_str());

                auto section = xINI->AddOrGetSection(secName);
                for (auto& pair : section->GetEntities())
                {
                    auto newSection = xINI->AddOrGetSection(newSectionName);
                    xINI->WriteString(newSection, pair.first, pair.second);
                }
                index++;
            }
        }
        getOriTileSetName(theaterIniType);
    }

    // phobos mode
    if (ExtConfigs::AllowInherits && ExtConfigs::InheritType)
    {
        for (const auto& sectionA : xINI->Dict) 
        {
            if (xINI->KeyExists(sectionA.first, "$Inherits"))
            {
                auto inherits = STDHelpers::SplitString(xINI->GetString(sectionA.first, "$Inherits"));
                for (const auto& sectionB : inherits)
                {
                    if (auto pSectionB = xINI->GetSection(sectionB))
                    {
                        for (const auto& [key, value] : pSectionB->GetEntities())
                        {
                            if (!xINI->KeyExists(sectionA.first, key))
                                xINI->WriteString(sectionA.first, key, value);
                        }
                    }
                }
            }
        }
    }

    if (INIIncludes::LoadedINIs.size() > 0)
    {
        auto ini_to_remove = INIIncludes::LoadedINIs.end() - 1;
        CINIManager::GetInstance().RemoveInstance(*ini_to_remove);
        INIIncludes::LoadedINIs.erase(ini_to_remove);
    }

    if (!INIIncludes::LoadedINIs.size()) {
        for (int j = INIIncludes::LoadedINIFiles.size() - 1; j >= 0; --j) {
            if (char* ptr = INIIncludes::LoadedINIFiles[j]) {
                free(ptr);
                INIIncludes::CurrentINIIdxHelper.clear();
            }
            INIIncludes::LoadedINIFiles.erase(INIIncludes::LoadedINIFiles.begin() + j);
        }
        INIIncludes::LastReadIndex = -1;
    }

    return 0;
}