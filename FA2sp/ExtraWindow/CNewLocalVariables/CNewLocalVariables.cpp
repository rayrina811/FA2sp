#include "CNewLocalVariables.h"
#include "../Common.h"
#include "../../FA2sp.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Helpers/MultimapHelper.h"
#include <CLoading.h>
#include <CFinalSunDlg.h>
#include <CObjectDatas.h>
#include "../CSearhReference/CSearhReference.h"

HWND CNewLocalVariables::m_hwnd;
CFinalSunDlg* CNewLocalVariables::m_parent;
CINI& CNewLocalVariables::map = CINI::CurrentDocument;
std::map<int, ppmfc::CString> CNewLocalVariables::VaribaleLabels;

HWND CNewLocalVariables::hVariables;
HWND CNewLocalVariables::hName;
HWND CNewLocalVariables::hValue;
HWND CNewLocalVariables::hNew;
HWND CNewLocalVariables::hSearch;
int CNewLocalVariables::SelectedIndex;
ppmfc::CString CNewLocalVariables::SelectedKey;

void CNewLocalVariables::Create(CFinalSunDlg* pWnd)
{
    m_parent = pWnd;
    m_hwnd = CreateDialog(
        static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(324),
        pWnd->GetSafeHwnd(),
        CNewLocalVariables::DlgProc
    );

    if (m_hwnd)
        ShowWindow(m_hwnd, SW_SHOW);
    else
    {
        Logger::Error("Failed to create CNewLocalVariables.\n");
        m_parent = NULL;
        return;
    }
}

void CNewLocalVariables::Initialize(HWND& hWnd)
{
    ppmfc::CString buffer;
    if (Translations::GetTranslationItem("LocalVariablesTitle", buffer))
        SetWindowText(hWnd, buffer);

    auto Translate = [&hWnd, &buffer](int nIDDlgItem, const char* pLabelName)
        {
            HWND hTarget = GetDlgItem(hWnd, nIDDlgItem);
            if (Translations::GetTranslationItem(pLabelName, buffer))
                SetWindowText(hTarget, buffer);
        };
    
	Translate(1000, "LocalVariablesSelect");
	Translate(1002, "LocalVariablesName");
	Translate(1004, "LocalVariablesValue");
	Translate(1006, "LocalVariablesNew");
	Translate(1007, "SearchReferenceTitle");

    hVariables = GetDlgItem(hWnd, Controls::Variables);
    hName = GetDlgItem(hWnd, Controls::Name);
    hValue = GetDlgItem(hWnd, Controls::Value);
    hNew = GetDlgItem(hWnd, Controls::New);
    hSearch = GetDlgItem(hWnd, Controls::Search);

    Update();
}

void CNewLocalVariables::Close(HWND& hWnd)
{
    EndDialog(hWnd, NULL);

    CNewLocalVariables::m_hwnd = NULL;
    CNewLocalVariables::m_parent = NULL;
}

BOOL CALLBACK CNewLocalVariables::DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        CNewLocalVariables::Initialize(hWnd);
        return TRUE;
    }
    case WM_COMMAND:
    {
        WORD ID = LOWORD(wParam);
        WORD CODE = HIWORD(wParam);
        switch (ID)
        {
        case Controls::Variables:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeVariable();
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeVariable(true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupCComboBox(hVariables, VaribaleLabels, true);
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            break;
        case Controls::New:
            if (CODE == BN_CLICKED)
                OnClickNew();
            break;
        case Controls::Search:
            if (CODE == BN_CLICKED)
                OnClickSearchReference();
            break;
        case Controls::Name:
            if (CODE == EN_CHANGE && SelectedKey != "" && map.KeyExists("VariableNames", SelectedKey))
            {
                char buffer[512]{ 0 };
                GetWindowText(hName, buffer, 511);
                auto atom = STDHelpers::SplitString(map.GetString("VariableNames", SelectedKey), 1);
                ppmfc::CString value;
                value.Format("%s,%s", buffer, atom[1]);
                map.WriteString("VariableNames", SelectedKey, value);
                ppmfc::CString text;
                text.Format("%s - %s", SelectedKey, value);

                SendMessage(hVariables, CB_DELETESTRING, SelectedIndex, NULL);
                SendMessage(hVariables, CB_INSERTSTRING, SelectedIndex, (LPARAM)(LPCSTR)text.m_pchData);
                SendMessage(hVariables, CB_SETCURSEL, SelectedIndex, NULL);
            }
            break;
        case Controls::Value:
            if (CODE == EN_CHANGE && SelectedKey != "" && map.KeyExists("VariableNames", SelectedKey))
            {
                char buffer[512]{ 0 };
                GetWindowText(hValue, buffer, 511);
                auto atom = STDHelpers::SplitString(map.GetString("VariableNames", SelectedKey), 1);
                ppmfc::CString value;
                value.Format("%s,%s", atom[0], buffer);
                map.WriteString("VariableNames", SelectedKey, value);
                ppmfc::CString text;
                text.Format("%s - %s", SelectedKey, value);

                SendMessage(hVariables, CB_DELETESTRING, SelectedIndex, NULL);
                SendMessage(hVariables, CB_INSERTSTRING, SelectedIndex, (LPARAM)(LPCSTR)text.m_pchData);
                SendMessage(hVariables, CB_SETCURSEL, SelectedIndex, NULL);
            }
            break;
        default:
            break;
        }
        break;
    }
    case WM_CLOSE:
    {
        CNewLocalVariables::Close(hWnd);
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

void CNewLocalVariables::Update()
{
    ShowWindow(m_hwnd, SW_SHOW);
    SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

    while (SendMessage(hVariables, CB_DELETESTRING, 0, NULL) != CB_ERR);
    if (auto pSection = map.GetSection("VariableNames"))
    {
        int i = 0;
        ppmfc::CString key;
        for (const auto& [key, value] : pSection->GetEntities())
        { 
            ppmfc::CString text;
            text.Format("%s - %s", key, value);
            SendMessage(hVariables, CB_INSERTSTRING, i, (LPARAM)(LPCSTR)text.m_pchData);
            i++;
        }
    }

    int count = SendMessage(hVariables, CB_GETCOUNT, NULL, NULL);
    if (SelectedIndex < 0)
        SelectedIndex = 0;
    if (SelectedIndex > count - 1)
        SelectedIndex = count - 1;
    SendMessage(hVariables, CB_SETCURSEL, SelectedIndex, NULL);
    OnSelchangeVariable();

    return;
}

void CNewLocalVariables::OnSelchangeVariable(bool edited)
{
    char buffer[512]{ 0 };

    if (edited && (SendMessage(hVariables, CB_GETCOUNT, NULL, NULL) > 0 || !VaribaleLabels.empty()))
    {
        ExtraWindow::OnEditCComboBox(hVariables, VaribaleLabels);
        return;
    }

    SelectedIndex = SendMessage(hVariables, CB_GETCURSEL, NULL, NULL);
    if (SelectedIndex < 0 || SelectedIndex >= SendMessage(hVariables, CB_GETCOUNT, NULL, NULL))
    {
        SelectedKey = "";
        SendMessage(hName, WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hValue, WM_SETTEXT, 0, (LPARAM)"");
        return;
    }

    ppmfc::CString key;
    SendMessage(hVariables, CB_GETLBTEXT, SelectedIndex, (LPARAM)buffer);
    key = buffer;
    STDHelpers::TrimIndex(key);

    SelectedKey = key;

    auto atom = STDHelpers::SplitString(map.GetString("VariableNames", key), 1);

    SendMessage(hName, WM_SETTEXT, 0, (LPARAM)atom[0].m_pchData);
    SendMessage(hValue, WM_SETTEXT, 0, (LPARAM)atom[1].m_pchData);
}

void CNewLocalVariables::OnClickNew()
{
    auto key = CINI::GetAvailableKey("VariableNames");
    ppmfc::CString value = "New Variable,0";
    map.WriteString("VariableNames", key, value);
    ppmfc::CString text;
    text.Format("%s - %s", key, value);
    int index = SendMessage(hVariables, CB_GETCOUNT, NULL, NULL);
    SendMessage(hVariables, CB_INSERTSTRING, index, (LPARAM)(LPCSTR)text.m_pchData);
    SendMessage(hVariables, CB_SETCURSEL, index, NULL);
    OnSelchangeVariable();
}

void CNewLocalVariables::OnCloseupCComboBox(HWND& hWnd, std::map<int, ppmfc::CString>& labels, bool isComboboxSelectOnly)
{
    if (!ExtraWindow::OnCloseupCComboBox(hWnd, labels, isComboboxSelectOnly))
    {
        if (hWnd == hVariables)
        {
            OnSelchangeVariable();
        } 
    }
}


void CNewLocalVariables::OnClickSearchReference()
{
    if (SelectedIndex < 0 || SelectedKey == "" || !map.KeyExists("VariableNames", SelectedKey))
        return;

    CSearhReference::SetSearchType(3);
    CSearhReference::SetSearchID(SelectedKey);
    if (CSearhReference::GetHandle() == NULL)
    {
        CSearhReference::Create(m_parent);
    }
    else
    {
        ::SendMessage(CSearhReference::GetHandle(), 114514, 0, 0);
    }
}