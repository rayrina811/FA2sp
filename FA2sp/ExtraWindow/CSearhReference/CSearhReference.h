#pragma once

#include <FA2PP.h>
#include <map>
#include <vector>
#include <string>
#include <regex>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../CNewScript/CNewScript.h"

// A static window class
class CSearhReference
{
public:
    enum Controls { Listbox = 1000, Refresh = 1001, ObjectText = 1002};
    static void Create(CFinalSunDlg* pWnd);

    static HWND GetHandle()
    {
        return CSearhReference::m_hwnd;
    }
    static void SetSearchID(const char* id) 
    {
        SearchID = id;
    }
    // 0 team, 1 trigger, 2 taskforce/script
    static void SetSearchType(int idx) 
    {
        IsTeamType = false;
        IsTrigger = false;
        switch (idx)
        {
        case 0:
            IsTeamType = true;
            break;
        case 1:
            IsTrigger = true;
            break;
        default:
            break;
        }
    }

protected:
    static void Initialize(HWND& hWnd);
    static void Close(HWND& hWnd);
    static void Update();

    static BOOL CALLBACK DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static void ListBoxProc(HWND hWnd, WORD nCode, LPARAM lParam);
    static void OnSelchangeListbox(HWND hWnd);


private:
    static HWND m_hwnd;
    static CFinalSunDlg* m_parent;
    static CINI& map;
    static MultimapHelper& rules;

    static HWND hListbox;
    static HWND hRefresh;
    static HWND hObjectText;
    static ppmfc::CString SearchID;
public:
    static bool IsTeamType;
    static bool IsTrigger;

};

