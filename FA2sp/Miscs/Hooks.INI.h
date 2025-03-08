#pragma once
#include <CINI.h>
#include <vector>
#include <map>
#include <CLoading.h>

#include "../Helpers/STDHelpers.h"
#include "../FA2sp.h"
#include <CFinalSunApp.h>
#include <CMixFile.h>
#include <fstream>

using std::map;
using std::vector;

class CINIExt : public CINI
{
public:
    char* FileName;
};


class INIIncludes
{
public:
    static int LastReadIndex;
    static vector<CINIExt*> LoadedINIs;
    static vector<char*> LoadedINIFiles;
    static map<ppmfc::CString, unsigned int> CurrentINIIdxHelper;
    static vector<char*> RulesIncludeFiles;
    static bool IsFirstINI;
    static bool IsMapINI;
    static bool MapINIWarn;
    static map<ppmfc::CString, map<ppmfc::CString, ppmfc::CString>> MapIncludedKeys;
};