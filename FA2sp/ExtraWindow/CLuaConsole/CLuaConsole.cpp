#include "CLuaConsole.h"
#include "LuaFunctions.cpp"
#include "../../FA2sp.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Helpers/MultimapHelper.h"
#include "../Common.h"

#include <CLoading.h>
#include <CFinalSunDlg.h>
#include <CObjectDatas.h>
#include <CMapData.h>
#include <CIsoView.h>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Ext/CMapData/Body.h"
#include "../CObjectSearch/CObjectSearch.h"
#include <ctime>
#include <chrono>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

HWND CLuaConsole::m_hwnd;
CFinalSunDlg* CLuaConsole::m_parent;
HWND CLuaConsole::hExecute;
HWND CLuaConsole::hRun;
HWND CLuaConsole::hOutputBox;
HWND CLuaConsole::hInputBox;
HWND CLuaConsole::hOutputText;
HWND CLuaConsole::hInputText;
HWND CLuaConsole::hScripts;
int CLuaConsole::origWndWidth;
int CLuaConsole::origWndHeight;
int CLuaConsole::minWndWidth;
int CLuaConsole::minWndHeight;
bool CLuaConsole::minSizeSet;
sol::state CLuaConsole::Lua;
using namespace::LuaFunctions;

#define BUFFER_SIZE 800000
char Buffer[BUFFER_SIZE]{ 0 };


void CLuaConsole::Create(CFinalSunDlg* pWnd)
{
    HMODULE hModule = LoadLibrary(TEXT("Riched32.dll"));
    if (!hModule)
        MessageBox(NULL, Translations::TranslateOrDefault("FailedLoadRiched32DLL", "Could not Load Riched32.dll£¡"), Translations::TranslateOrDefault("Error", "Error"), MB_ICONERROR);

    m_parent = pWnd;
    m_hwnd = CreateDialog(
        static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(320),
        pWnd->GetSafeHwnd(),
        CLuaConsole::DlgProc
    );

    if (m_hwnd)
        ShowWindow(m_hwnd, SW_SHOW);
    else
    {
        Logger::Error("Failed to create CLuaConsole.\n");
        m_parent = NULL;
        return;
    }

    ExtraWindow::CenterWindowPos(m_parent->GetSafeHwnd(), m_hwnd);
}

void CLuaConsole::Initialize(HWND& hWnd)
{
    ppmfc::CString buffer;
    if (Translations::GetTranslationItem("LuaScriptConsoleTitle", buffer))
        SetWindowText(hWnd, buffer);

    auto Translate = [&hWnd, &buffer](int nIDDlgItem, const char* pLabelName)
        {
            HWND hTarget = GetDlgItem(hWnd, nIDDlgItem);
            if (Translations::GetTranslationItem(pLabelName, buffer))
                SetWindowText(hTarget, buffer);
        };

    Translate(1005, "LuaScriptConsoleDescription");
    Translate(1006, "LuaScriptConsoleOuput");
    Translate(1007, "LuaScriptConsoleInput");
    Translate(1008, "LuaScriptConsoleScripts");
    Translate(Controls::Execute, "LuaScriptConsoleExecute");
    Translate(Controls::Run, "LuaScriptConsoleRun");

    hExecute = GetDlgItem(hWnd, Controls::Execute);
    hRun = GetDlgItem(hWnd, Controls::Run);
    hOutputBox = GetDlgItem(hWnd, Controls::OutputBox);
    hInputBox = GetDlgItem(hWnd, Controls::InputBox);
    hScripts = GetDlgItem(hWnd, Controls::Scripts);
    hOutputText = GetDlgItem(hWnd, Controls::OutputText);
    hInputText = GetDlgItem(hWnd, Controls::InputText);

    SendMessage(hOutputBox, EM_SETREADONLY, (WPARAM)TRUE, 0);
    //SendMessage(hOutputBox, EM_SETBKGNDCOLOR, 0, (LPARAM)RGB(240, 240, 240));
    ExtraWindow::SetEditControlFontSize(hInputBox, 1.4f, true);
    ExtraWindow::SetEditControlFontSize(hOutputBox, 1.4f, true);
    int tabWidth = 16;
    SendMessage(hInputBox, EM_SETTABSTOPS, 1, (LPARAM)&tabWidth);
    SendMessage(hOutputBox, EM_SETTABSTOPS, 1, (LPARAM)&tabWidth);


    Lua.open_libraries(sol::lib::base, sol::lib::package);
    Lua.set_function("print", luaPrint);
    Lua.set_function("clear", clear);
    Lua.set_function("placeTerrain", placeTerrain);
    Lua.set_function("redrawWindow", redrawFA2Window);

    Update(hWnd);
}

void CLuaConsole::Close(HWND& hWnd)
{
    EndDialog(hWnd, NULL);
    ShowWindow(hWnd, SW_HIDE);

    CLuaConsole::m_hwnd = NULL;
    CLuaConsole::m_parent = NULL;
}

void CLuaConsole::Update(HWND& hWnd)
{
    while (SendMessage(hScripts, LB_DELETESTRING, 0, NULL) != CB_ERR);
  
    std::string scriptPath = CFinalSunApp::ExePath();
    scriptPath += "\\Scripts\\";
    if (fs::exists(scriptPath) && fs::is_directory(scriptPath)) {
        for (const auto& entry : fs::directory_iterator(scriptPath)) {
            if (entry.is_regular_file() && entry.path().extension().string() == ".lua") {
                SendMessage(hScripts, LB_ADDSTRING, 0, (LPARAM)(LPCSTR)entry.path().filename().string().c_str());
            }
        }
    }
}

BOOL CALLBACK CLuaConsole::DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        CLuaConsole::Initialize(hWnd);
        RECT rect;
        GetClientRect(hWnd, &rect);
        origWndWidth = rect.right - rect.left;
        origWndHeight = rect.bottom - rect.top;
        minSizeSet = false;
        return TRUE;
    }
    case WM_GETMINMAXINFO: {
        if (!minSizeSet) {
            int borderWidth = GetSystemMetrics(SM_CXBORDER);
            int borderHeight = GetSystemMetrics(SM_CYBORDER);
            int captionHeight = GetSystemMetrics(SM_CYCAPTION);
            minWndWidth = origWndWidth + 2 * borderWidth;
            minWndHeight = origWndHeight + captionHeight + 2 * borderHeight;
            minSizeSet = true;
        }
        MINMAXINFO* pMinMax = (MINMAXINFO*)lParam;
        pMinMax->ptMinTrackSize.x = minWndWidth;
        pMinMax->ptMinTrackSize.y = minWndHeight;
        return TRUE;
    }
    case WM_SIZE: {
        int newWndWidth = LOWORD(lParam);
        int newWndHeight = HIWORD(lParam);

        int heightOffset = newWndHeight - origWndHeight;
        int heightOffsetOutput = heightOffset / 2;
        int heightOffsetInput = heightOffset - heightOffsetOutput;

        RECT rect;
        GetWindowRect(hOutputBox, &rect);
        POINT topLeft = { rect.left, rect.top };
        ScreenToClient(hWnd, &topLeft);

        int newWidth = rect.right - rect.left + newWndWidth - origWndWidth;
        int newHeight = rect.bottom - rect.top + heightOffsetOutput;
        MoveWindow(hOutputBox, topLeft.x, topLeft.y, newWidth, newHeight, TRUE);

        GetWindowRect(hInputBox, &rect);
        topLeft = { rect.left, rect.top };
        ScreenToClient(hWnd, &topLeft);
        newWidth = rect.right - rect.left + newWndWidth - origWndWidth;
        newHeight = rect.bottom - rect.top + heightOffsetInput;
        MoveWindow(hInputBox, topLeft.x, topLeft.y + heightOffsetOutput, newWidth, newHeight, TRUE);

        GetWindowRect(hInputText, &rect);
        topLeft = { rect.left, rect.top };
        ScreenToClient(hWnd, &topLeft);
        newWidth = rect.right - rect.left;
        newHeight = rect.bottom - rect.top;
        MoveWindow(hInputText, topLeft.x, topLeft.y + heightOffsetOutput, newWidth, newHeight, TRUE);

        GetWindowRect(hScripts, &rect);
        topLeft = { rect.left, rect.top };
        ScreenToClient(hWnd, &topLeft);
        newWidth = rect.right - rect.left + newWndWidth - origWndWidth;
        newHeight = rect.bottom - rect.top;
        MoveWindow(hScripts, topLeft.x, topLeft.y, newWidth, newHeight, TRUE);

        GetWindowRect(hRun, &rect);
        topLeft = { rect.left, rect.top };
        ScreenToClient(hWnd, &topLeft);
        newWidth = rect.right - rect.left;
        newHeight = rect.bottom - rect.top;
        MoveWindow(hRun, topLeft.x + newWndWidth - origWndWidth, topLeft.y, newWidth, newHeight, TRUE);

        GetWindowRect(hExecute, &rect);
        topLeft = { rect.left, rect.top };
        ScreenToClient(hWnd, &topLeft);
        newWidth = rect.right - rect.left;
        newHeight = rect.bottom - rect.top;
        MoveWindow(hExecute, topLeft.x + newWndWidth - origWndWidth, topLeft.y, newWidth, newHeight, TRUE);

        origWndWidth = newWndWidth;
        origWndHeight = newWndHeight;
        break;
    }
    case WM_COMMAND:
    {
        WORD ID = LOWORD(wParam);
        WORD CODE = HIWORD(wParam);
        switch (ID)
        {
        case Controls::Run:
            OnClickRun(false);
            break;
        case Controls::Execute:
            OnClickRun(true);
            break;
        default:
            break;
        }
        break;
    }
    case WM_CLOSE:
    {
        CLuaConsole::Close(hWnd);
        return TRUE;
    }
    case 114514: // used for update
    {
        Update(hWnd);
        return TRUE;
    }
    }

    // Process this message through default handler
    return FALSE;
}

void CLuaConsole::OnClickRun(bool fromFile)
{
    std::string script;
    if (fromFile)
    {
        std::string scriptPath = CFinalSunApp::ExePath();
        scriptPath += "\\Scripts\\";
        int idx = SendMessage(hScripts, LB_GETCURSEL, NULL, NULL);
        char fileName[260]{ 0 };
        SendMessage(hScripts, LB_GETTEXT, idx, (LPARAM)fileName);
        scriptPath += fileName;
        std::ifstream file(scriptPath);
        if (file.fail())
            return;
        std::stringstream buffer;
        buffer << file.rdbuf();
        script = buffer.str();
    }
    else
    {
        GetWindowText(hInputBox, Buffer, BUFFER_SIZE);
        script = Buffer;
    }

    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    char* dt = ctime(&now_c);
    auto timeStart = duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::ostringstream oss;
    oss << "==============================\r\n   Lua Script Console for FA2SPHE\r\n   Time: " << dt << "   ==============================\r\n";
    std::string text = oss.str();

    writeLuaConsole(text);
    try {
        sol::protected_function_result result = Lua.script(script, sol::script_pass_on_error);
        if (!result.valid()) {
            sol::error err = result;
            std::string errorMessage = "Lua Error: " + std::string(err.what()) + "\r\n";
            writeLuaConsole(errorMessage);
        }
        else
        {
            oss.str("");
            auto timeEnd = duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            oss << "Successfully executed script.\r\n   Elapsed Time: " << timeEnd - timeStart << " ms.\r\n";
            text = oss.str();
            writeLuaConsole(text);
        }
    }
    catch (const std::exception& e) {
        std::string errorMessage = "Critical Error: " + std::string(e.what()) + "\r\n";
        writeLuaConsole(errorMessage);
    }
}
