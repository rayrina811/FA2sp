#include "TaskforceSort.h"

#include "../../../FA2sp.h"
#include "../../../Helpers/STDHelpers.h"

#include <CFinalSunDlg.h>
#include "../../../ExtraWindow/CNewTeamTypes/CNewTeamTypes.h"
#include "../../../ExtraWindow/CNewTaskforce/CNewTaskforce.h"
#include "../../../Helpers/Translations.h"
TaskforceSort TaskforceSort::Instance;
std::vector<ppmfc::CString> TaskforceSort::TreeViewTexts;
std::vector<std::vector<ppmfc::CString>> TaskforceSort::TreeViewTextsVector;

void TaskforceSort::LoadAllTriggers()
{
    ExtConfigs::InitializeMap = false;
    this->Clear();
    TreeViewTexts.clear();
    TreeViewTextsVector.clear();
    // TODO : 
    // Optimisze the efficiency
    if (auto pSection = CINI::CurrentDocument->GetSection("TaskForces"))
    {
        for (auto& pair : pSection->GetEntities())
        {
            this->AddTrigger(pair.second);
        }
    }
    ExtConfigs::InitializeMap = true;
}

void TaskforceSort::Clear()
{
    TreeView_DeleteAllItems(this->GetHwnd());
}

BOOL TaskforceSort::OnNotify(LPNMTREEVIEW lpNmTreeView)
{
    switch (lpNmTreeView->hdr.code)
    {
    case TVN_SELCHANGED:
        if (auto pID = reinterpret_cast<const char*>(lpNmTreeView->itemNew.lParam))
        {
            
            if (strlen(pID) && ExtConfigs::InitializeMap)
            {
                bool Success = false;
                if (IsWindowVisible(CNewTaskforce::GetHandle()))
                {
                    auto pStr = CINI::CurrentDocument->GetString(pID, "Name");
                    ppmfc::CString space1 = " (";
                    ppmfc::CString space2 = ")";

                    int idx = SendMessage(CNewTaskforce::hSelectedTaskforce, CB_FINDSTRINGEXACT, 0, (LPARAM)(pID + space1 + pStr + space2).m_pchData);
                    if (idx != CB_ERR)
                    {
                        SendMessage(CNewTaskforce::hSelectedTaskforce, CB_SETCURSEL, idx, NULL);
                        CNewTaskforce::OnSelchangeTaskforce();
                        Success = true;
                    }
                }
                //else if (IsWindowVisible(CNewTeamTypes::GetHandle()))
                //{
                //    auto pStr = CINI::CurrentDocument->GetString(pID, "Name");
                //    ppmfc::CString space1 = " (";
                //    ppmfc::CString space2 = ")";
                //
                //    int idx = SendMessage(CNewTeamTypes::hTaskforce, CB_FINDSTRINGEXACT, 0, (LPARAM)(pID + space1 + pStr + space2).m_pchData);
                //    if (idx != CB_ERR)
                //    {
                //        SendMessage(CNewTeamTypes::hTaskforce, CB_SETCURSEL, idx, NULL);
                //        CNewTeamTypes::OnSelchangeTaskForce();
                //        Success = true;
                //    }
                //    Success = true;
                //}
                return Success;
            }
        }
        break;
    default:
        break;
    }

    return FALSE;
}

BOOL TaskforceSort::OnMessage(PMSG pMsg)
{
    switch (pMsg->message)
    {
    case WM_RBUTTONDOWN:
        this->ShowMenu(pMsg->pt);
        return TRUE;
    default:
        break;
    }

    return FALSE;
}

void TaskforceSort::Create(HWND hParent)
{
    RECT rect;
    ::GetClientRect(hParent, &rect);

    this->m_hWnd = CreateWindowEx(NULL, "SysTreeView32", nullptr,
        WS_CHILD | WS_BORDER | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | 
        TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
        rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, hParent,
        NULL, static_cast<HINSTANCE>(FA2sp::hInstance), nullptr);
}

void TaskforceSort::OnSize() const
{
    RECT rect;
    ::GetClientRect(::GetParent(this->GetHwnd()), &rect);
    ::MoveWindow(this->GetHwnd(), 2, 29, rect.right - rect.left - 6, rect.bottom - rect.top - 35, FALSE);
}

void TaskforceSort::ShowWindow(bool bShow) const
{
    ::ShowWindow(this->GetHwnd(), bShow ? SW_SHOW : SW_HIDE);
}

void TaskforceSort::ShowWindow() const
{
    this->ShowWindow(true);
}

void TaskforceSort::HideWindow() const
{
    this->ShowWindow(false);
}

void TaskforceSort::ShowMenu(POINT pt) const
{
    HMENU hPopupMenu = ::CreatePopupMenu();
    ::AppendMenu(hPopupMenu, MF_STRING, (UINT_PTR)MenuItem::Refresh, Translations::TranslateOrDefault("Refresh", "Refresh"));
    ::TrackPopupMenu(hPopupMenu, TPM_VERTICAL | TPM_HORIZONTAL, pt.x, pt.y, NULL, this->GetHwnd(), nullptr);
}

bool TaskforceSort::IsValid() const
{
    return this->GetHwnd() != NULL;
}

bool TaskforceSort::IsVisible() const
{
    return this->IsValid() && ::IsWindowVisible(this->m_hWnd);
}

void TaskforceSort::Menu_AddTrigger()
{
    HTREEITEM hItem = TreeView_GetSelection(this->GetHwnd());
    ppmfc::CString prefix = "";
    if (hItem != NULL)
    {
        const char* pID = nullptr;
        while (true)
        {
            TVITEM tvi;
            tvi.hItem = hItem;
            TreeView_GetItem(this->GetHwnd(), &tvi);
            if (pID = reinterpret_cast<const char*>(tvi.lParam))
                break;
            hItem = TreeView_GetChild(this->GetHwnd(), hItem);
            if (hItem == NULL)
            {
                this->m_strPrefix = prefix;
                return;
            }
        }

        ppmfc::CString buffer;
        prefix += "[";
        for (auto group : this->GetGroup(pID, buffer))
            prefix += group + ".";
        if (prefix[prefix.GetLength() - 1] == '.')
        {
            prefix.SetAt(prefix.GetLength() - 1, ']');
            if (prefix.GetLength() == 2)
                prefix = "";
        }
        else
            prefix = "";
    }
    this->m_strPrefix = prefix;
}

const ppmfc::CString& TaskforceSort::GetCurrentPrefix() const
{
    return this->m_strPrefix;
}

HWND TaskforceSort::GetHwnd() const
{
    return this->m_hWnd;
}

TaskforceSort::operator HWND() const
{
    return this->GetHwnd();
}

HTREEITEM TaskforceSort::FindLabel(HTREEITEM hItemParent, LPCSTR pszLabel) const
{
    TVITEM tvi;
    char chLabel[0x200];

    for (tvi.hItem = TreeView_GetChild(this->GetHwnd(), hItemParent); tvi.hItem;
        tvi.hItem = TreeView_GetNextSibling(this->GetHwnd(), tvi.hItem))
    {
        tvi.mask = TVIF_TEXT | TVIF_CHILDREN;
        tvi.pszText = chLabel;
        tvi.cchTextMax = _countof(chLabel);
        if (TreeView_GetItem(this->GetHwnd(), &tvi))
        {
            if (strcmp(tvi.pszText, pszLabel) == 0)
                return tvi.hItem;
            if (tvi.cChildren)
            {
                HTREEITEM hChildSearch = this->FindLabel(tvi.hItem, pszLabel);
                if (hChildSearch) 
                    return hChildSearch;
            }
        }
    }
    return NULL;
}

std::vector<ppmfc::CString> TaskforceSort::GetGroup(ppmfc::CString triggerId, ppmfc::CString& name) const
{
    //dont change this
    auto name2 = std::string(CINI::CurrentDocument->GetString(triggerId, "Name", ""));
    ppmfc::CString pSrc = name2.c_str();

    auto ret = std::vector<ppmfc::CString>{};
    //pSrc = ret[2];
    int nStart = pSrc.Find('[');
    int nEnd = pSrc.Find(']');
    if (nStart < nEnd && nStart == 0)
    {
        name = pSrc.Mid(nEnd + 1);
        pSrc = pSrc.Mid(nStart + 1, nEnd - nStart - 1);
        ret = STDHelpers::SplitString(pSrc, ".");
        return ret;
    }
    else
        name = pSrc;
    
    ret.clear();
    return ret;
}


void TaskforceSort::AddTrigger(std::vector<ppmfc::CString> group, ppmfc::CString name, ppmfc::CString id) const
{
    TreeViewTextsVector.push_back(group);
    HTREEITEM hParent = TVI_ROOT;
    for (auto& node : TreeViewTextsVector.back())
    {
        if (HTREEITEM hNode = this->FindLabel(hParent, node))
        {
            hParent = hNode;
            continue;
        }
        else
        {
            TVINSERTSTRUCT tvis;
            tvis.hInsertAfter = TVI_SORT;
            tvis.hParent = hParent;
            tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
            tvis.item.lParam = NULL;
            tvis.item.pszText = node.m_pchData;
            hParent = TreeView_InsertItem(this->GetHwnd(), &tvis);
        }
    }

    if (HTREEITEM hNode = this->FindLabel(hParent, name))
    {
        TVITEM item;
        item.hItem = hNode;
        if (TreeView_GetItem(this->GetHwnd(), &item))
        {
            ppmfc::CString text = item.pszText;
            text += " (" + id + ")";
            TreeViewTexts.push_back(text);
            item.pszText = TreeViewTexts.back().m_pchData;
            TreeViewTexts.push_back(id);
            item.lParam = (LPARAM)TreeViewTexts.back().m_pchData;
            TreeView_SetItem(this->GetHwnd(), &item);
        }
    }
    else
    {
        TVINSERTSTRUCT tvis;
        tvis.hInsertAfter = TVI_SORT;
        tvis.hParent = hParent;
        tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
        ppmfc::CString text = name;
        text += " (" + id + ")";
        TreeViewTexts.push_back(text);
        tvis.item.pszText = TreeViewTexts.back().m_pchData;
        TreeViewTexts.push_back(id);
        tvis.item.lParam = (LPARAM)TreeViewTexts.back().m_pchData;
        TreeView_InsertItem(this->GetHwnd(), &tvis);
    }
}

void TaskforceSort::AddTrigger(ppmfc::CString triggerId) const
{
    if (this->IsVisible())
    {
        ppmfc::CString name;
        auto group = this->GetGroup(triggerId, name);

        this->AddTrigger(group, name, triggerId);
    }
}


void TaskforceSort::DeleteTrigger(ppmfc::CString triggerId, HTREEITEM hItemParent) const
{
    if (this->IsVisible())
    {
        TVITEM tvi;

        for (tvi.hItem = TreeView_GetChild(this->GetHwnd(), hItemParent); tvi.hItem;
            tvi.hItem = TreeView_GetNextSibling(this->GetHwnd(), tvi.hItem))
        {
            tvi.mask = TVIF_PARAM | TVIF_CHILDREN;
            if (TreeView_GetItem(this->GetHwnd(), &tvi))
            {
                if (tvi.lParam && strcmp((const char*)tvi.lParam, triggerId) == 0)
                {
                    TreeView_DeleteItem(this->GetHwnd(), tvi.hItem);
                    return;
                }
                if (tvi.cChildren)
                    this->DeleteTrigger(triggerId, tvi.hItem);
            }
        }
    }
}

