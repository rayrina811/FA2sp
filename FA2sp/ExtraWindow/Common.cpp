#include "Common.h"
#include "CNewTrigger/CNewTrigger.h"
#include "../Helpers/STDHelpers.h"
#include "CNewScript/CNewScript.h"
#include "../Helpers/Translations.h"
#include "CObjectSearch/CObjectSearch.h"
#include "../Helpers/FString.h"
#include <regex>
#include <string>
#include <vector>
#include <algorithm>
#include <CFinalSunApp.h>
#include "../Miscs/StringtableLoader.h"
#include "../Ext/CMapData/Body.h"

CINI& ExtraWindow::map = CINI::CurrentDocument;
CINI& ExtraWindow::fadata = CINI::FAData;
MultimapHelper& ExtraWindow::rules = Variables::RulesMap;

bool ExtraWindow::bComboLBoxSelected = false;
bool ExtraWindow::bEnterSearch = false;

FString ExtraWindow::GetTeamDisplayName(const char* id)
{
    FString name;
    name.Format("%s (%s)", id, map.GetString(id, "Name"));
    return name;
}
FString ExtraWindow::GetAITriggerDisplayName(const char* id)
{
    FString name = "";
    auto atoms = FString::SplitString(map.GetString("AITriggerTypes", id));
    if (atoms.size() < 1)
        return name;

    name.Format("%s (%s)", id, atoms[0]);
    return name;
}
void ExtraWindow::SetEditControlFontSize(HWND hWnd, float nFontSizeMultiplier, bool richEdit, const char* newFont)
{
    if (richEdit)
    {
        CHARFORMAT cf;
        ZeroMemory(&cf, sizeof(cf));
        cf.cbSize = sizeof(cf);
        SendMessage(hWnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

        int currentSize = cf.yHeight / 10;
        int newSize = (int)(currentSize * nFontSizeMultiplier);

        ZeroMemory(&cf, sizeof(cf));
        cf.cbSize = sizeof(cf);
        cf.dwMask = CFM_SIZE | CFM_FACE;
        cf.yHeight = newSize * 10;
        if (strcmp(newFont, "") != 0)
            lstrcpy(cf.szFaceName, newFont);
        SendMessage(hWnd, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    }
    else
    {
        HFONT hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
        LOGFONT logfont = { 0 };

        if (hFont) {
            GetObject(hFont, sizeof(LOGFONT), &logfont);
        }
        logfont.lfHeight *= nFontSizeMultiplier;        
        if (strcmp(newFont, "") != 0)
            lstrcpy(logfont.lfFaceName, newFont);


        HFONT hNewFont = CreateFontIndirect(&logfont);
        SendMessage(hWnd, WM_SETFONT, (WPARAM)hNewFont, TRUE);

    }
}

FString ExtraWindow::FormatTriggerDisplayName(const char* id, const char* name)
{
    FString ret;
    ret.Format("%s (%s)", id, name);
    return ret;
}

FString ExtraWindow::GetTriggerDisplayName(const char* id)
{
    FString name;
    if (strcmp(id, "<none>") == 0)
        return id;
    auto atoms = FString::SplitString(map.GetString("Triggers", id, "Americans,<none>,MISSING,0,1,1,1,0"));
    name.Format("%s (%s)", id, atoms[2]);
    return name;
}

FString ExtraWindow::GetTriggerName(const char* id)
{
    FString name;
    if (strcmp(id, "<none>") == 0)
        return id;
    auto atoms = FString::SplitString(map.GetString("Triggers", id, "Americans,<none>,MISSING,0,1,1,1,0"));
    return atoms[2];
}

FString ExtraWindow::GetAITriggerName(const char* id)
{
    FString name;
    if (strcmp(id, "<none>") == 0)
        return id;
    auto atoms = FString::SplitString(map.GetString("AITriggerTypes", id, "MISSING"));
    return atoms[0];
}

FString ExtraWindow::GetTagName(const char* id)
{
    FString name;
    if (strcmp(id, "<none>") == 0)
        return id;
    auto atoms = FString::SplitString(map.GetString("Tags", id, "0,MISSING,01000000"));
    return atoms[1];
}

FString ExtraWindow::GetEventDisplayName(const char* id, int index)
{
    FString name;
    FString name2;
    FString atom = FString::SplitString(fadata.GetString("EventsRA2", id, "MISSING"))[0];
    name.Format("%s %s", id, atom);
    if (index >= 0)
        name2.Format("[%d] %s", index, name);
    else name2 = name;
    return name2;
}

FString ExtraWindow::GetActionDisplayName(const char* id, int index)
{
    FString name;
    FString name2;
    FString atom = FString::SplitString(fadata.GetString("ActionsRA2", id, "MISSING"))[0];
    name.Format("%s %s", id, atom);
    if (index >= 0)
        name2.Format("[%d] %s", index, name);
    else name2 = name;
    return name2;
}

int ExtraWindow::FindCBStringExactStart(HWND hComboBox, const char* searchText)
{
    int itemCount = SendMessage(hComboBox, CB_GETCOUNT, 0, 0);
    for (int i = 0; i < itemCount; ++i) {
        char buffer[256]{ 0 };
        SendMessage(hComboBox, CB_GETLBTEXT, i, reinterpret_cast<LPARAM>(buffer));

        if (strncmp(buffer, searchText, strlen(searchText)) == 0) {
            return i;
        }
    }
    return -1;
}

void ExtraWindow::AdjustDropdownWidth(HWND hWnd)
{
    if (ExtConfigs::AdjustDropdownWidth)
    {
        int itemCount = (int)SendMessage(hWnd, CB_GETCOUNT, 0, 0);
        int maxWidth = 120;
        for (int i = 0; i < itemCount; ++i)
        {
            TCHAR buffer[512];
            SendMessage(hWnd, CB_GETLBTEXT, i, (LPARAM)buffer);

            int thisSize = strlen(buffer) * ExtConfigs::AdjustDropdownWidth_Factor;
            if (thisSize > maxWidth)
                maxWidth = thisSize;
            if (maxWidth > ExtConfigs::AdjustDropdownWidth_Max)
            {
                maxWidth = ExtConfigs::AdjustDropdownWidth_Max;
                break;
            }
        }
        SendMessage(hWnd, CB_SETDROPPEDWIDTH, maxWidth, 0);
    }
}

void ExtraWindow::SyncComboBoxContent(HWND hSource, HWND hTarget, bool addNone)
{
    SendMessage(hTarget, CB_RESETCONTENT, 0, 0);
    if (addNone)
        SendMessage(hTarget, CB_INSERTSTRING, 0, (LPARAM)(LPCSTR)"<none>");

    int count = (int)SendMessage(hSource, CB_GETCOUNT, 0, 0);
    for (int i = 0; i < count; i++)
    {
        char buffer[512]{ 0 };
        SendMessage(hSource, CB_GETLBTEXT, i, (LPARAM)buffer);
        if (addNone)
            SendMessage(hTarget, CB_INSERTSTRING, i + 1, (LPARAM)buffer);
        else
            SendMessage(hTarget, CB_INSERTSTRING, i, (LPARAM)buffer);
    }
}

void ExtraWindow::LoadParams(HWND& hWnd, FString idx)
{
    FString addonN1 = "-1 - ";
    FString addonN2 = "-2 - ";
    FString addonN3 = "-3 - ";

    while (SendMessage(hWnd, CB_DELETESTRING, 0, NULL) != CB_ERR);
    switch (atoi(idx)) {
    case 1:
        LoadParam_Waypoints(hWnd);
        break;
    case 2:
        LoadParam_ActionList(hWnd);
        break;
    case 3:
        LoadParam_CountryList(hWnd);
        LoadParam_HouseAddon_Multi(hWnd);

        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)(addonN1 + Translations::TranslateOrDefault("NonNeutralrandomhouse", "Non-Neutral random house")).c_str());
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)(addonN2 + Translations::TranslateOrDefault("FirstNeutralhouse", "First Neutral house")).c_str());
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)(addonN3 + Translations::TranslateOrDefault("RandomHumanplayer", "Random Human player")).c_str());
        break;
    case 4:
        LoadParam_CountryList(hWnd);
        LoadParam_HouseAddon_Multi(hWnd);
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)(addonN1 + Translations::TranslateOrDefault("Anyhouse", "Any house")).c_str());
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)(addonN2 + Translations::TranslateOrDefault("Triggerhouse", "Trigger house")).c_str());
        break;
    case 5:
        LoadParam_CountryList(hWnd);
        LoadParam_HouseAddon_Multi(hWnd);
        break;
    case 6:
        LoadParam_CountryList(hWnd);
        LoadParam_HouseAddon_MultiAres(hWnd);
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)(addonN1 + Translations::TranslateOrDefault("Anyhouse", "Any house")).c_str());
        break;
    case 7:
        LoadParam_CountryList(hWnd);
        LoadParam_HouseAddon_MultiAres(hWnd);
        break;
    case 8:
        LoadParam_TechnoTypes(hWnd);
        break;
    case 9:
        LoadParam_Triggers(hWnd);
        break;
    case 10:
        if (!ExtConfigs::TutorialTexts_Viewer)
            LoadParam_Stringtables(hWnd);
        break;
    case 11:
        LoadParam_Tags(hWnd);
        break;
    case 12: // float
        CNewTrigger::ActionParamUsesFloat = true;
        break;
    case 13:
        LoadParam_CountryList(hWnd);
        LoadParam_HouseAddon_Multi(hWnd);
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)(addonN1 + Translations::TranslateOrDefault("Allhouse", "All house")).c_str());
        break;
    case 14:
        LoadParam_CountryList(hWnd);
        LoadParam_HouseAddon_Multi(hWnd);
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)(addonN1 + Translations::TranslateOrDefault("CancelForceEnemy", "Cancel force enemy")).c_str());
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)(addonN2 + Translations::TranslateOrDefault("ForceNoEnemy", "Force no enemy")).c_str());
        break;
    default:
        if (atoi(idx) >= 500)
        {
            if (auto pSectionNewParamTypes = fadata.GetSection("NewParamTypes"))
            {
                auto atoms3 = FString::SplitString(fadata.GetString("NewParamTypes", idx), 4);
                auto& sectionName = atoms3[0];
                auto& loadFrom = atoms3[1];
                auto& strictOrder = atoms3[2];
                auto& showUIName = atoms3[3];
                auto& useValue = atoms3[4];
                MultimapHelper mmh;
                
                LoadFrom(mmh, loadFrom);

                if (useValue == "1")
                {
                    int i = 0;
                    for (auto& kvp : mmh.GetSection(sectionName))
                    {
                        FString output;
                        output.Format("%s", kvp.second);
                        if (showUIName == "1")
                        {
                            FString uiname = CViewObjectsExt::QueryUIName(kvp.second, true);
                            if (uiname != kvp.second && uiname != "")
                            {
                                FString tmp = output;
                                output.Format("%s - %s", tmp, uiname);
                            }
                        }
                        SendMessage(hWnd, CB_INSERTSTRING, i, output);
                        i++;
                    }
                }
                else
                {
                    if (strictOrder == "1")
                    {
                        for (auto pINI : mmh.GetINIData())
                        {
                            if (pINI == &CINI::CurrentDocument)
                            {
                                // refresh indicies
                                CMapDataExt::UpdateMapSectionIndicies(sectionName);
                            }
                        }
                        auto&& entries = mmh.ParseIndicies(sectionName, true);
                        for (size_t i = 0, sz = entries.size(); i < sz; i++)
                        {
                            FString output;
                            output.Format("%d - %s", i, entries[i]);
                            if (showUIName == "1")
                            {
                                FString uiname = CViewObjectsExt::QueryUIName(entries[i], true);
                                if (uiname != entries[i] && uiname != "")
                                {
                                    FString tmp = output;
                                    output.Format("%s - %s", tmp, uiname);
                                }
                            }
                            SendMessage(hWnd, CB_INSERTSTRING, i, (LPARAM)(LPCSTR)output.c_str());
                        }
                    }
                    else
                    {
                        int i = 0;
                        for (auto& kvp : mmh.GetUnorderedUnionSection(sectionName.c_str()))
                        {
                            FString output;
                            output.Format("%s - %s", kvp.first, kvp.second);
                            if (showUIName == "1")
                            {
                                FString uiname = CViewObjectsExt::QueryUIName(kvp.second, true);
                                if (uiname != kvp.second && uiname != "")
                                {
                                    FString tmp = output;
                                    output.Format("%s - %s", tmp, uiname);
                                }
                            }
                            SendMessage(hWnd, CB_INSERTSTRING, i, (LPARAM)(LPCSTR)output.c_str());
                            i++;
                        }
                    }
                }
            }
        }
        break;
    }
}

void ExtraWindow::LoadParam_Waypoints(HWND& hWnd)
{
    int i = 0;
    if (auto pSection = map.GetSection("Waypoints")) 
    {
        for (auto& kvp : pSection->GetEntities())
        {
            FString output;
            int point = atoi(kvp.second);
            int x = point % 1000;
            int y = point / 1000;

            output.Format("%s - (%d, %d)", kvp.first, x, y);
            SendMessage(hWnd, CB_INSERTSTRING, i, (LPARAM)(LPCSTR)output.c_str());
            i++;
        }
    }

}

void ExtraWindow::LoadParam_ActionList(HWND& hWnd)
{
    FString key;
    FString text;
    for (int i = 0; i < 50; i++)
    {
        key.Format("%d", i);
        auto value = map.GetString(CNewScript::CurrentScriptID, key.c_str());
        auto atoms = FString::SplitString(value, 1);
        key.Format("%d", i + 1);
        if (value != "")
        {
            if (CNewScript::ActionHasExtraParam[atoms[0]])
            {
                int param = atoi(atoms[1]);
                int high = param / 0x10000;
                int low = param % 0x10000;
                text.Format("%s - [%s - (%d, %d)]", key, atoms[0], low, high);
            }
            else
                text.Format("%s - [%s - %s]", key, atoms[0], atoms[1]);

            SendMessage(hWnd, CB_INSERTSTRING, i, (LPARAM)(LPCSTR)text.c_str());
        }
    }
}

void ExtraWindow::LoadParam_CountryList(HWND& hWnd)
{
    MultimapHelper mmh;
    mmh.AddINI(&CINI::Rules);
    mmh.AddINI(&CINI::CurrentDocument);

    int idx = 0;
    int rIdx = 0;
    const auto& indicies = Variables::RulesMap.ParseIndicies("Countries", true);
    for (auto& value : indicies)
    {
        if (value == "Nod" || value == "GDI") {
            rIdx++;
            continue;
        }
        FString output;
            output.Format("%d - %s", rIdx, value);
            FString uiname = CViewObjectsExt::QueryUIName(value, true);
            if (uiname != value && uiname != "")
            {
                FString tmp = output;
                output.Format("%s - %s", tmp, uiname);
            }

        SendMessage(hWnd, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)output.c_str());
        rIdx++;
    }
}

void ExtraWindow::LoadParam_TechnoTypes(HWND& hWnd, int specificType, int style, bool sort)
{
    int idx = 0;

    auto addValueList = [&](const char* secName)
        {
            const auto& indicies = Variables::RulesMap.ParseIndicies(secName, true);
            for (auto& value : indicies)
            {
                FString output;
                output.Format("%s", value);
                FString uiname = CViewObjectsExt::QueryUIName(value, true);
                switch (style)
                {
                case 0:
                {
                    FString tmp = output;
                    output.Format("%s - %s", tmp, uiname);
                }
                break;
                case 1:
                {
                    FString tmp = output;
                    output.Format("%s - %s", tmp, uiname);
                }
                break;
                default:
                    break;
                }

                if (sort)
                    SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)output.c_str());
                else
                    SendMessage(hWnd, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)output.c_str());
            }
        };

    switch (specificType)
    {
    case 0:
        addValueList("BuildingTypes");
        break;
    case 1:
        addValueList("AircraftTypes");
        break;
    case 2:
        addValueList("InfantryTypes");
        break;
    case 3:
        addValueList("VehicleTypes");
        break;
    case 4:
        addValueList("AircraftTypes");
        addValueList("InfantryTypes");
        addValueList("VehicleTypes");
        break;
    default:        
        addValueList("BuildingTypes");
        addValueList("AircraftTypes");
        addValueList("InfantryTypes");
        addValueList("VehicleTypes");
        break;
    }
}

void ExtraWindow::LoadParam_HouseAddon_Multi(HWND& hWnd)
{
    if (CMapData::Instance->IsMultiOnly())
    {
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4475 - <Player @ A>");
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4476 - <Player @ B>");
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4477 - <Player @ C>");
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4478 - <Player @ D>");
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4479 - <Player @ E>");
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4480 - <Player @ F>");
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4481 - <Player @ G>");
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4482 - <Player @ H>");
    }
}

void ExtraWindow::LoadParam_HouseAddon_MultiAres(HWND& hWnd)
{
    if (CMapData::Instance->IsMultiOnly())
    {
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4475 - <Player @ A> (Ares0.A+)");
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4476 - <Player @ B> (Ares0.A+)");
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4477 - <Player @ C> (Ares0.A+)");
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4478 - <Player @ D> (Ares0.A+)");
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4479 - <Player @ E> (Ares0.A+)");
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4480 - <Player @ F> (Ares0.A+)");
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4481 - <Player @ G> (Ares0.A+)");
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4482 - <Player @ H> (Ares0.A+)");
    }
}

void ExtraWindow::LoadParam_Triggers(HWND& hWnd)
{
    ExtraWindow::SyncComboBoxContent(CNewTrigger::hSelectedTrigger, hWnd);
}

void ExtraWindow::LoadParam_Tags(HWND& hWnd)
{
    if (auto pSection = CINI::CurrentDocument().GetSection("Tags"))
    {
        int idx = 0;
        for (auto& kvp : pSection->GetEntities())
        {
            auto tagAtoms = FString::SplitString(kvp.second);
            if (tagAtoms.size() < 3) continue;
            FString text;
            text.Format("%s - %s", kvp.first, tagAtoms[1]);
            SendMessage(hWnd, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)text.c_str());
        }
    }
}

#define MAX_COMBOBOX_STRING_LENGTH 192

void ExtraWindow::LoadParam_Stringtables(HWND& hWnd)
{
    for (auto& x : StringtableLoader::CSFFiles_Stringtable)
    {
        char buffer[MAX_COMBOBOX_STRING_LENGTH + 1];
        _tcsncpy(buffer, x.first + " - " + x.second, MAX_COMBOBOX_STRING_LENGTH);
        buffer[MAX_COMBOBOX_STRING_LENGTH] = _T('\0');
        SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)buffer);
    }

}

bool ExtraWindow::OnCloseupCComboBox(HWND& hWnd, std::map<int, FString>& labels, bool isComboboxSelectOnly)
{
    if (!labels.empty())
    {
        char buffer[512]{ 0 };
        GetWindowText(hWnd, buffer, 511);
        FString text(buffer);
        SendMessage(hWnd, CB_GETLBTEXT, SendMessage(hWnd, CB_GETCURSEL, NULL, NULL), (LPARAM)buffer);
        while (SendMessage(hWnd, CB_DELETESTRING, 0, NULL) != CB_ERR);
        for (auto& pair : labels)
            SendMessage(hWnd, CB_INSERTSTRING, pair.first, (LPARAM)(LPCSTR)pair.second.c_str());
        labels.clear();
        int idx = SendMessage(hWnd, CB_FINDSTRINGEXACT, 0, (LPARAM)buffer);
        if (idx == CB_ERR)
        {
            if (isComboboxSelectOnly)
                SendMessage(hWnd, CB_SETCURSEL, 0, NULL);
            else
                SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM)text.c_str());
        }
        else
            SendMessage(hWnd, CB_SETCURSEL, idx, NULL);

        if (!ExtraWindow::bComboLBoxSelected || isComboboxSelectOnly)
        {
            ExtraWindow::bComboLBoxSelected = false;
            return false;
        }
    }
    ExtraWindow::bComboLBoxSelected = false;
    return true;
}

void ExtraWindow::OnEditCComboBox(HWND& hWnd, std::map<int, FString>& labels)
{
    if ((SendMessage(hWnd, CB_GETCOUNT, NULL, NULL) > ExtConfigs::SearchCombobox_MaxCount 
        || labels.size() > ExtConfigs::SearchCombobox_MaxCount) 
        && !bEnterSearch)
    {
        return;
    }

    ExtraWindow::bComboLBoxSelected = false;

    char buffer[512]{ 0 };
    char buffer2[512]{ 0 };

    if (!labels.empty())
    {
        while (SendMessage(hWnd, CB_DELETESTRING, 0, NULL) != CB_ERR);
        for (auto& pair : labels)
        {
            SendMessage(hWnd, CB_INSERTSTRING, pair.first, (LPARAM)(LPCSTR)pair.second.c_str());
        }
        labels.clear();
    }

    GetWindowText(hWnd, buffer, 511);
    SendMessage(hWnd, CB_SHOWDROPDOWN, TRUE, NULL);

    std::vector<int> deletedLabels;
    for (int idx = SendMessage(hWnd, CB_GETCOUNT, NULL, NULL) - 1; idx >= 0; idx--)
    {
        SendMessage(hWnd, CB_GETLBTEXT, idx, (LPARAM)buffer2);
        bool del = false;
        FString tmp(buffer2);
        if (!(IsLabelMatch(buffer2, buffer) || strcmp(buffer, "")   == 0))
        {
            deletedLabels.push_back(idx);
        }
        labels[idx] = tmp;
    }
    for (int idx : deletedLabels)
    {
        SendMessage(hWnd, CB_DELETESTRING, idx, NULL);
    }
    if (strlen(buffer) == 1)
    {
        SetWindowText(hWnd, (LPCSTR)buffer);
        SendMessage(hWnd, CB_SETEDITSEL, 0, MAKELPARAM(1, 1));
    }
    HCURSOR hCursor = LoadCursor(NULL, IDC_ARROW);
    SetCursor(hCursor);
}

bool ExtraWindow::SortLabels(FString a, FString b)
{
    if (!ExtConfigs::SortByLabelName) {
        return a < b;
    }
    FString::TrimIndexElse(a);
    FString::TrimIndexElse(b);
    a = a.Mid(1, a.GetLength() - 2);
    b = b.Mid(1, b.GetLength() - 2);

    auto sa = std::string(a);
    auto sb = std::string(b);

    std::regex re("(\\D*)(\\d+)");
    std::sregex_iterator itA(sa.begin(), sa.end(), re);
    std::sregex_iterator itB(sb.begin(), sb.end(), re);
    std::sregex_iterator end;
    VEHGuard guard(false);

    while (itA != end && itB != end) {

        std::string prefixA = (*itA)[1].str();
        std::string prefixB = (*itB)[1].str();
        if (prefixA != prefixB) return prefixA < prefixB;

        int numA = INT_MAX;
        int numB = INT_MAX;
        try {
            numA = std::stoi((*itA)[2].str());
        }
        catch (const std::out_of_range& e)
        {
            UNREFERENCED_PARAMETER(e);
        }
        try {
            numB = std::stoi((*itB)[2].str());
        }
        catch (const std::out_of_range& e)
        {
            UNREFERENCED_PARAMETER(e);
        }
        if (numA != numB) return numA < numB;

        if (numA == INT_MAX) {
            std::string suffixA = (*itA)[2].str();
            std::string suffixB = (*itB)[2].str();
            if (suffixA != suffixB) return suffixA < suffixB;
        }
        ++itA;
        ++itB;
    }

    return sa < sb;
}

bool ExtraWindow::SortRawStrings(std::string sa, std::string sb)
{
    std::regex re("(\\D*)(\\d+)");
    std::sregex_iterator itA(sa.begin(), sa.end(), re);
    std::sregex_iterator itB(sb.begin(), sb.end(), re);
    std::sregex_iterator end;
    VEHGuard guard(false);

    while (itA != end && itB != end) {

        std::string prefixA = (*itA)[1].str();
        std::string prefixB = (*itB)[1].str();
        if (prefixA != prefixB) return prefixA < prefixB;

        int numA = INT_MAX;
        int numB = INT_MAX;
        try {
            numA = std::stoi((*itA)[2].str());
        }
        catch (const std::out_of_range& e)
        {
            UNREFERENCED_PARAMETER(e);
        }
        try {
            numB = std::stoi((*itB)[2].str());
        }
        catch (const std::out_of_range& e)
        {
            UNREFERENCED_PARAMETER(e);
        }
        if (numA != numB) return numA < numB;

        if (numA == INT_MAX) {
            std::string suffixA = (*itA)[2].str();
            std::string suffixB = (*itB)[2].str();
            if (suffixA != suffixB) return suffixA < suffixB;
        }
        ++itA;
        ++itB;
    }

    return sa < sb;
}

void ExtraWindow::SortTeams(HWND& hWnd, FString section, int& selectedIndex, FString id)
{
    while (SendMessage(hWnd, CB_DELETESTRING, 0, NULL) != CB_ERR);
    std::vector<FString> labels;
    if (auto pSection = map.GetSection(section)) {
        for (auto& pair : pSection->GetEntities()) {
            labels.push_back(ExtraWindow::GetTeamDisplayName(pair.second));
        }
    }

    bool tmp = ExtConfigs::SortByLabelName;
    
    if (section == "ScriptTypes")
        ExtConfigs::SortByLabelName = ExtConfigs::SortByLabelName_Script;
    else if (section == "TaskForces")
        ExtConfigs::SortByLabelName = ExtConfigs::SortByLabelName_Taskforce;
    else if (section == "TeamTypes")
        ExtConfigs::SortByLabelName = ExtConfigs::SortByLabelName_Team;

    std::sort(labels.begin(), labels.end(), ExtraWindow::SortLabels);

    ExtConfigs::SortByLabelName = tmp;

    for (size_t i = 0; i < labels.size(); ++i) {
        SendMessage(hWnd, CB_INSERTSTRING, i, (LPARAM)(LPCSTR)labels[i].c_str());
    }
    if (id != "") {
        selectedIndex = SendMessage(hWnd, CB_FINDSTRINGEXACT, 0, (LPARAM)ExtraWindow::GetTeamDisplayName(id).c_str());
        SendMessage(hWnd, CB_SETCURSEL, selectedIndex, NULL);
    }
}

void ExtraWindow::SortAITriggers(HWND& hWnd, int& selectedIndex, FString id)
{
    while (SendMessage(hWnd, CB_DELETESTRING, 0, NULL) != CB_ERR);
    std::vector<FString> labels;
    if (auto pSection = map.GetSection("AITriggerTypes")) {
        for (auto& pair : pSection->GetEntities()) {
            labels.push_back(ExtraWindow::GetAITriggerDisplayName(pair.first));
        }
    }

    bool tmp = ExtConfigs::SortByLabelName;
    ExtConfigs::SortByLabelName = ExtConfigs::SortByLabelName_AITrigger;

    std::sort(labels.begin(), labels.end(), ExtraWindow::SortLabels);

    ExtConfigs::SortByLabelName = tmp;

    for (size_t i = 0; i < labels.size(); ++i) {
        SendMessage(hWnd, CB_INSERTSTRING, i, (LPARAM)(LPCSTR)labels[i].c_str());
    }
    if (id != "") {
        selectedIndex = SendMessage(hWnd, CB_FINDSTRINGEXACT, 0, (LPARAM)ExtraWindow::GetAITriggerDisplayName(id).c_str());
        SendMessage(hWnd, CB_SETCURSEL, selectedIndex, NULL);
    }
}

bool ExtraWindow::IsLabelMatch(const char* target, const char* source, bool exactMatch)
{
    std::string simple_target = target;
    std::string simple_source = source;

    if (!exactMatch)
    {
        simple_target = STDHelpers::ToUpperCase(simple_target);
        simple_source = STDHelpers::ToUpperCase(simple_source);
        simple_target = STDHelpers::ChineseTraditional_ToSimple((std::string)simple_target);
        simple_source = STDHelpers::ChineseTraditional_ToSimple((std::string)simple_source);
    }

    ppmfc::CString divide = simple_source.c_str();
    STDHelpers::TrimString(divide);
    auto splits = STDHelpers::SplitString(divide, "|");
    for (auto& split : splits)
    {
        auto atoms = STDHelpers::SplitString(split, "*");
        if (atoms.size() > 1)
        {
            ppmfc::CString tmp = simple_target.c_str();
            char* pCompare = tmp.m_pchData;
            bool match = true;
            for (int i = 0; i < atoms.size(); i++)
            {
                auto& atom = atoms[i];
                if (i == 0 && exactMatch)
                {
                    if (atom == "")
                        continue;
                    else
                    {
                        if (auto pSub = (char*)_mbsstr((unsigned char*)pCompare, (unsigned char*)atom.m_pchData))
                            if (strcmp(pCompare, pSub) != 0)
                            {
                                match = false;
                                break;
                            }
                    }
                }
                else if (i == atoms.size() - 1 && exactMatch)
                {
                    if (atom == "")
                        continue;
                    else
                    {
                        if (strcmp(pCompare, atom) != 0)
                        {
                            match = false;
                            break;
                        }
                    }
                }

                if (atom == "")
                    continue;
                auto pSub = (char*)_mbsstr((unsigned char*)pCompare, (unsigned char*)atom.m_pchData);
                if (pSub != NULL)
                {
                    pCompare = pSub;
                }
                else
                {
                    match = false;
                    break;
                }
            }
            if (match)
                return true;
        }
        else
        {
            if (exactMatch)
            {
                if (strcmp(simple_target.c_str(), split) == 0)
                    return true;
            }
            else
            {
                if ((char*)_mbsstr((unsigned char*)simple_target.c_str(), (unsigned char*)split.m_pchData) != NULL)
                    return true;
            }

        }
    }
    return false;
}

FString ExtraWindow::GetCloneName(FString oriName)
{
    FString newName = oriName;
    if (ExtConfigs::CloneWithOrderedID)
    {
        std::string input(oriName);
        size_t pos = input.size();
        while (pos > 0 && std::isdigit(input[pos - 1])) {
            --pos;
        }

        if (pos == input.size()) {
            return newName + " 02";
        }

        std::string prefix = input.substr(0, pos);
        std::string numberStr = input.substr(pos);

        int number = INT_MAX;
        VEHGuard guard(false);
        try {
            number = std::stoi(numberStr);
        }
        catch (const std::out_of_range& e) {
            UNREFERENCED_PARAMETER(e);
        }
        if (number < INT_MAX) {
            ++number;
        }
        else {
            return newName + " 02";
        }
        
        std::string newNumberStr = std::to_string(number);
        while (newNumberStr.size() < numberStr.size()) {
            newNumberStr = "0" + newNumberStr;
        }

        newName = prefix.c_str();
        newName += newNumberStr.c_str();
    }
    else
        newName = oriName + " Clone";

    return newName;
}

void ExtraWindow::LoadFrom(MultimapHelper& mmh, FString loadfrom)
{
    if (loadfrom == "0" || loadfrom == "fadata")
        mmh.AddINI(&CINI::FAData);
    else if (loadfrom == "1" || loadfrom == "rules")
    {
        mmh = Variables::Rules;
    }
    else if (loadfrom == "2" || loadfrom == "rules+map")
    {
        mmh = Variables::RulesMap;
    }
    else if (loadfrom == "3" || loadfrom == "map")
        mmh.AddINI(&CINI::CurrentDocument);
    else if (loadfrom == "4" || loadfrom == "art")
        mmh.AddINI(&CINI::Art);
    else if (loadfrom == "5" || loadfrom == "sound")
        mmh.AddINI(&CINI::Sound);
    else if (loadfrom == "6" || loadfrom == "theme")
        mmh.AddINI(&CINI::Theme);
    else if (loadfrom == "7" || loadfrom == "ai+map")
    {
        mmh.AddINI(&CINI::Ai);
        mmh.AddINI(&CINI::CurrentDocument);
    }
    else if (loadfrom == "8" || loadfrom == "eva")
        mmh.AddINI(&CINI::Eva);
    else if (loadfrom == "9" || loadfrom == "theater")
        mmh.AddINI(CINI::CurrentTheater);
    else if (loadfrom == "10" || loadfrom == "ai")
        mmh.AddINI(&CINI::Ai);

}