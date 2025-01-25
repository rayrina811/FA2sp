#pragma once

#include "Body.h"

#include <CFinalSunDlg.h>

#include <unordered_set>

#include "../../Helpers/STDHelpers.h"
#include "../../Helpers/ControlHelpers.h"
#include "../../FA2sp.h"
#include "../../Helpers/Translations.h"

class CScriptTypesFunctions
{
public:

// negative
static void CScriptTypes_LoadParams_TypeList(ppmfc::CComboBox& comboBox, int nID)
{
    comboBox.DeleteAllStrings();

    ControlHelpers::ComboBox::LoadSectionList(comboBox,
		CScriptTypeParamCustom::ExtParamsCustom[nID].Section_,
		CScriptTypeParamCustom::ExtParamsCustom[nID].LoadFrom_,
		CScriptTypeParamCustom::ExtParamsCustom[nID].ShowIndex_);
}

// 1
static void CScriptTypes_LoadParams_Target(ppmfc::CComboBox& comboBox)
{
    comboBox.DeleteAllStrings();

    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Target.0", "0 - 任意目标")), 0);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Target.1", "1 - 任意目标(同0)")), 1);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Target.2", "2 - 建筑物")), 2);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Target.3", "3 - 矿车矿场")), 3);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Target.4", "4 - 步兵")), 4);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Target.5", "5 - 车辆")), 5);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Target.6", "6 - 生产建筑")), 6);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Target.7", "7 - 防御建筑")), 7);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Target.8", "8 - 基地威胁")), 8);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Target.9", "9 - 电厂")), 9);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Target.10", "10 - 驻军建筑")), 10);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Target.11", "11 - 科技建筑")), 11);
}

// 2
static void CScriptTypes_LoadParams_Waypoint(ppmfc::CComboBox& comboBox)
{
    comboBox.DeleteAllStrings();

    auto& doc = CINI::CurrentDocument();
    
    if (!ExtConfigs::ExtWaypoints)
    {
        int waypoints[702];
        memset(waypoints, -1, sizeof waypoints);
        if (auto entries = doc.GetSection("Waypoints"))
            for (auto& x : entries->GetEntities())
                if (x.first != "Name" && !STDHelpers::IsNoneOrEmpty(x.second))
                {
                    int l = atoi(x.first);
                    if (l <= 701 && l >= 0)
                        waypoints[l] = atoi(x.second);
                }
        char buffer[0x20];
        for (int i = 0; i < 702; ++i)
        {
            if (waypoints[i] >= 0)
            {
                sprintf_s(buffer, "%u - (%u, %u)", i, waypoints[i] % 1000, waypoints[i] / 1000);
                int idx = comboBox.AddString(buffer);
                comboBox.SetItemData(idx, i);
            }
        }
    }
    else
    {
        std::map<int, int> waypoints;
        if (auto entries = doc.GetSection("Waypoints"))
            for (auto& x : entries->GetEntities())
                if (x.first != "Name" && !STDHelpers::IsNoneOrEmpty(x.second))
                {
                    int l = atoi(x.first);
                    if (l >= 0)
                        waypoints[l] = atoi(x.second);
                }
        ppmfc::CString buffer;
        for (auto& pair : waypoints)
        {
            if (pair.second >= 0)
            {
                buffer.Format("%u - (%u, %u)", pair.first, pair.second % 1000, pair.second / 1000);
                comboBox.SetItemData(comboBox.AddString(buffer), pair.first);
            }
        }
    }
    
}

// 3
static void CScriptTypes_LoadParams_ScriptLine(ppmfc::CComboBox& comboBox, ppmfc::CComboBox& currentScript, ppmfc::CListBox& listBox)
{
    int cnt = listBox.GetCount();
    // up to 50
    if (cnt > 50)
        cnt = 50;

    comboBox.DeleteAllStrings();

    auto& doc = CINI::CurrentDocument();

    ppmfc::CString buffer, scriptName, parambuf;
    currentScript.GetLBText(currentScript.GetCurSel(), scriptName);
    STDHelpers::TrimIndex(scriptName);

    for (int i = 0; i < cnt; ++i)
    {
        buffer.Format("%d", i);
        buffer = doc.GetString(scriptName, buffer, "0,0");
        int actionIndex = buffer.Find(',');
        if (actionIndex == CB_ERR)
            actionIndex = -1;
        else
            actionIndex = atoi(buffer.Mid(0, actionIndex));
        buffer.Format("%d - %s", i + 1, CScriptTypeAction::ExtActions[actionIndex].Name_);
        int idx = comboBox.AddString(buffer);
        comboBox.SetItemData(idx, i);
    }
}

// 4
static void CScriptTypes_LoadParams_SplitGroup(ppmfc::CComboBox& comboBox)
{
    comboBox.DeleteAllStrings();
    
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.SplitGroup.0", "0 - 保留载具，保留成员")), 0);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.SplitGroup.1", "1 - 保留载具，丢弃成员")), 1);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.SplitGroup.2", "2 - 丢弃载具，保留成员")), 2);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.SplitGroup.3", "3 - 丢弃载具，丢弃成员")), 3);
}

// 5
static void CScriptTypes_LoadParams_GlobalVariables(ppmfc::CComboBox& comboBox)
{
    comboBox.DeleteAllStrings();

    MultimapHelper mmh;
    mmh.AddINI(&CINI::Rules());

    auto&& entries = mmh.ParseIndicies("VariableNames", true);
    if (entries.size() > 0)
    {
        CString buffer;
        for (size_t i = 0, sz = entries.size(); i < sz; ++i)
        {
            buffer.Format("%u - %s", i, entries[i]);
            comboBox.SetItemData(comboBox.AddString(buffer), i);
        }
    }
}

// 6
static void CScriptTypes_LoadParams_ScriptTypes(ppmfc::CComboBox& comboBox)
{
    comboBox.DeleteAllStrings();

    MultimapHelper mmh;
    mmh.AddINI(&CINI::Ai());
    mmh.AddINI(&CINI::CurrentDocument());

    auto&& entries = mmh.ParseIndicies("ScriptTypes", true);
    if (entries.size() > 0)
    {
        CString text, finaltext = "";
        for (size_t i = 0, sz = entries.size(); i < sz; ++i)
        {
            text = mmh.GetString(entries[i], "Name");
            finaltext.Format("%d - %s - %s", i, entries[i], text);
            comboBox.SetItemData(comboBox.AddString(finaltext), i);
        }
    }

}

// 7
static void CScriptTypes_LoadParams_TeamTypes(ppmfc::CComboBox& comboBox)
{
    comboBox.DeleteAllStrings();
    
    MultimapHelper mmh;
    mmh.AddINI(&CINI::Ai());
    mmh.AddINI(&CINI::CurrentDocument());

    auto&& entries = mmh.ParseIndicies("TeamTypes", true);
    if (entries.size() > 0)
    {
        CString text, finaltext = "";
        for (size_t i = 0, sz = entries.size(); i < sz; ++i)
        {
            text = mmh.GetString(entries[i], "Name");
            finaltext.Format("%d - %s - %s", i, entries[i], text);
            comboBox.SetItemData(comboBox.AddString(finaltext), i);
        }
    }

}

// 8
static void CScriptTypes_LoadParams_Houses(ppmfc::CComboBox& comboBox)
{
    ControlHelpers::ComboBox::LoadHouses(comboBox, true);
}

// 9
static void CScriptTypes_LoadParams_Speechs(ppmfc::CComboBox& comboBox)
{
    comboBox.DeleteAllStrings();

    auto& eva = CINI::Eva();
    MultimapHelper mmh;
    mmh.AddINI(&CINI::Eva());

    auto&& entries = mmh.ParseIndicies("DialogList", true);
    if (entries.size() > 0)
    {
        CString buffer;
        for (size_t i = 0, sz = entries.size(); i < sz; ++i)
        {
            buffer.Format("%d - %s - %s", i, entries[i], eva.GetString(entries[i], "Text"));
            comboBox.SetItemData(comboBox.AddString(buffer), i);
        }
    }



}

// 10
static void CScriptTypes_LoadParams_Sounds(ppmfc::CComboBox& comboBox)
{
    comboBox.DeleteAllStrings();

    MultimapHelper mmh;
    mmh.AddINI(&CINI::Sound());

    auto&& entries = mmh.ParseIndicies("SoundList", true);
    if (entries.size() > 0)
    {
        CString buffer;
        for (size_t i = 0, sz = entries.size(); i < sz; ++i)
        {
            buffer.Format("%u - %s", i, entries[i]);
            comboBox.SetItemData(comboBox.AddString(buffer), i);
        }
    }

}

// 11
static void CScriptTypes_LoadParams_Movies(ppmfc::CComboBox& comboBox)
{
    comboBox.DeleteAllStrings();

    MultimapHelper mmh;
    mmh.AddINI(&CINI::Art());
    auto&& entries = mmh.ParseIndicies("Movies", true);
    if (entries.size() > 0)
    {
        CString buffer;
        for (size_t i = 0, sz = entries.size(); i < sz; ++i)
        {

            buffer.Format("%d - %s", i, entries[i]);
            comboBox.SetItemData(comboBox.AddString(buffer), i);
        }
    }
}

// 12
static void CScriptTypes_LoadParams_Themes(ppmfc::CComboBox& comboBox)
{
    comboBox.DeleteAllStrings();

    MultimapHelper mmh;
    mmh.AddINI(&CINI::Theme());
    auto&& entries = mmh.ParseIndicies("Themes", true);
    if (entries.size() > 0)
    {
        CString buffer;
        for (size_t i = 0, sz = entries.size(); i < sz; ++i)
        {

            buffer.Format("%d - %s", i, entries[i]);
            comboBox.SetItemData(comboBox.AddString(buffer), i);
        }
    }
}

// 13
static void CScriptTypes_LoadParams_Countries(ppmfc::CComboBox& comboBox)
{
    ControlHelpers::ComboBox::LoadCountries(comboBox, true);
}

// 14
static void CScriptTypes_LoadParams_LocalVariables(ppmfc::CComboBox& comboBox)
{
    comboBox.DeleteAllStrings();

    auto& doc = CINI::CurrentDocument();
    if (auto entities = doc.GetSection("VariableNames"))
    {
        CString text;
        for (auto& x : entities->GetEntities())
        {
            if (STDHelpers::IsNoneOrEmpty(x.first) || x.first == "Name")
                continue;
            int l = atoi(x.first);
            text.Format("%d - %s", l, x.second);
            comboBox.SetItemData(comboBox.AddString(text), l);
        }
    }
}

// 15
static void CScriptTypes_LoadParams_Facing(ppmfc::CComboBox& comboBox)
{
    comboBox.DeleteAllStrings();

    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Facing.0", "0 - ↗")), 0);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Facing.1", "1 - →")), 1);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Facing.2", "2 - ↘")), 2);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Facing.3", "3 - ↓")), 3);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Facing.4", "4 - ↙")), 4);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Facing.5", "5 - ←")), 5);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Facing.6", "6 - ↖")), 6);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Facing.7", "7 - ↑")), 7);
}

// 16
static void CScriptTypes_LoadParams_BuildingTypes(ppmfc::CComboBox& comboBox)
{
    ControlHelpers::ComboBox::LoadGenericList(comboBox, "BuildingTypes", true, true);
}

// 17
static void CScriptTypes_LoadParams_Animations(ppmfc::CComboBox& comboBox)
{
    ControlHelpers::ComboBox::LoadGenericList(comboBox, "Animations", false, true, true);
}

// 18
static void CScriptTypes_LoadParams_TalkBubble(ppmfc::CComboBox& comboBox)
{
    comboBox.DeleteAllStrings();

    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.TalkBubble.0", "0 - 无")), 0);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.TalkBubble.1", "1 - 星号(*)")), 1);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.TalkBubble.2", "2 - 问号(?)")), 2);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.TalkBubble.3", "3 - 感叹号(!)")), 3);
}

// 19
static void CScriptTypes_LoadParams_Status(ppmfc::CComboBox& comboBox)
{
    comboBox.DeleteAllStrings();

    int i = 0;
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.0", "0 - ※Sleep (休眠，不还击)")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.1", "1 - Attack nearest enemy")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.2", "2 - Move")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.3", "3 - QMove")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.4", "4 - Retreat (撤离地图)")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.5", "5 - ※Guard (原地警戒)")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.6", "6 - Sticky (固守，不主动攻击)")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.7", "7 - Enter object")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.8", "8 - Capture object")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.9", "9 - Move into & get eaten")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.10", "10 - Harvest (采矿)")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.11", "11 - ※Area Guard (区域警戒)")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.12", "12 - Return (to refinery)")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.13", "13 - Stop")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.14", "14 - Ambush (wait until discovered)")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.15", "15 - ※Hunt (游猎)")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.16", "16 - ※Unload (卸载或部署)")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.17", "17 - Sabotage (move in & destroy)")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.18", "18 - Construction")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.19", "19 - Deconstruction")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.20", "20 - Repair")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.21", "21 - Rescue")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.22", "22 - Missile")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.23", "23 - ※Harmless (无威胁)")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.24", "24 - Open")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.25", "25 - Patrol")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.26", "26 - Paradrop approach drop zone")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.27", "27 - Paradrop overlay drop zone")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.28", "28 - Wait")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.29", "29 - Attack again")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.30", "30 - Spyplane approach (YR)")), i++);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Status.31", "31 - Spyplane overfly (YR)")), i++);
}

// 20
static void CScriptTypes_LoadParams_Boolean(ppmfc::CComboBox& comboBox)
{
    comboBox.DeleteAllStrings();

    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Boolean.0", "0 - 否")), 0);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.Boolean.1", "1 - 是")), 1);
}

// 21
static void CScriptTypes_LoadParams_CameraSpeed(ppmfc::CComboBox& comboBox)
{
	if (ControlHelpers::ComboBox::LoadSectionList(comboBox, "CameraSpeed", 3, false))
		return;

    comboBox.DeleteAllStrings();

    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.CameraSpeed.0", "0 - 非常慢")), 0);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.CameraSpeed.1", "1 - 慢")), 1);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.CameraSpeed.2", "2 - 正常")), 2);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.CameraSpeed.3", "3 - 快")), 3);
    comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptParam.CameraSpeed.4", "4 - 非常快")), 4);
}

};