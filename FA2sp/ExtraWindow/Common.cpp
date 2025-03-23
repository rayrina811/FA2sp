#include "Common.h"
#include "CNewTrigger/CNewTrigger.h"
#include "../Helpers/STDHelpers.h"
#include "CNewScript/CNewScript.h"
#include "../Helpers/Translations.h"
#include "CObjectSearch/CObjectSearch.h"
#include <regex>
#include <string>
#include <vector>
#include <algorithm>
#include <CFinalSunApp.h>

CINI& ExtraWindow::map = CINI::CurrentDocument;
CINI& ExtraWindow::fadata = CINI::FAData;
MultimapHelper& ExtraWindow::rules = Variables::Rules;

bool ExtraWindow::bComboLBoxSelected = false;
bool ExtraWindow::bEnterSearch = false;

void ExtraWindow::CenterWindowPos(HWND parent, HWND target)
{
    RECT parentRect;
    GetClientRect(parent, &parentRect);

    RECT dlgRect;
    GetWindowRect(target, &dlgRect);

    int dlgWidth = dlgRect.right - dlgRect.left;
    int dlgHeight = dlgRect.bottom - dlgRect.top;
    int parentWidth = parentRect.right - parentRect.left;
    int parentHeight = parentRect.bottom - parentRect.top;

    int xPos = parentRect.left + (parentWidth - dlgWidth) / 2;
    int yPos = parentRect.top + (parentHeight - dlgHeight) / 2;

    SetWindowPos(target, HWND_TOP, xPos, yPos, 0, 0, SWP_NOSIZE);
}
ppmfc::CString ExtraWindow::GetTeamDisplayName(const char* id)
{
    ppmfc::CString name;
    name.Format("%s (%s)", id, map.GetString(id, "Name"));
    return name;
}
ppmfc::CString ExtraWindow::GetAITriggerDisplayName(const char* id)
{
    ppmfc::CString name = "";
    auto atoms = STDHelpers::SplitString(map.GetString("AITriggerTypes", id));
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

ppmfc::CString ExtraWindow::FormatTriggerDisplayName(const char* id, const char* name)
{
    ppmfc::CString ret;
    ret.Format("%s (%s)", id, name);
    return ret;
}

ppmfc::CString ExtraWindow::GetTriggerDisplayName(const char* id)
{
    ppmfc::CString name;
    if (strcmp(id, "<none>") == 0)
        return id;
    auto atoms = STDHelpers::SplitString(map.GetString("Triggers", id, "Americans,<none>,MISSING,0,1,1,1,0"));
    name.Format("%s (%s)", id, atoms[2]);
    return name;
}

ppmfc::CString ExtraWindow::GetTriggerName(const char* id)
{
    ppmfc::CString name;
    if (strcmp(id, "<none>") == 0)
        return id;
    auto atoms = STDHelpers::SplitString(map.GetString("Triggers", id, "Americans,<none>,MISSING,0,1,1,1,0"));
    return atoms[2];
}

ppmfc::CString ExtraWindow::GetAITriggerName(const char* id)
{
    ppmfc::CString name;
    if (strcmp(id, "<none>") == 0)
        return id;
    auto atoms = STDHelpers::SplitString(map.GetString("AITriggerTypes", id, "MISSING"));
    return atoms[0];
}

ppmfc::CString ExtraWindow::GetTagName(const char* id)
{
    ppmfc::CString name;
    if (strcmp(id, "<none>") == 0)
        return id;
    auto atoms = STDHelpers::SplitString(map.GetString("Tags", id, "0,MISSING,01000000"));
    return atoms[1];
}

ppmfc::CString ExtraWindow::GetEventDisplayName(const char* id, int index)
{
    ppmfc::CString name;
    ppmfc::CString name2;
    ppmfc::CString atom = STDHelpers::SplitString(fadata.GetString("EventsRA2", id, "MISSING"))[0];
    name.Format("%s %s", id, atom);
    if (index >= 0)
        name2.Format("[%d] %s", index, name);
    else name2 = name;
    return name2;
}

ppmfc::CString ExtraWindow::GetActionDisplayName(const char* id, int index)
{
    ppmfc::CString name;
    ppmfc::CString name2;
    ppmfc::CString atom = STDHelpers::SplitString(fadata.GetString("ActionsRA2", id, "MISSING"))[0];
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

void ExtraWindow::LoadParams(HWND& hWnd, ppmfc::CString idx)
{
    ppmfc::CString addonN1 = "-1 - ";
    ppmfc::CString addonN2 = "-2 - ";
    ppmfc::CString addonN3 = "-3 - ";

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

        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)(addonN1 + Translations::TranslateOrDefault("NonNeutralrandomhouse", "Non-Neutral random house")).m_pchData);
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)(addonN2 + Translations::TranslateOrDefault("FirstNeutralhouse", "First Neutral house")).m_pchData);
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)(addonN3 + Translations::TranslateOrDefault("RandomHumanplayer", "Random Human player")).m_pchData);
        break;
    case 4:
        LoadParam_CountryList(hWnd);
        LoadParam_HouseAddon_Multi(hWnd);
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)(addonN1 + Translations::TranslateOrDefault("Anyhouse", "Any house")).m_pchData);
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)(addonN2 + Translations::TranslateOrDefault("Triggerhouse", "Trigger house")).m_pchData);
        break;
    case 5:
        LoadParam_CountryList(hWnd);
        LoadParam_HouseAddon_Multi(hWnd);
        break;
    case 6:
        LoadParam_CountryList(hWnd);
        LoadParam_HouseAddon_MultiAres(hWnd);
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)(addonN1 + Translations::TranslateOrDefault("Anyhouse", "Any house")).m_pchData);
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
        break;;
    case 11:
        LoadParam_Tags(hWnd);
        break;
    case 12: // float
        CNewTrigger::ActionParamUsesFloat = true;
        break;
    default:
        if (atoi(idx) >= 500)
        {
            if (auto pSectionNewParamTypes = fadata.GetSection("NewParamTypes"))
            {
                auto atoms3 = STDHelpers::SplitString(fadata.GetString("NewParamTypes", idx), 4);
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
                        ppmfc::CString output;
                        output.Format("%s", kvp.second);
                        if (showUIName == "1")
                        {
                            ppmfc::CString uiname = CViewObjectsExt::QueryUIName(kvp.second, true);
                            if (uiname != kvp.second && uiname != "")
                                output.Format("%s - %s", output, uiname);
                        }
                        SendMessage(hWnd, CB_INSERTSTRING, i, (LPARAM)(LPCSTR)output.m_pchData);
                        i++;
                    }
                }
                else
                {
                    if (strictOrder == "1")
                    {
                        // rules
                        if (loadFrom == "1" || loadFrom == "2") {
                            if (const auto& indicies = loadFrom == "1" ? Variables::GetRulesSection(sectionName) : Variables::GetRulesMapSection(sectionName))
                            {
                                int idx = 0;
                                for (auto& pair : *indicies)
                                {
                                    ppmfc::CString output;
                                    output.Format("%d - %s", idx, pair.second);
                                    if (showUIName == "1")
                                    {
                                        ppmfc::CString uiname = CViewObjectsExt::QueryUIName(pair.second, true);
                                        if (uiname != pair.second && uiname != "")
                                            output.Format("%s - %s", output, uiname);
                                    }
                                    SendMessage(hWnd, CB_INSERTSTRING, idx, (LPARAM)(LPCSTR)output.m_pchData);
                                    idx++;
                                }
                            }
                        }
                        else {
                            auto&& entries = mmh.ParseIndicies(sectionName, true);
                            for (size_t i = 0, sz = entries.size(); i < sz; i++)
                            {
                                ppmfc::CString output;
                                output.Format("%d - %s", i, entries[i]);
                                if (showUIName == "1")
                                {
                                    ppmfc::CString uiname = CViewObjectsExt::QueryUIName(entries[i], true);
                                    if (uiname != entries[i] && uiname != "")
                                        output.Format("%s - %s", output, uiname);
                                }
                                SendMessage(hWnd, CB_INSERTSTRING, i, (LPARAM)(LPCSTR)output.m_pchData);
                            }
                        }
                    }
                    else
                    {
                        int i = 0;
                        for (auto& kvp : mmh.GetUnorderedUnionSection(sectionName))
                        {
                            ppmfc::CString output;
                            output.Format("%s - %s", kvp.first, kvp.second);
                            if (showUIName == "1")
                            {
                                ppmfc::CString uiname = CViewObjectsExt::QueryUIName(kvp.second, true);
                                if (uiname != kvp.second && uiname != "")
                                    output.Format("%s - %s", output, uiname);
                            }
                            SendMessage(hWnd, CB_INSERTSTRING, i, (LPARAM)(LPCSTR)output.m_pchData);
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
            ppmfc::CString output;
            int point = atoi(kvp.second);
            int x = point % 1000;
            int y = point / 1000;

            output.Format("%s - (%d, %d)", kvp.first, x, y);
            SendMessage(hWnd, CB_INSERTSTRING, i, (LPARAM)(LPCSTR)output.m_pchData);
            i++;
        }
    }

}

void ExtraWindow::LoadParam_ActionList(HWND& hWnd)
{
    ppmfc::CString key;
    ppmfc::CString text;
    for (int i = 0; i < 50; i++)
    {
        key.Format("%d", i);
        auto value = map.GetString(CNewScript::CurrentScriptID, key);
        auto atoms = STDHelpers::SplitString(value, 1);
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

            SendMessage(hWnd, CB_INSERTSTRING, i, (LPARAM)(LPCSTR)text.m_pchData);
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
    if (const auto& indicies = Variables::GetRulesMapSection("Countries"))
    {
        for (auto& pair : *indicies)
        {
            if (pair.second == "Nod" || pair.second == "GDI") {
                rIdx++;
                continue;
            }
            ppmfc::CString output;
                output.Format("%d - %s", rIdx, pair.second);
                ppmfc::CString uiname = CViewObjectsExt::QueryUIName(pair.second, true);
                if (uiname != pair.second && uiname != "")
                    output.Format("%s - %s", output, uiname);

            SendMessage(hWnd, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)output.m_pchData);
            rIdx++;
        }
    }

}

void ExtraWindow::LoadParam_TechnoTypes(HWND& hWnd, int specificType, int style, bool sort)
{
    int idx = 0;

    auto addValueList = [&](const char* secName)
        {
            if (const auto& indicies = Variables::GetRulesMapSection(secName))
            {
                for (auto& pair : *indicies)
                {
                    ppmfc::CString output;
                    output.Format("%s", pair.second);
                    ppmfc::CString uiname = CViewObjectsExt::QueryUIName(pair.second, true);
                    switch (style)
                    {
                    case 0:
                        output.Format("%s - %s", output, uiname);
                        break;
                    case 1:
                        output.Format("%s (%s)", output, uiname);
                        break;
                    default:
                        break;
                    }

                    if (sort)
                        SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)output.m_pchData);
                    else
                        SendMessage(hWnd, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)output.m_pchData);
                }
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
            auto tagAtoms = STDHelpers::SplitString(kvp.second);
            if (tagAtoms.size() < 3) continue;
            ppmfc::CString text;
            text.Format("%s - %s", kvp.first, tagAtoms[1]);
            SendMessage(hWnd, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)text.m_pchData);
        }
    }
}

#define MAX_COMBOBOX_STRING_LENGTH 192

void ExtraWindow::LoadParam_Stringtables(HWND& hWnd)
{
    int idx = 0;
    for (auto& x : FA2sp::TutorialTextsMap)
    {
        char buffer[MAX_COMBOBOX_STRING_LENGTH + 1];
        _tcsncpy(buffer, x.first + " - " + x.second, MAX_COMBOBOX_STRING_LENGTH);
        buffer[MAX_COMBOBOX_STRING_LENGTH] = _T('\0');
        SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)buffer);
    }

}

bool ExtraWindow::OnCloseupCComboBox(HWND& hWnd, std::map<int, ppmfc::CString>& labels, bool isComboboxSelectOnly)
{
    if (!labels.empty())
    {
        char buffer[512]{ 0 };
        GetWindowText(hWnd, buffer, 511);
        ppmfc::CString text(buffer);
        SendMessage(hWnd, CB_GETLBTEXT, SendMessage(hWnd, CB_GETCURSEL, NULL, NULL), (LPARAM)buffer);
        while (SendMessage(hWnd, CB_DELETESTRING, 0, NULL) != CB_ERR);
        for (auto& pair : labels)
            SendMessage(hWnd, CB_INSERTSTRING, pair.first, (LPARAM)(LPCSTR)pair.second.m_pchData);
        labels.clear();
        int idx = SendMessage(hWnd, CB_FINDSTRINGEXACT, 0, (LPARAM)buffer);
        if (idx == CB_ERR)
        {
            if (isComboboxSelectOnly)
                SendMessage(hWnd, CB_SETCURSEL, 0, NULL);
            else
                SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM)text.m_pchData);
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

void ExtraWindow::OnEditCComboBox(HWND& hWnd, std::map<int, ppmfc::CString>& labels)
{
    if ((SendMessage(hWnd, CB_GETCOUNT, NULL, NULL) > ExtConfigs::SearchCombobox_MaxCount 
        || labels.size() > ExtConfigs::SearchCombobox_MaxCount) 
        && !bEnterSearch)
    {
        return;
    }

    char buffer[512]{ 0 };
    char buffer2[512]{ 0 };

    if (!labels.empty())
    {
        while (SendMessage(hWnd, CB_DELETESTRING, 0, NULL) != CB_ERR);
        for (auto& pair : labels)
        {
            SendMessage(hWnd, CB_INSERTSTRING, pair.first, (LPARAM)(LPCSTR)pair.second.m_pchData);
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
        ppmfc::CString tmp(buffer2);
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

bool ExtraWindow::SortLabels(ppmfc::CString a, ppmfc::CString b)
{
    if (!ExtConfigs::SortByLabelName) {
        return a < b;
    }
    STDHelpers::TrimIndexElse(a);
    STDHelpers::TrimIndexElse(b);
    a = a.Mid(1, a.GetLength() - 2);
    b = b.Mid(1, b.GetLength() - 2);

    auto sa = std::string(a);
    auto sb = std::string(b);

    std::regex re("(\\D*)(\\d+)");
    std::sregex_iterator itA(sa.begin(), sa.end(), re);
    std::sregex_iterator itB(sb.begin(), sb.end(), re);
    std::sregex_iterator end;

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
        }
        try {
            numB = std::stoi((*itB)[2].str());
        }
        catch (const std::out_of_range& e)
        {
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
        }
        try {
            numB = std::stoi((*itB)[2].str());
        }
        catch (const std::out_of_range& e)
        {
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

void ExtraWindow::SortTeams(HWND& hWnd, ppmfc::CString section, int& selectedIndex, ppmfc::CString id)
{
    while (SendMessage(hWnd, CB_DELETESTRING, 0, NULL) != CB_ERR);
    std::vector<ppmfc::CString> labels;
    if (auto pSection = map.GetSection(section)) {
        for (auto& pair : pSection->GetEntities()) {
            labels.push_back(ExtraWindow::GetTeamDisplayName(pair.second));
        }
    }

    std::sort(labels.begin(), labels.end(), ExtraWindow::SortLabels);

    for (size_t i = 0; i < labels.size(); ++i) {
        SendMessage(hWnd, CB_INSERTSTRING, i, (LPARAM)(LPCSTR)labels[i].m_pchData);
    }
    if (id != "") {
        selectedIndex = SendMessage(hWnd, CB_FINDSTRINGEXACT, 0, (LPARAM)ExtraWindow::GetTeamDisplayName(id).m_pchData);
        SendMessage(hWnd, CB_SETCURSEL, selectedIndex, NULL);
    }
}

void ExtraWindow::SortAITriggers(HWND& hWnd, int& selectedIndex, ppmfc::CString id)
{
    while (SendMessage(hWnd, CB_DELETESTRING, 0, NULL) != CB_ERR);
    std::vector<ppmfc::CString> labels;
    if (auto pSection = map.GetSection("AITriggerTypes")) {
        for (auto& pair : pSection->GetEntities()) {
            labels.push_back(ExtraWindow::GetAITriggerDisplayName(pair.first));
        }
    }

    std::sort(labels.begin(), labels.end(), ExtraWindow::SortLabels);

    for (size_t i = 0; i < labels.size(); ++i) {
        SendMessage(hWnd, CB_INSERTSTRING, i, (LPARAM)(LPCSTR)labels[i].m_pchData);
    }
    if (id != "") {
        selectedIndex = SendMessage(hWnd, CB_FINDSTRINGEXACT, 0, (LPARAM)ExtraWindow::GetAITriggerDisplayName(id).m_pchData);
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

ppmfc::CString ExtraWindow::GetCloneName(ppmfc::CString oriName)
{
    ppmfc::CString newName = oriName;
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
        try {
            number = std::stoi(numberStr);
        }
        catch (const std::out_of_range& e) {

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
void ExtraWindow::LoadFrom(MultimapHelper& mmh, ppmfc::CString loadfrom)
{
    if (loadfrom == "0" || loadfrom == "fadata")
        mmh.AddINI(&CINI::FAData);
    else if (loadfrom == "1" || loadfrom == "rules")
        mmh.AddINI(&CINI::Rules);
    else if (loadfrom == "2" || loadfrom == "rules+map")
    {
        mmh.AddINI(&CINI::Rules);
        mmh.AddINI(&CINI::CurrentDocument);
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