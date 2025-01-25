#include <Helpers/Macro.h>
#include <CMapData.h>

#include <map>

#include "../FA2sp.h"
#include "../Helpers/STDHelpers.h"
#include "../Helpers/ControlHelpers.h"

DEFINE_HOOK(43CE50, Miscs_LoadParamToCombobox, 7)
{
    GET_STACK(ppmfc::CComboBox*, pComboBox, 0x4);
    GET_STACK(int, nCode, 0x8);

    if (nCode <= 30) // 30 is our float
        return 0;

    if (nCode >= 500) // Custom params from FAData TriggerParamTypes section
	{
        ControlHelpers::ComboBox::LoadTriggerParamType(*pComboBox, nCode);
	    return 0x43D058;
	}
    auto const pINI = CMapData::GetMapDocument(true);
	
    // Consistence with FA2Ext
    switch (nCode)
    {
    case 31: // Enter Status
        pComboBox->DeleteAllStrings();
        pComboBox->AddString("0 - ※Sleep (休眠，不还击)");
        pComboBox->AddString("1 - Attack nearest enemy");
        pComboBox->AddString("2 - Move");
        pComboBox->AddString("3 - QMove");
        pComboBox->AddString("4 - Retreat (撤离地图)");
        pComboBox->AddString("5 - ※Guard (原地警戒)");
        pComboBox->AddString("6 - Sticky (固守，不主动攻击)");
        pComboBox->AddString("7 - Enter object");
        pComboBox->AddString("8 - Capture object");
        pComboBox->AddString("9 - Move into & get eaten");
        pComboBox->AddString("10 - Harvest (采矿)");
        pComboBox->AddString("11 - ※Area Guard (区域警戒)");
        pComboBox->AddString("12 - Return (to refinery)");
        pComboBox->AddString("13 - Stop");
        pComboBox->AddString("14 - Ambush (wait until discovered)");
        pComboBox->AddString("15 - ※Hunt (游猎)");
        pComboBox->AddString("16 - ※Unload (卸载或部署)");
        pComboBox->AddString("17 - Sabotage (move in & destroy)");
        pComboBox->AddString("18 - Construction");
        pComboBox->AddString("19 - Deconstruction");
        pComboBox->AddString("20 - Repair");
        pComboBox->AddString("21 - Rescue");
        pComboBox->AddString("22 - Missile");
        pComboBox->AddString("23 - ※Harmless (无威胁)");
        pComboBox->AddString("24 - Open");
        pComboBox->AddString("25 - Patrol");
        pComboBox->AddString("26 - Paradrop approach drop zone");
        pComboBox->AddString("27 - Paradrop overlay drop zone");
        pComboBox->AddString("28 - Wait");
        pComboBox->AddString("29 - Attack again");
        pComboBox->AddString("30 - Spyplane approach (YR)");
        pComboBox->AddString("31 - Spyplane overfly (YR)");
        break;
    case 32: // Targets
        pComboBox->DeleteAllStrings();
        pComboBox->AddString("0 - 任意目标");
        pComboBox->AddString("1 - 任意目标");
        pComboBox->AddString("2 - 建筑物");
        pComboBox->AddString("3 - 矿车矿场");
        pComboBox->AddString("4 - 步兵");
        pComboBox->AddString("5 - 车辆");
        pComboBox->AddString("6 - 生产建筑");
        pComboBox->AddString("7 - 防御建筑");
        pComboBox->AddString("8 - 基地威胁");
        pComboBox->AddString("9 - 电厂");
        pComboBox->AddString("10 - 驻军建筑");
        pComboBox->AddString("11 - 科技建筑");
        break;
    case 33: // Facing
        pComboBox->DeleteAllStrings();
        pComboBox->AddString("0 - ↗");
        pComboBox->AddString("1 - →");
        pComboBox->AddString("2 - ↘");
        pComboBox->AddString("3 - ↓");
        pComboBox->AddString("4 - ↙");
        pComboBox->AddString("5 - ←");
        pComboBox->AddString("6 - ↖");
        pComboBox->AddString("7 - ↑");
        break;
    case 34: // Split
        pComboBox->DeleteAllStrings();
        pComboBox->AddString("0 - 保留载具，保留成员");
        pComboBox->AddString("1 - 保留载具，丢弃成员");
        pComboBox->AddString("2 - 丢弃载具，保留成员");
        pComboBox->AddString("3 - 丢弃载具，丢弃成员");
        break;
    case 35: // Camera Move Speed
        pComboBox->DeleteAllStrings();
        pComboBox->AddString("0 - 非常慢");
        pComboBox->AddString("1 - 慢");
        pComboBox->AddString("2 - 正常");
        pComboBox->AddString("3 - 快");
        pComboBox->AddString("4 - 非常快");
        break;
    case 37: // Radar Event Type
        pComboBox->DeleteAllStrings();
        pComboBox->AddString("0 - 战斗");
        pComboBox->AddString("1 - 非战斗");
        pComboBox->AddString("2 - 空降区");
        pComboBox->AddString("3 - 基地受击");
        pComboBox->AddString("4 - 矿车受击");
        pComboBox->AddString("5 - 发现敌军");
        pComboBox->AddString("6 - 单位生产");
        pComboBox->AddString("7 - 单位阵亡");
        pComboBox->AddString("8 - 单位维修");
        pComboBox->AddString("9 - 建筑被渗透");
        pComboBox->AddString("10 - 建筑被占领");
        pComboBox->AddString("11 - 信标放置");
        pComboBox->AddString("12 - 发现超武");
        pComboBox->AddString("13 - 超武启动");
        pComboBox->AddString("14 - 桥梁维修");
        pComboBox->AddString("15 - 放弃驻扎");
        pComboBox->AddString("16 - 友军受击");
        break;
    case 38: // Tabpage
        pComboBox->DeleteAllStrings();
        pComboBox->AddString("0 - 建筑物");
        pComboBox->AddString("1 - 防御建筑");
        pComboBox->AddString("2 - 步兵");
        pComboBox->AddString("3 - 单位");
        break;
    case 39: // SuperWeaponTypes (ID)
        ControlHelpers::ComboBox::LoadGenericList(*pComboBox, "SuperWeaponTypes", true, false, true);
        break;
    case 40: // Variable Operators
        pComboBox->AddString("0 - 赋值 (=)     A=B");
        pComboBox->AddString("1 - 加 (+)         A=A+B");
        pComboBox->AddString("2 - 减 (-)         A=A-B");
        pComboBox->AddString("3 - 乘 (*)         A=A*B");
        pComboBox->AddString("4 - 除 (/)         A=A/B");
        pComboBox->AddString("5 - 求余 (%)    A=A%B");
        pComboBox->AddString("6 - 左移 (<<)   A=A<<B");
        pComboBox->AddString("7 - 右移 (>>)   A=A>>B");
        pComboBox->AddString("8 - 反转 (~)     A=~A");
        pComboBox->AddString("9 - 异或           A=A⊕B");
        pComboBox->AddString("10 - 或            A=A|B");
        pComboBox->AddString("11 - 与            A=A&B");
		break;
    case 41: // House Addons
        ControlHelpers::ComboBox::LoadCountries(*pComboBox, true);
        ControlHelpers::ComboBox::LoadHousesAddonMulti(*pComboBox, true);
        ControlHelpers::ComboBox::LoadHousesAddon(*pComboBox, true);
        break;
    case 42: // House Addons for pb events
        ControlHelpers::ComboBox::LoadCountries(*pComboBox, true);
        ControlHelpers::ComboBox::LoadHousesAddonMulti(*pComboBox, true);
        ControlHelpers::ComboBox::LoadHousesAddon2(*pComboBox, true);
        break;
    case 43: //Tag List for action 70

        pComboBox->DeleteAllStrings();

        
        pComboBox->LockWindowUpdate();

        if (ExtConfigs::SortByTriggerName && pComboBox->GetDlgCtrlID() == 1402)
        {
            std::map<ppmfc::CString, ppmfc::CString> collector;

            if (auto const pSection = pINI->GetSection("Tags"))
            {
                for (auto& pair : pSection->GetEntities())
                {
                    auto splits = STDHelpers::SplitString(pair.second, 2);
                    ppmfc::CString buffer(pair.first);
                    buffer += " (" + splits[1] + ")" + " (" + splits[2] + ")";
                    collector.insert(std::make_pair(splits[1], buffer));
                }
            }

            for (auto& pair : collector)
                pComboBox->AddString(pair.second);

            collector.clear();
        }
        else
        {
            if (auto pSection = pINI->GetSection("Tags"))
            {
                for (auto& pair : pSection->GetEntities())
                {
                    auto splits = STDHelpers::SplitString(pair.second, 2);
                    ppmfc::CString buffer = pair.first;
                    buffer += " (" + splits[1] + ")" + " (" + splits[2] + ")";
                    pComboBox->AddString(buffer);
                }
            }
        }

        pComboBox->UnlockWindowUpdate();
        break;
    case 44: // Country Addons -1
        ControlHelpers::ComboBox::LoadCountries(*pComboBox, true);
        ControlHelpers::ComboBox::LoadHousesAddon3(*pComboBox, true);
        break;
    case 45: // pure House
        ControlHelpers::ComboBox::LoadCountries(*pComboBox, true);
        ControlHelpers::ComboBox::LoadHousesAddonMulti(*pComboBox, true);
        break;
    case 46: // Country Addons -1 + multi (ares)
        ControlHelpers::ComboBox::LoadCountries(*pComboBox, true);
        ControlHelpers::ComboBox::LoadHousesAddonMultiAres(*pComboBox, true);
        ControlHelpers::ComboBox::LoadHousesAddon3(*pComboBox, true);
        break;
    case 47: // Country + multi (ares)
        ControlHelpers::ComboBox::LoadCountries(*pComboBox, true);
        ControlHelpers::ComboBox::LoadHousesAddonMultiAres(*pComboBox, true);
        break;
    default: // Not a valid param
        break;
    }

    return 0x43D058;
}

DEFINE_HOOK(43D037, Miscs_LoadParams_AdjustComboboxDropdownWidth, C)
{
    if (ExtConfigs::AdjustDropdownWidth)
    {
        GET_STACK(ppmfc::CComboBox*, pComboBox, STACK_OFFS(0x18, -0x4));

        int nWidth = 120;
        for (int i = 0; i < pComboBox->GetCount() && nWidth <= ExtConfigs::AdjustDropdownWidth_Max; ++i)
            nWidth = std::max(nWidth, pComboBox->GetLBTextLen(i) * ExtConfigs::AdjustDropdownWidth_Factor);

        nWidth = std::min(nWidth, ExtConfigs::AdjustDropdownWidth_Max);
        pComboBox->SetDroppedWidth(nWidth);
    }

    return 0;
}

DEFINE_HOOK(43CFE4, Miscs_LoadParams_SpeechBubble, 6)
{
    auto AddString = [](HWND hComboBox, const char* lpString)
    {
        SendMessage(hComboBox, CB_ADDSTRING, NULL, (LPARAM)lpString);
    };

    GET(HWND, hComboBox, ECX);
    while (SendMessage(hComboBox, CB_GETCOUNT, 0, NULL) > 0)
        SendMessage(hComboBox, CB_DELETESTRING, 0, 0);
    AddString(hComboBox, "0 - 无");
    AddString(hComboBox, "1 - 星号(*)");
    AddString(hComboBox, "2 - 问号(?)");
    AddString(hComboBox, "3 - 感叹号(!)");
    return 0x43D037;
}

DEFINE_HOOK(441910, Miscs_LoadParams_TutorialTexts, 7)
{
    GET_STACK(ppmfc::CComboBox*, pComboBox, 0x4);
    if (ExtConfigs::TutorialTexts_Hide)
    {
        pComboBox->DeleteAllStrings();
        return 0x441A34;
    }
    if (ExtConfigs::TutorialTexts_Fix)
    {   
        pComboBox->DeleteAllStrings();
        for (auto& x : FA2sp::TutorialTextsMap)
            pComboBox->AddString(x.first + " : " + x.second);
        Logger::Debug("%d csf entities added.\n", FA2sp::TutorialTextsMap.size());
        return 0x441A34;
    }
    return 0;
}

DEFINE_HOOK(441A40, Miscs_LoadParams_Triggers, 6)
{
    GET_STACK(ppmfc::CComboBox*, pComboBox, 0x4);

    pComboBox->DeleteAllStrings();

    auto const pINI = CMapData::GetMapDocument(true);
    pComboBox->LockWindowUpdate();

    if (ExtConfigs::SortByTriggerName && pComboBox->GetDlgCtrlID() == 1402)
    {
        std::map<ppmfc::CString, ppmfc::CString> collector;
        
        if (auto const pSection = pINI->GetSection("Triggers"))
        {
            for (auto& pair : pSection->GetEntities())
            {
                auto splits = STDHelpers::SplitString(pair.second, 2);
                ppmfc::CString buffer(pair.first);
                buffer += " (" + splits[2] + ")";
                collector.insert(std::make_pair(splits[2], buffer));
            }
        }

        for (auto& pair : collector)
            pComboBox->AddString(pair.second);

        collector.clear();
    }
    else
    {
        if (auto pSection = pINI->GetSection("Triggers"))
        {
            for (auto& pair : pSection->GetEntities())
            {
                auto splits = STDHelpers::SplitString(pair.second, 2);
                ppmfc::CString buffer = pair.first;
                buffer += " (" + splits[2] + ")";
                pComboBox->AddString(buffer);
            }
        }
    }

    pComboBox->UnlockWindowUpdate();
    return 0x441DF6;
}

