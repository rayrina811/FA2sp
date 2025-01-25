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

    if (IsTeamType || IsTrigger)
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
