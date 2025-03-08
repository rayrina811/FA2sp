#include "Hooks.INI.h"

/*
* Codes imported from Ares
*/

using std::map;
using std::vector;

int INIIncludes::LastReadIndex = -1;
bool INIIncludes::IsFirstINI = true;
bool INIIncludes::IsMapINI = false;
bool INIIncludes::MapINIWarn = false;
vector<CINIExt*> INIIncludes::LoadedINIs;
vector<char*> INIIncludes::LoadedINIFiles;
vector<char*> INIIncludes::RulesIncludeFiles;
map<ppmfc::CString, unsigned int> INIIncludes::CurrentINIIdxHelper;
map<ppmfc::CString, map<ppmfc::CString, ppmfc::CString>> INIIncludes::MapIncludedKeys;

DEFINE_HOOK(4530F7, CLoading_ParseINI_PlusSupport, 8)
{
    if (ExtConfigs::AllowPlusEqual)
    {
        // length [0x1000]
        GET_STACK(CINI*, pINI, STACK_OFFS(0x22FC, 0x22D0));
        LEA_STACK(char*, lpKey, STACK_OFFS(0x22FC, 0x200C));
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
    GET_STACK(CINIExt*, pINI, 0x8);

    INIIncludes::LoadedINIs.push_back(pINI);
    INIIncludes::LoadedINIFiles.push_back(_strdup(pFile));
    pINI->FileName = _strdup(pFile);

    return 0;
}

DEFINE_HOOK(480880, INIClass_LoadTSINI_IncludeSupport_2, 5)
{
    if (INIIncludes::LoadedINIs.size() == 0)
        return 0;
    CINIExt* xINI = INIIncludes::LoadedINIs.back();
    if (!xINI)
        return 0;

    bool readExtra = false;
    if (INIIncludes::IsFirstINI)
    {
        readExtra = true;
        INIIncludes::IsFirstINI = false;
    }

    auto toLower = [&](const std::string& input) {
        std::string result = input;
        std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
            return std::tolower(c);
            });
        return result;
        };
    auto toLowerC = [&](const char* input) {
        std::string result = input;
        std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
            return std::tolower(c);
            });
        return result;
        };

    char buffer[0x80]{0};

    auto fullPath = ppmfc::CString(xINI->FileName);
    std::vector<ppmfc::CString> folders;
    for (auto& f : STDHelpers::SplitString(fullPath, "\\")) {
        if (f != "") {
            folders.push_back(f);
        }
    }
    if (folders.empty())
        return 0;
    auto fileName = std::string(folders.back().m_pchData);
    folders.pop_back();

    // to fit some mods that change these names
    std::string extraName = toLower(fileName);
    bool isTheaterIni = false;
    bool isRulesIni = false;
    bool isPartOfRulesIni = false;

    if (extraName == toLowerC(CINI::FAData->GetString("Filenames", "SnowYR", "SnowMd.ini"))
        || extraName == toLowerC(CINI::FAData->GetString("Filenames", "UrbanYR", "UrbanMd.ini"))
        || extraName == toLowerC(CINI::FAData->GetString("Filenames", "UrbanNYR", "UrbanNMd.ini"))
        || extraName == toLowerC(CINI::FAData->GetString("Filenames", "LunarYR", "lunarmd.ini"))
        || extraName == toLowerC(CINI::FAData->GetString("Filenames", "DesertYR", "desertmd.ini"))
        || extraName == toLowerC(CINI::FAData->GetString("Filenames", "TemperateYR", "TemperatMd.ini"))
        )
    {
        isTheaterIni = true;
    }
    else if (extraName == toLowerC(CINI::FAData->GetString("Filenames", "RulesYR", "rulesmd.ini"))) {
        isRulesIni = true;
        isPartOfRulesIni = true;
        INIIncludes::RulesIncludeFiles.clear();
        Variables::OrderedRulesIndicies.clear();
        Variables::OrderedRulesMapIndicies.clear();
    }
    for (size_t j = 0; j < INIIncludes::RulesIncludeFiles.size(); ++j) {
        std::string name1 = toLower(INIIncludes::RulesIncludeFiles[j]);
        std::string name2 = toLower(std::string(fullPath));
        //MessageBox(0, name1.c_str(), name2.c_str(), 0);

        if (name1 == name2) {
            isPartOfRulesIni = true;
            break;
        }
    }

    if (fileName.find("fa2extra_") == std::string::npos)
        extraName = "fa2extra_" + extraName;

    if (ExtConfigs::AllowIncludes)
    {
        const char* includeSection = "#include";

        int nMix = CLoading::Instance->SearchFile(fileName.c_str());
        CINI ini;
        std::string path = CFinalSunApp::Instance->FilePath();
        std::string externalPath = CFinalSunApp::Instance->FilePath();
        for (auto& f : folders) {
            path += "\\" + f;
            externalPath += "\\" + f;
        }
        path += "\\FinalSun";
        externalPath += "\\";
        path += fileName;
        externalPath += fileName;

        std::ifstream fin;
        fin.open(externalPath, std::ios::in | std::ios::binary);
        if (fin.is_open())  {
            fin.close();
            CopyFile(externalPath.c_str(), path.c_str(), FALSE);
        }
        else if (CLoading::Instance->HasFile(fileName.c_str(), nMix)){
            CMixFile::ExtractFile(fileName.c_str(), path.c_str(), nMix);
        }

        ini.ClearAndLoad(path.c_str());
        if (isPartOfRulesIni) {
            for (const auto& section : ini.Dict) {
                auto&& cur = ini.ParseIndiciesData(section.first);
                auto& Indicies = Variables::OrderedRulesIndicies[section.first];
                for (int i = 0; i < cur.size(); i++)
                {
                    auto& key = cur[i];
                    ppmfc::CString value = ini.GetString(section.first, key, "");
                    value.Trim();
                    // the include of Ares will delete the same key in registries
                    // and then add it to the bottom
                    // it will ignore empty values
                    if (value != "") {
                        Indicies.erase(
                            std::remove_if(Indicies.begin(), Indicies.end(),
                                [&key](const std::pair<ppmfc::CString, ppmfc::CString>& item) {
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

        if (auto pSection = ini.GetSection(includeSection)) {
            for (auto& pair : pSection->GetEntities()) {
                ppmfc::CString includeFile = pair.second;
                includeFile.Trim();

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

    if (isTheaterIni)
    {
        int index;
        vector<int> newMarbles;
        vector<std::string> newMarblesSection;
        for (index = 0; index < 10000; index++)
        {
            char sectionName[114];
            sprintf(sectionName, "TileSet%04d", index);

            if (!xINI->SectionExists(sectionName))
            {
                break;
            }
            else
            {
                auto temp = std::string(xINI->GetString(sectionName, "NewMarbleMadness"));
                if (temp != "")
                {
                    newMarbles.push_back(index);
                    newMarblesSection.push_back(temp);
                }

            }
        }
        if (index > 0 && newMarbles.size() > 0)
        {
            for (size_t i = 0; i < newMarbles.size(); ++i) {
                char sectionName[114];
                char newSectionName[114];
                sprintf(sectionName, "TileSet%04d", newMarbles[i]);
                sprintf(newSectionName, "TileSet%04d", index);
                xINI->WriteString(sectionName, "MarbleMadness", std::to_string(index).c_str());

                auto section3 = xINI->AddOrGetSection(newMarblesSection[i].c_str());
                for (auto& pair : section3->GetEntities())
                {
                    auto section4 = xINI->AddOrGetSection(newSectionName);
                    xINI->WriteString(section4, pair.first, pair.second);
                }
                index++;
            }
        }
    }

    if (INIIncludes::LoadedINIs.size() > 0)
        INIIncludes::LoadedINIs.erase(INIIncludes::LoadedINIs.end() - 1);
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