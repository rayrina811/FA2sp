#include "Translations.h"

#include <CFinalSunApp.h>
#include <Helpers/Macro.h>
#include <CINI.h>
#include "../FA2sp.h"
#include <CMapData.h>

CString FinalAlertConfig::lpPath;
char FinalAlertConfig::pLastRead[0x400];

// Load after ExePath is initialized
DEFINE_HOOK(41F7F5, Translations_Initialzation, 9)
{
    FinalAlertConfig::lpPath = CFinalSunApp::ExePath();
    FinalAlertConfig::lpPath += "FinalAlert.ini";
    FinalAlertConfig::ReadString("FinalSun", "Language", "English");
    strcpy_s(Translations::pLanguage[0], FinalAlertConfig::pLastRead);
    strcpy_s(Translations::pLanguage[1], FinalAlertConfig::pLastRead);
    strcpy_s(Translations::pLanguage[2], FinalAlertConfig::pLastRead);
    strcpy_s(Translations::pLanguage[3], FinalAlertConfig::pLastRead);
    strcat_s(Translations::pLanguage[0], "-StringsRA2");
    strcat_s(Translations::pLanguage[1], "-TranslationsRA2");
    strcat_s(Translations::pLanguage[2], "-Strings");
    strcat_s(Translations::pLanguage[3], "-Translations");
    return 0;
}

DWORD FinalAlertConfig::ReadString(const char* pSection, const char* pKey, const char* pDefault, char* pBuffer)
{
    DWORD dwRet = GetPrivateProfileString(pSection, pKey, pDefault, FinalAlertConfig::pLastRead, 0x400, lpPath);
    if (pBuffer)
        strcpy_s(pBuffer, 0x400, pLastRead);
    return dwRet;
}
void FinalAlertConfig::WriteString(const char* pSection, const char* pKey, const char* pContent)
{
    WritePrivateProfileString(pSection, pKey, pContent, lpPath);
};

char Translations::pLanguage[4][0x400];
bool Translations::GetTranslationItem(const char* pLabelName, ppmfc::CString& ret)
{
    auto& falanguage = CINI::FALanguage();

    for(const auto& language : Translations::pLanguage)
        if (auto section = falanguage.GetSection(language))
        {
            auto itr = section->GetEntities().find(pLabelName);
            if (itr != section->GetEntities().end())
            {
                auto buffer = itr->second;
                buffer.Replace("\\n", "\n");
                buffer.Replace("\\t", "\t");
                ret = buffer;
                return true;
            }
        }

        return false;
}

const char* Translations::TranslateOrDefault(const char* lpLabelName, const char* lpDefault)
{
    for (const auto& language : Translations::pLanguage)
    {
        if (auto section = CINI::FALanguage->GetSection(language))
        {
            auto itr = section->GetEntities().find(lpLabelName);
            if (itr != section->GetEntities().end())
            {
                auto buffer = itr->second;
                buffer.Replace("\\n", "\n");
                buffer.Replace("\\t", "\t");
                buffer.Replace("\\r", "\r");
                return buffer;
            }
                
        }
    }

    return lpDefault;
}
const char* Translations::TranslateStringVariables(int n, const char* originaltext, const char* inserttext)
{
    char c[50];
    _itoa(n, c, 10);

    char seekedstring[50];
    seekedstring[0] = '%';
    seekedstring[1] = 0;
    strcat(seekedstring, c);

    ppmfc::CString orig = originaltext;
    if (orig.Find(seekedstring) < 0) return orig;

    orig.Replace(seekedstring, inserttext);

    return orig;
}

void Translations::TranslateItem(CWnd* pWnd, int nSubID, const char* lpKey)
{
    ppmfc::CString buffer;
    if (Translations::GetTranslationItem(lpKey, buffer))
        pWnd->SetDlgItemText(nSubID, buffer);
}

void Translations::TranslateItem(CWnd* pWnd, const char* lpKey)
{
    ppmfc::CString buffer;
    if (Translations::GetTranslationItem(lpKey, buffer))
        pWnd->SetWindowText(buffer);
}

DEFINE_HOOK(43C3C0, Miscs_ParseHouseName, 7)
{
    if (ExtConfigs::NoHouseNameTranslation)
    {
        GET_STACK(ppmfc::CString*, pRet, 0x4);
        REF_STACK(ppmfc::CString, src, 0x8);
        GET_STACK(bool, IDToUIName, 0xC);

        new(pRet) ppmfc::CString(src);
        R->EAX(pRet);

        return 0x43CA72;
    }
    else
    {
        GET_STACK(ppmfc::CString*, pRet, 0x4);
        REF_STACK(ppmfc::CString, src, 0x8);
        GET_STACK(bool, IDToUIName, 0xC);


        auto& countries = CINI::Rules->GetSection("Countries")->GetEntities();
        ppmfc::CString translated;

        if (IDToUIName)
        {
            for (auto& pair : countries)
            {
                if (ExtConfigs::BetterHouseNameTranslation)
                    translated = CMapData::GetUIName(pair.second) + "(" + pair.second + ")";
                else
                    translated = CMapData::GetUIName(pair.second);

                src.Replace(pair.second, translated);
            }
        }
        else
        {
            for (auto& pair : countries)
            {
                if (ExtConfigs::BetterHouseNameTranslation)
                    translated = CMapData::GetUIName(pair.second) + "(" + pair.second + ")";
                else
                    translated = CMapData::GetUIName(pair.second);
                src.Replace(translated, pair.second);
            }
        }
        new(pRet) ppmfc::CString(src);
        R->EAX(pRet);

        return 0x43CA72;

    }
    return 0;
}