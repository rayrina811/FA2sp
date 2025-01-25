#pragma once
#include "../FA2sp.h"

#include <map>
#include <fstream>

class StringtableLoader
{
public:
    static void LoadCSFFiles();
    static void LoadCSFFile(const char* pName, bool fa2path = false);
    static std::vector<ppmfc::CString> GetLinesFromBuffer(char* buffer, DWORD size);
    static bool ParseCSFFile(char* buffer, DWORD size);
    static bool ParseLLFFile(std::vector<ppmfc::CString> ret);
    static bool ParseECSFile(std::vector<ppmfc::CString> ret);
    static void WriteCSFFile();
    static bool LoadToBuffer();

    static std::map<CString, CString> CSFFiles_Stringtable;
    static char* pEDIBuffer;
    static bool bLoadRes;
};