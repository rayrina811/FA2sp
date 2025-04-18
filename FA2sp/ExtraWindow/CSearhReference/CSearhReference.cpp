#include "CSearhReference.h"
#include "../CNewTeamTypes/CNewTeamTypes.h"
#include "../CNewTrigger/CNewTrigger.h"
#include "../CNewAITrigger/CNewAITrigger.h"
#include "../Common.h"
#include "../../FA2sp.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Helpers/MultimapHelper.h"

#include <CLoading.h>
#include <CFinalSunDlg.h>
#include <CObjectDatas.h>
#include <CMapData.h>
#include <CIsoView.h>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Ext/CMapData/Body.h"

HWND CSearhReference::m_hwnd;
CFinalSunDlg* CSearhReference::m_parent;
CINI& CSearhReference::map = CINI::CurrentDocument;
MultimapHelper& CSearhReference::rules = Variables::Rules;

HWND CSearhReference::hListbox;
HWND CSearhReference::hRefresh;
HWND CSearhReference::hObjectText;
ppmfc::CString CSearhReference::SearchID = "";
bool CSearhReference::IsTeamType = false;
bool CSearhReference::IsTrigger = false;
bool CSearhReference::IsVariable = false;
std::map<int, ScriptParamPos> CSearhReference::LocalVariableScripts;
std::map<int, int> CSearhReference::LocalVariableEvents;
std::map<int, int> CSearhReference::LocalVariableActions;
std::map<int, std::map<int, ppmfc::CString>> CSearhReference::LocalVariableParamAffectedEvents;
std::map<int, std::map<int, ppmfc::CString>> CSearhReference::LocalVariableParamAffectedActions;

void CSearhReference::Create(CFinalSunDlg* pWnd)
{
    m_parent = pWnd;
    m_hwnd = CreateDialog(
        static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(310),
        pWnd->GetSafeHwnd(),
        CSearhReference::DlgProc
    );

    if (m_hwnd)
        ShowWindow(m_hwnd, SW_SHOW);
    else
    {
        Logger::Error("Failed to create CSearhReference.\n");
        m_parent = NULL;
        return;
    }

    ExtraWindow::CenterWindowPos(m_parent->GetSafeHwnd(), m_hwnd);
}

void CSearhReference::Initialize(HWND& hWnd)
{
    ppmfc::CString buffer;
    if (Translations::GetTranslationItem("SearchReferenceTitle", buffer))
        SetWindowText(hWnd, buffer);

    auto Translate = [&hWnd, &buffer](int nIDDlgItem, const char* pLabelName)
        {
            HWND hTarget = GetDlgItem(hWnd, nIDDlgItem);
            if (Translations::GetTranslationItem(pLabelName, buffer))
                SetWindowText(hTarget, buffer);
        };
    
	Translate(1001, "SearchReferenceRefresh");

    hListbox = GetDlgItem(hWnd, Controls::Listbox);
    hRefresh = GetDlgItem(hWnd, Controls::Refresh);
    hObjectText = GetDlgItem(hWnd, Controls::ObjectText);

    Update();
}

void CSearhReference::Close(HWND& hWnd)
{
    EndDialog(hWnd, NULL);

    CSearhReference::m_hwnd = NULL;
    CSearhReference::m_parent = NULL;
    CSearhReference::SearchID = "";

}

BOOL CALLBACK CSearhReference::DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        CSearhReference::Initialize(hWnd);
        return TRUE;
    }
    case WM_COMMAND:
    {
        WORD ID = LOWORD(wParam);
        WORD CODE = HIWORD(wParam);
        switch (ID)
        {
        case Controls::Listbox:
            ListBoxProc(hWnd, CODE, lParam);
            break;
        case Controls::Refresh:
            if (CODE == BN_CLICKED)
                Update();
            break;
        default:
            break;
        }
        break;
    }
    case WM_CLOSE:
    {
        CSearhReference::Close(hWnd);
        return TRUE;
    }
    case 114514: // used for update
    {
        Update();
        return TRUE;
    }

    }

    // Process this message through default handler
    return FALSE;
}

void CSearhReference::ListBoxProc(HWND hWnd, WORD nCode, LPARAM lParam)
{
    if (SendMessage(hListbox, LB_GETCOUNT, NULL, NULL) <= 0)
        return;

    switch (nCode)
    {
    case LBN_SELCHANGE:
    case LBN_DBLCLK:
        OnSelchangeListbox(hWnd);
        break;
    default:
        break;
    }

}
void CSearhReference::OnSelchangeListbox(HWND hWnd)
{
    if (SendMessage(hListbox, LB_GETCURSEL, NULL, NULL) < 0 || SendMessage(hListbox, LB_GETCOUNT, NULL, NULL) <= 0)
    {
        return;
    }

    int idx = SendMessage(hListbox, LB_GETCURSEL, 0, NULL);
    char buffer[512]{ 0 };
    SendMessage(hListbox, LB_GETTEXT, idx, (LPARAM)buffer);

    ppmfc::CString ID = buffer;
    STDHelpers::TrimIndex(ID);

    if (IsTeamType || IsTrigger || IsVariable)
    {
        int data = SendMessage(hListbox, LB_GETITEMDATA, idx, 0);
        if (data == 1)
        {
            if (CNewTrigger::GetHandle() == NULL)
                CNewTrigger::Create(m_parent);

            auto dlg = GetDlgItem(CNewTrigger::GetHandle(), CNewTrigger::Controls::SelectedTrigger);
            auto idx = SendMessage(dlg, CB_FINDSTRINGEXACT, 0, (LPARAM)ExtraWindow::GetTriggerDisplayName(ID).m_pchData);
            if (idx == CB_ERR)
                return;
            SendMessage(dlg, CB_SETCURSEL, idx, NULL);
            CNewTrigger::OnSelchangeTrigger();
        }
        else if (data == 2)
        {
            if (CNewAITrigger::GetHandle() == NULL)
                CNewAITrigger::Create(m_parent);

            auto dlg = GetDlgItem(CNewAITrigger::GetHandle(), CNewAITrigger::Controls::SelectedAITrigger);
            auto idx = SendMessage(dlg, CB_FINDSTRINGEXACT, 0, (LPARAM)ExtraWindow::GetAITriggerDisplayName(ID).m_pchData);
            if (idx == CB_ERR)
                return;
            SendMessage(dlg, CB_SETCURSEL, idx, NULL);
            CNewAITrigger::OnSelchangeAITrigger();
        }
        else if (data >= 500 && data < 550)
        {
            if (CNewScript::GetHandle() == NULL)
                CNewScript::Create(m_parent);

            auto dlg = GetDlgItem(CNewScript::GetHandle(), CNewScript::Controls::SelectedScript);
            auto idx = SendMessage(dlg, CB_FINDSTRINGEXACT, 0, (LPARAM)ExtraWindow::GetTeamDisplayName(ID).m_pchData);
            if (idx == CB_ERR)
                return;
            SendMessage(dlg, CB_SETCURSEL, idx, NULL);
            CNewScript::OnSelchangeScript();
            auto dlg2 = GetDlgItem(CNewScript::GetHandle(), CNewScript::Controls::ActionsListBox);
            SendMessage(dlg2, LB_SETCURSEL, data - 500, NULL);
            CNewScript::OnSelchangeActionListbox();
        }
    }
    else
    {
        if (CNewTeamTypes::GetHandle() == NULL)
            CNewTeamTypes::Create(m_parent);

        auto dlg = GetDlgItem(CNewTeamTypes::GetHandle(), CNewTeamTypes::Controls::SelectedTeam);
        auto idx = SendMessage(dlg, CB_FINDSTRINGEXACT, 0, (LPARAM)buffer);
        if (idx == CB_ERR)
            return;
        SendMessage(dlg, CB_SETCURSEL, idx, NULL);
        CNewTeamTypes::OnSelchangeTeamtypes();
    }

}

void CSearhReference::Update()
{
    ShowWindow(m_hwnd, SW_SHOW);
    SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

    while (SendMessage(hListbox, LB_DELETESTRING, 0, NULL) != CB_ERR);
    int idx = 0;
    ppmfc::CString tmp;
    if (IsTeamType || IsTrigger)
    {
        if (IsTeamType)
            SendMessage(hObjectText, WM_SETTEXT, 0, (LPARAM)ExtraWindow::GetTeamDisplayName(SearchID).m_pchData);
        else if (IsTrigger)
            SendMessage(hObjectText, WM_SETTEXT, 0, (LPARAM)ExtraWindow::GetTriggerDisplayName(SearchID).m_pchData);
        for (auto& triggerPair : CMapDataExt::Triggers)
        {
            auto& trigger = triggerPair.second;
            int index = 0;
            bool addEvent = false;
            bool addAction = false;
            bool addAttached = false;
            ppmfc::CString eventCount = " ";
            eventCount += Translations::TranslateOrDefault("Event", "Event");
            eventCount += "[";
            ppmfc::CString actionCount = " ";
            actionCount += Translations::TranslateOrDefault("Action", "Action");
            actionCount += "[";
            ppmfc::CString attached = " ";
            attached += Translations::TranslateOrDefault("SearchReference.AttachedTrigger", "Attached");
            for (auto& e : trigger->Events)
            {
                for (auto& ep : e.Params)
                {
                    if (ep == SearchID)
                    {
                        tmp.Format("%d", index);
                        if (addEvent)
                            eventCount += ", ";
                        eventCount += tmp;
                        addEvent = true;
                    }
                }
                index++;
            }
            eventCount += "]";
            index = 0;
            for (auto& a : trigger->Actions)
            {
                for (auto& ap : a.Params)
                {
                    if (ap == SearchID)
                    {
                        tmp.Format("%d", index);
                        if (addAction)
                            actionCount += ", ";
                        actionCount += tmp;
                        addAction = true;
                    }
                }
                index++;
            }
            if (trigger->AttachedTrigger == SearchID)
                addAttached = true;

            actionCount += "]";
            if (addAction || addEvent || addAttached)
            {
                auto text = ExtraWindow::GetTriggerDisplayName(trigger->ID);
                if (addEvent)
                    text += eventCount;
                if (addAction)
                    text += actionCount;
                if (addAttached)
                    text += attached;
                SendMessage(
                    hListbox,
                    LB_SETITEMDATA,
                    SendMessage(hListbox, LB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)text.m_pchData),
                    1
                );
            }

            // 1 means trigger
        }
        if (IsTeamType)
            if (auto pSection = map.GetSection("AITriggerTypes"))
            {
                for (auto& pair : pSection->GetEntities())
                {
                    //01000139=GM2-ORCA-10,01000138,GoodMid2,1,0,GAWEAT,0100000003000000000000000000000000000000000000000000000000000000,70.000000,30.000000,70.000000,1,0,3,0,<none>,1,1,1
                    auto atoms = STDHelpers::SplitString(map.GetString("AITriggerTypes", pair.first));
                    if (atoms.size() < 18) continue;
                    if (atoms[1] == SearchID || atoms[14] == SearchID)
                    {
                        SendMessage(
                            hListbox,
                            LB_SETITEMDATA,
                            SendMessage(hListbox, LB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)(pair.first + " (" + atoms[0] + ")").m_pchData),
                            2
                        );
                        // 2 means aitrigger
                    }
                }
            }
    }
    else if (IsVariable)
    {
        ppmfc::CString text;
        text.Format("%s - %s", SearchID, map.GetString("VariableNames", SearchID));
        SendMessage(hObjectText, WM_SETTEXT, 0, (LPARAM)text.m_pchData);

        if (LocalVariableScripts.empty())
        {
            if (auto pSection = CINI::FAData->GetSection("ScriptsRA2"))
            {
                for (const auto& [key, value] : pSection->GetEntities())
                {
                    auto atoms = STDHelpers::SplitString(value, 1);
                    auto scriptParams = STDHelpers::SplitString(CINI::FAData->GetString("ScriptParams", atoms[1]));
                    if (scriptParams.size() >= 2)
                    {
                        auto newParamTypes = STDHelpers::SplitString(CINI::FAData->GetString("NewParamTypes", scriptParams[1]), 4);
                        auto& sectionName = newParamTypes[0];
                        auto& loadFrom = newParamTypes[1];

                        if (sectionName == "VariableNames" && (loadFrom == "3" || loadFrom == "map"))
                        {
                            LocalVariableScripts[atoi(key)] = scriptParams.size() == 4 ? ScriptParamPos::low : ScriptParamPos::normal;
                        }
                    }
                    if (scriptParams.size() >= 4)
                    {
                        auto newParamTypes = STDHelpers::SplitString(CINI::FAData->GetString("NewParamTypes", scriptParams[3]), 4);
                        auto& sectionName = newParamTypes[0];
                        auto& loadFrom = newParamTypes[1];

                        if (sectionName == "VariableNames" && (loadFrom == "3" || loadFrom == "map"))
                        {
                            LocalVariableScripts[atoi(key)] = ScriptParamPos::high;
                        }
                    }
                }
            }
        }
        if (LocalVariableEvents.empty())
        {
            LocalVariableParamAffectedEvents.clear();
            if (auto pSection = CINI::FAData->GetSection("EventsRA2"))
            {
                for (const auto& [key, value] : pSection->GetEntities())
                {
                    auto eventInfos = STDHelpers::SplitString(value, 8);
                    ppmfc::CString paramType[2];
                    paramType[0] = eventInfos[1];
                    paramType[1] = eventInfos[2];
                    std::vector<ppmfc::CString> pParamTypes[2];
                    pParamTypes[0] = STDHelpers::SplitString(CINI::FAData->GetString("ParamTypes", paramType[0], "MISSING,0"));
                    pParamTypes[1] = STDHelpers::SplitString(CINI::FAData->GetString("ParamTypes", paramType[1], "MISSING,0"));
                    ppmfc::CString code = "0";
                    if (pParamTypes[0].size() == 3) code = pParamTypes[0][2];

                    if (CINI::FAData->KeyExists("NewParamTypes", pParamTypes[0][1]))
                    {
                        auto newParamTypes = STDHelpers::SplitString(CINI::FAData->GetString("NewParamTypes", pParamTypes[0][1]), 4);
                        auto& sectionName = newParamTypes[0];
                        auto& loadFrom = newParamTypes[1];

                        if (sectionName == "VariableNames" && (loadFrom == "3" || loadFrom == "map"))
                        {
                            LocalVariableEvents[atoi(key)] = code == "2" ? 1 : 0;
                        }
                    }
                    if (CINI::FAData->KeyExists("NewParamTypes", pParamTypes[1][1]))
                    {
                        auto newParamTypes = STDHelpers::SplitString(CINI::FAData->GetString("NewParamTypes", pParamTypes[1][1]), 4);
                        auto& sectionName = newParamTypes[0];
                        auto& loadFrom = newParamTypes[1];

                        if (sectionName == "VariableNames" && (loadFrom == "3" || loadFrom == "map"))
                        {
                            LocalVariableEvents[atoi(key)] = code == "2" ? 2 : 1;
                        }
                    }
                }
            }
            
            auto& params = CNewTrigger::EventParamAffectedParams;
            for (const auto& param : params)
            {
                for (const auto& [value, paramType] : param.ParamMap)
                {
                    ppmfc::CString type = STDHelpers::SplitString(CINI::FAData->GetString("ParamTypes", paramType, "MISSING,0"))[1];
                    if (CINI::FAData->KeyExists("NewParamTypes", type))
                    {
                        auto newParamTypes = STDHelpers::SplitString(CINI::FAData->GetString("NewParamTypes", type), 4);
                        auto& sectionName = newParamTypes[0];
                        auto& loadFrom = newParamTypes[1];

                        if (sectionName == "VariableNames" && (loadFrom == "3" || loadFrom == "map"))
                        {
                            ppmfc::CString eventKey;
                            eventKey.Format("%d", param.Index);
                            auto eventInfos = STDHelpers::SplitString(CINI::FAData->GetString("EventsRA2", eventKey), 8);
                            ppmfc::CString paramType[2];
                            paramType[0] = eventInfos[1];
                            paramType[1] = eventInfos[2];
                            std::vector<ppmfc::CString> pParamTypes[2];
                            pParamTypes[0] = STDHelpers::SplitString(CINI::FAData->GetString("ParamTypes", paramType[0], "MISSING,0"));
                            pParamTypes[1] = STDHelpers::SplitString(CINI::FAData->GetString("ParamTypes", paramType[1], "MISSING,0"));
                            ppmfc::CString code = "0";
                            if (pParamTypes[0].size() == 3) code = pParamTypes[0][2];

                            LocalVariableEvents[param.Index] = param.AffectedParam + 10 + (code == "2" ? 1 : 0);
                            LocalVariableParamAffectedEvents[param.Index][param.SourceParam + (code == "2" ? 1 : 0)] =  value;
                            break;
                        }
                    }         
                }
            }
        }
        if (LocalVariableActions.empty())
        {
            LocalVariableParamAffectedActions.clear();
            if (auto pSection = CINI::FAData->GetSection("ActionsRA2"))
            {
                for (const auto& [key, value] : pSection->GetEntities())
                {
                    auto actionInfos = STDHelpers::SplitString(value, 13);
                    ppmfc::CString paramType[7];
                    for (int i = 0; i < 7; i++)
                        paramType[i] = actionInfos[i + 1];

                    std::vector<ppmfc::CString> pParamTypes[7];
                    for (int i = 0; i < 7; i++)
                        pParamTypes[i] = STDHelpers::SplitString(CINI::FAData->GetString("ParamTypes", paramType[i], "MISSING,0"));

                    for (int i = 0; i < 7; i++)
                    {
                        if (CINI::FAData->KeyExists("NewParamTypes", pParamTypes[i][1]))
                        {
                            auto newParamTypes = STDHelpers::SplitString(CINI::FAData->GetString("NewParamTypes", pParamTypes[i][1]), 4);
                            auto& sectionName = newParamTypes[0];
                            auto& loadFrom = newParamTypes[1];

                            if (sectionName == "VariableNames" && (loadFrom == "3" || loadFrom == "map"))
                            {
                                LocalVariableActions[atoi(key)] = i;
                                break;
                            }
                        }
                    }
                }
            }

            auto& params = CNewTrigger::ActionParamAffectedParams;
            for (const auto& param : params)
            {
                for (const auto& [value, paramType] : param.ParamMap)
                {
                    ppmfc::CString type = STDHelpers::SplitString(CINI::FAData->GetString("ParamTypes", paramType, "MISSING,0"))[1];
                    if (CINI::FAData->KeyExists("NewParamTypes", type))
                    {
                        auto newParamTypes = STDHelpers::SplitString(CINI::FAData->GetString("NewParamTypes", type), 4);
                        auto& sectionName = newParamTypes[0];
                        auto& loadFrom = newParamTypes[1];

                        if (sectionName == "VariableNames" && (loadFrom == "3" || loadFrom == "map"))
                        {
                            ppmfc::CString actionKey;
                            actionKey.Format("%d", param.Index);
                            auto actionInfos = STDHelpers::SplitString(CINI::FAData->GetString("ActionsRA2", actionKey), 13);
                            ppmfc::CString paramType[7];
                            for (int i = 0; i < 7; i++)
                                paramType[i] = actionInfos[i + 1];

                            int paramIdx[7];
                            for (int i = 0; i < 7; i++)
                                paramIdx[i] = atoi(paramType[i]);

                            std::vector<int> affectedParamIndex2RealIndex;
                            for (int i = 0; i < 7; i++)
                            {
                                if (paramIdx[i] > 0)
                                {
                                    affectedParamIndex2RealIndex.push_back(i);
                                }
                            }
                            if (affectedParamIndex2RealIndex.size() > param.AffectedParam && affectedParamIndex2RealIndex.size() > param.SourceParam)
                            {
                                LocalVariableActions[param.Index] = affectedParamIndex2RealIndex[param.AffectedParam] + 10;
                                LocalVariableParamAffectedEvents[param.Index][affectedParamIndex2RealIndex[param.SourceParam]] = value;
                            }
                            break;
                        }
                    }
                }
            }
        }
        if (auto pSection = map.GetSection("ScriptTypes"))
        {
            for (const auto& [_, id] : pSection->GetEntities())
            {
                if (auto pScript = map.GetSection(id))
                {
                    ppmfc::CString key;
                    for (int i = 0; i < 50; i++)
                    {
                        key.Format("%d", i);
                        if (map.KeyExists(id, key))
                        {
                            auto atoms = STDHelpers::SplitString(map.GetString(id, key), 1);
                            int action = atoi(atoms[0]);
                            int param = atoi(atoms[1]);
                            if (LocalVariableScripts.find(action) != LocalVariableScripts.end())
                            {
                                int targetParam = 0;
                                switch (LocalVariableScripts[action])
                                {
                                case ScriptParamPos::normal:
                                    targetParam = param;
                                    break;
                                case ScriptParamPos::low:
                                    targetParam = LOWORD(param);
                                    break;
                                case ScriptParamPos::high:
                                    targetParam = HIWORD(param);
                                    break;
                                default:
                                    continue;
                                    break;
                                }
                                if (atoi(SearchID) == targetParam)
                                {
                                    ppmfc::CString text;
                                    text.Format("%s %s[%d]", ExtraWindow::GetTeamDisplayName(id), Translations::TranslateOrDefault("SearchReferenceScriptLine", "Line"), i);

                                    SendMessage(
                                        hListbox,
                                        LB_SETITEMDATA,
                                        SendMessage(hListbox, LB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)text.m_pchData),
                                        500 + i
                                    );
                                    // 500+ means script and line
                                }
                            }

                        }
                        else
                        {
                            break;
                        }
                    }
                }
            }
        }

        for (const auto& [id, trigger] : CMapDataExt::Triggers)
        {
            int i = 0;
            for (const auto& eve : trigger->Events)
            {
                if (LocalVariableEvents.find(atoi(eve.EventNum)) != LocalVariableEvents.end())
                {
                    int pos = LocalVariableEvents[atoi(eve.EventNum)];
                    if (pos > 10)
                    {
                        const auto& sourceParams = LocalVariableParamAffectedEvents[atoi(eve.EventNum)];
                        for (const auto& [s, v] : sourceParams)
                        {
                            if (eve.Params[s] == v)
                            {
                                pos -= 10;
                                break;
                            }
                        }
                    }
                    if (pos < 0 || pos >= 3)
                    {
                        i++;
                        continue;
                    }
                    int value = atoi(eve.Params[pos]);
                    if (atoi(SearchID) == value)
                    {
                        ppmfc::CString text;
                        text.Format("%s %s[%d]", ExtraWindow::GetTriggerDisplayName(id), Translations::TranslateOrDefault("Event", "Event"), i);

                        SendMessage(
                            hListbox,
                            LB_SETITEMDATA,
                            SendMessage(hListbox, LB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)text.m_pchData),
                            1
                        );
                    }
                }
                i++;
            }
            i = 0;
            for (const auto& act : trigger->Actions)
            {
                if (LocalVariableActions.find(atoi(act.ActionNum)) != LocalVariableActions.end())
                {
                    int pos = LocalVariableActions[atoi(act.ActionNum)];
                    if (pos > 10)
                    {
                        const auto& sourceParams = LocalVariableParamAffectedEvents[atoi(act.ActionNum)];
                        for (const auto& [s, v] : sourceParams)
                        {
                            if (act.Params[s] == v)
                            {
                                pos -= 10;
                                break;
                            }
                        }
                    }
                    if (pos < 0 || pos >= 7)
                    {
                        i++;
                        continue;
                    }
                    int value = atoi(act.Params[pos]);
                    if (atoi(SearchID) == value)
                    {
                        ppmfc::CString text;
                        text.Format("%s %s[%d]", ExtraWindow::GetTriggerDisplayName(id), Translations::TranslateOrDefault("Action", "Action"), i);

                        SendMessage(
                            hListbox,
                            LB_SETITEMDATA,
                            SendMessage(hListbox, LB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)text.m_pchData),
                            1
                        );
                    }
                }
                i++;
            }
        }
    }   
    else
    {
        SendMessage(hObjectText, WM_SETTEXT, 0, (LPARAM)ExtraWindow::GetTeamDisplayName(SearchID).m_pchData);
        if (auto pSection = map.GetSection("TeamTypes"))
        {
            for (auto& pair : pSection->GetEntities())
            {
                auto refTaskforce = map.GetString(pair.second, "TaskForce");
                auto refScript = map.GetString(pair.second, "Script");
                if (refTaskforce == SearchID || refScript == SearchID)
                {
                    SendMessage(hListbox, LB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)ExtraWindow::GetTeamDisplayName(pair.second).m_pchData);
                }
            }
        }
    }

    return;
}
