#pragma once

#include "../Body.h"

#include <map>
#include <vector>

class TriggerSort
{
public:
    static TriggerSort Instance;

    static std::map<ppmfc::CString, ppmfc::CString>TriggerTags;
    static std::map<ppmfc::CString, std::vector<ppmfc::CString>>TriggerTagsParent;

    TriggerSort() : m_hWnd{ NULL } {}

    enum class MenuItem : int
    {
        AddTrigger = 0x1000,
        Refresh = 0x2000
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
    void Menu_AddTrigger();
    void DeleteTrigger(ppmfc::CString triggerId, HTREEITEM hItemParent = TVI_ROOT) const;
    void AddTrigger(ppmfc::CString triggerId) const;
    const ppmfc::CString& GetCurrentPrefix() const;
    HWND GetHwnd() const;
    operator HWND() const;

    static std::vector<ppmfc::CString>attachedTriggers;
    static std::vector<ppmfc::CString>TreeViewTexts;
    static std::vector< std::vector<ppmfc::CString>>TreeViewTextsVector;

private:
    HTREEITEM FindLabel(HTREEITEM hItemParent, LPCSTR pszLabel) const;
    std::vector<ppmfc::CString> GetGroup(ppmfc::CString triggerId, ppmfc::CString& name) const;
    void AddTrigger(std::vector<ppmfc::CString> group, ppmfc::CString name, ppmfc::CString id) const;
    void AddAttachedTrigger(HTREEITEM hParent, ppmfc::CString triggerID, ppmfc::CString parentName) const;
    void AddAttachedTriggerReverse(HTREEITEM hParent, ppmfc::CString triggerID, ppmfc::CString parentName) const;

private:
    HWND m_hWnd;
    ppmfc::CString m_strPrefix;
};