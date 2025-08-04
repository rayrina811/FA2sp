#pragma once
#include <CINI.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <CLoading.h>

#include "../Helpers/STDHelpers.h"
#include "../FA2sp.h"
#include <CFinalSunApp.h>
#include <CMixFile.h>
#include <fstream>

using std::map;
using std::vector;

struct CINIExt
{
    ppmfc::CString Name;
};

class CINIManager 
{
private:
    static std::unordered_map<CINI*, CINIExt> propertyMap;
    CINIManager() = default;

public:
    static CINIManager& GetInstance() {
        static CINIManager instance;
        return instance;
    }

    void SetProperty(CINI* instance, CINIExt value) {
        if (instance) {
            propertyMap[instance] = value;
        }
    }

    CINIExt GetProperty(CINI* instance, CINIExt defaultValue = {}) {
        auto it = propertyMap.find(instance);
        return (it != propertyMap.end()) ? it->second : defaultValue;
    }

    void RemoveInstance(CINI* instance) {
        propertyMap.erase(instance);
    }

};

class INIIncludes
{
public:
    static int LastReadIndex;
    static vector<CINI*> LoadedINIs;
    static vector<char*> LoadedINIFiles;
    static map<ppmfc::CString, unsigned int> CurrentINIIdxHelper;
    static vector<char*> RulesIncludeFiles;
    static bool IsFirstINI;
    static bool IsMapINI;
    static bool MapINIWarn;
    static bool SkipBracketFix;
    static std::unordered_map<ppmfc::CString, std::unordered_map<ppmfc::CString, ppmfc::CString>> MapIncludedKeys;
};