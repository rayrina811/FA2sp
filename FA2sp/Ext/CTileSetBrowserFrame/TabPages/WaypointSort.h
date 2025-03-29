#pragma once

#include "../Body.h"

#include <map>
#include <vector>
#include <CIsoView.h>

class WaypointSort
{
public:
    static WaypointSort Instance;

    WaypointSort() : m_hWnd{ NULL } {}

    enum class MenuItem : int
    {
        AddTrigger = 0x2000,
        Refresh = 0x3000
    };

    void LoadAllTriggers();
    void Clear();
    BOOL OnNotify(LPNMTREEVIEW lpNmhdr);
    BOOL OnMessage(PMSG pMsg);
    void Create(HWND hParent);
    void OnSize() const;
    void ShowWindow(bool bShow) const;
    void ShowWindow() const;
    void HideWindow() const;
    void ShowMenu(POINT pt) const;
    bool IsValid() const;
    bool IsVisible() const;
    void DeleteTrigger(ppmfc::CString triggerId, HTREEITEM hItemParent = TVI_ROOT) const;
    void AddTrigger(ppmfc::CString triggerId, ppmfc::CString x, ppmfc::CString y) const;
    const ppmfc::CString& GetCurrentPrefix() const;
    HWND GetHwnd() const;
    operator HWND() const;

private:
    HTREEITEM FindLabel(HTREEITEM hItemParent, LPCSTR pszLabel) const;
    std::vector<ppmfc::CString> GetGroup(ppmfc::CString triggerId, ppmfc::CString& name) const;
    static std::vector<ppmfc::CString> TreeViewTexts;
    static std::vector<std::vector<ppmfc::CString>> TreeViewTextsVector;

private:
    HWND m_hWnd;
    ppmfc::CString m_strPrefix;
};