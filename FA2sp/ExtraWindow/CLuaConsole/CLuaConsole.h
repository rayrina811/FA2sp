#pragma once

#include <FA2PP.h>
#include <map>
#include <vector>
#include <string>
#include <regex>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Sol/sol.hpp"

// A static window class
class CLuaConsole
{
public:
    enum Controls {
        Execute = 1000,
        Run = 1001,
        OutputBox = 1002,
        InputBox = 1003,
        Scripts = 1004,
        Description = 1005,
        OutputText = 1006,
        InputText = 1007,
        ScriptText = 1008

    };
    static void Create(CFinalSunDlg* pWnd);

    static HWND GetHandle()
    {
        return CLuaConsole::m_hwnd;
    }

protected:
    static void Initialize(HWND& hWnd);
    static void Close(HWND& hWnd);
    static void Update(HWND& hWnd);
    static void OnClickRun(bool fromFile);
    static void ListBoxProc(HWND& hWnd, WORD nCode, LPARAM lParam);

    static BOOL CALLBACK DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);


private:
    static HWND m_hwnd;
    static CFinalSunDlg* m_parent;
    static int origWndWidth;
    static int origWndHeight;
    static int minWndWidth;
    static int minWndHeight;
    static bool minSizeSet;

public:
    static HWND hExecute;
    static HWND hRun;
    static HWND hOutputBox;
    static HWND hInputBox;
    static HWND hOutputText;
    static HWND hInputText;
    static HWND hScripts;
    static sol::state Lua;
};

