#pragma once

#include <FA2PP.h>
#include <map>
#include <vector>
#include <string>
#include <regex>
#include "../../Ext/CFinalSunDlg/Body.h"

// A static window class
class COptions
{
public:
    static void Create(CFinalSunDlg* pWnd);
    enum Controls {
        List = 1000,
    };

    static HWND GetHandle()
    {
        return COptions::m_hwnd;
    }

protected:
    static void Initialize(HWND& hWnd);
    static void Update();
    static void Close(HWND& hWnd);

    static BOOL CALLBACK DlgProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);


private:
    static HWND m_hwnd;
    static HWND hList;
    static CFinalSunDlg* m_parent;
    static bool initialized;
};

