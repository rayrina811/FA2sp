#include "TriggerSort.h"

#include "../../../FA2sp.h"
#include "../../../Helpers/STDHelpers.h"

#include <CFinalSunDlg.h>
#include "TeamSort.h"
#include "WaypointSort.h"
#include "../../../ExtraWindow/CNewTrigger/CNewTrigger.h"
#include "../../../Helpers/Translations.h"
#include "../../CMapData/Body.h"
using namespace std;

TriggerSort TriggerSort::Instance;
std::map<ppmfc::CString, ppmfc::CString> TriggerSort::TriggerTags;
std::map<ppmfc::CString, std::vector<ppmfc::CString>> TriggerSort::TriggerTagsParent;
std::vector<ppmfc::CString> TriggerSort::attachedTriggers;
std::vector<ppmfc::CString> TriggerSort::TreeViewTexts;
std::vector<std::vector<ppmfc::CString>> TriggerSort::TreeViewTextsVector;

void TriggerSort::LoadAllTriggers()
{
    ExtConfigs::InitializeMap = false;
    this->Clear();

    TriggerTags.clear();
    TriggerTagsParent.clear();
    TreeViewTexts.clear();
    TreeViewTextsVector.clear();
    for (auto& triggerPair : CMapDataExt::Triggers)
    {
        auto& trigger = triggerPair.second;

        if (trigger->AttachedTrigger != "<none>")
        {
            if (auto atri = CMapDataExt::GetTrigger(trigger->AttachedTrigger))
            {
                TriggerTags[trigger->ID] = trigger->AttachedTrigger;
            }
            TriggerTagsParent[trigger->AttachedTrigger].push_back(trigger->ID);
        }
    }
    for (auto& triggerPair : CMapDataExt::Triggers)
    {
        auto& trigger = triggerPair.second;
        this->AddTrigger(trigger->ID);
    }
    ExtConfigs::InitializeMap = true;
}

void TriggerSort::Clear()
{
    TreeView_DeleteAllItems(this->GetHwnd());
}

BOOL TriggerSort::OnNotify(LPNMTREEVIEW lpNmTreeView)
{
    switch (lpNmTreeView->hdr.code)
    {
    case TVN_SELCHANGED:
        if (auto pID = reinterpret_cast<const char*>(lpNmTreeView->itemNew.lParam))
        {
            if (strlen(pID) && ExtConfigs::InitializeMap)
            {
                if (IsWindowVisible(CNewTrigger::GetHandle()))
                {
                    auto pStr = CINI::CurrentDocument->GetString("Triggers", pID);
                    auto results = STDHelpers::SplitString(pStr);
                    if (results.size() <= 3)
                        return FALSE;
                    pStr = results[2];
                    //if (ExtConfigs::DisplayTriggerID)
                    {
                        ppmfc::CString tmp;
                        pStr.Format("%s (%s)", pID, pStr);
                    }
                    auto idx = SendMessage(CNewTrigger::hSelectedTrigger, CB_FINDSTRINGEXACT, 0, (LPARAM)pStr.m_pchData);
                    if (idx == CB_ERR)
                        return FALSE;

                    SendMessage(CNewTrigger::hSelectedTrigger, CB_SETCURSEL, idx, NULL);
                    CNewTrigger::OnSelchangeTrigger();
                    return TRUE;
                }
                else
                    return FALSE;
            }
        }
        break;
    default:
        break;
    }

    return FALSE;
}

BOOL TriggerSort::OnMessage(PMSG pMsg)
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

void TriggerSort::Create(HWND hParent)
{
    RECT rect;
    ::GetClientRect(hParent, &rect);

    this->m_hWnd = CreateWindowEx(NULL, "SysTreeView32", nullptr,
        WS_CHILD | WS_BORDER | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | 
        TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
        rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, hParent,
        NULL, static_cast<HINSTANCE>(FA2sp::hInstance), nullptr);
}

void TriggerSort::OnSize() const
{
    RECT rect;
    ::GetClientRect(::GetParent(this->GetHwnd()), &rect);
    ::MoveWindow(this->GetHwnd(), 2, 29, rect.right - rect.left - 6, rect.bottom - rect.top - 35, FALSE);
}

void TriggerSort::ShowWindow(bool bShow) const
{
    ::ShowWindow(this->GetHwnd(), bShow ? SW_SHOW : SW_HIDE);
}

void TriggerSort::ShowWindow() const
{
    this->ShowWindow(true);
}

void TriggerSort::HideWindow() const
{
    this->ShowWindow(false);
}

void TriggerSort::ShowMenu(POINT pt) const
{
    HMENU hPopupMenu = ::CreatePopupMenu();
    ::AppendMenu(hPopupMenu, MF_STRING, (UINT_PTR)MenuItem::Refresh, Translations::TranslateOrDefault("Refresh", "Refresh"));
    ::TrackPopupMenu(hPopupMenu, TPM_VERTICAL | TPM_HORIZONTAL, pt.x, pt.y, NULL, this->GetHwnd(), nullptr);
}

bool TriggerSort::IsValid() const
{
    return this->GetHwnd() != NULL;
}

bool TriggerSort::IsVisible() const
{
    return this->IsValid() && ::IsWindowVisible(this->m_hWnd);
}

void TriggerSort::Menu_AddTrigger()
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

const ppmfc::CString& TriggerSort::GetCurrentPrefix() const
{
    return this->m_strPrefix;
}

HWND TriggerSort::GetHwnd() const
{
    return this->m_hWnd;
}

TriggerSort::operator HWND() const
{
    return this->GetHwnd();
}

HTREEITEM TriggerSort::FindLabel(HTREEITEM hItemParent, LPCSTR pszLabel) const
{
    TVITEM tvi;
    char chLabel[0x200] = { 0 };

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
void TriggerSort::AddAttachedTrigger(HTREEITEM hParent, ppmfc::CString triggerID, ppmfc::CString parentName) const
{
    for (auto id : attachedTriggers)
    {
        if (TriggerTags[triggerID] == id)
        {
            if (HTREEITEM hNode = this->FindLabel(hParent, parentName))
            {
                ppmfc::CString pTrigger2 = Translations::TranslateOrDefault("Sort.DetectedLoopedTrigger", "Detected Looped Trigger!");
                TreeViewTexts.push_back(pTrigger2);
                hParent = hNode;
                TVINSERTSTRUCT tvis;
                tvis.hInsertAfter = TVI_SORT;
                tvis.hParent = hParent;
                tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
                tvis.item.pszText = TreeViewTexts.back().m_pchData;
                tvis.item.lParam = (LPARAM)TreeViewTexts.back().m_pchData;
                TreeView_InsertItem(this->GetHwnd(), &tvis);

                return;
            }
        }
    }
    
    if (TriggerTags[triggerID] != "")
        if (HTREEITEM hNode = this->FindLabel(hParent, parentName))
        {
            
            auto pTrigger2 = CINI::CurrentDocument->GetString("Triggers", TriggerTags[triggerID], "");
            auto RET2 = STDHelpers::SplitString(pTrigger2);
            if (RET2.size() > 2)
            {
                pTrigger2 = RET2[2];

                ppmfc::CString pszText = ppmfc::CString(Translations::TranslateOrDefault("Sort.AttachedTrigger", "Attached Trigger:")) + " " + pTrigger2 + " (" + TriggerTags[triggerID] + ")";
                TreeViewTexts.push_back(pszText);

                hParent = hNode;
                TVINSERTSTRUCT tvis;
                tvis.hInsertAfter = TVI_SORT;
                tvis.hParent = hParent;
                tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
                tvis.item.pszText = TreeViewTexts.back().m_pchData;
                tvis.item.lParam = (LPARAM)TriggerTags[triggerID].m_pchData;
                TreeView_InsertItem(this->GetHwnd(), &tvis);


                ppmfc::CString element = TriggerTags[triggerID];
                attachedTriggers.push_back(element);
                AddAttachedTrigger(hParent, TriggerTags[triggerID], TreeViewTexts.back());

            }

        }
}

void TriggerSort::AddAttachedTriggerReverse(HTREEITEM hParent, ppmfc::CString triggerID, ppmfc::CString parentName) const
{
    auto hParent2 = hParent;
    if (TriggerTagsParent[triggerID].size() > 0)
        for (auto& parentTrigger : TriggerTagsParent[triggerID])
        {
            if (HTREEITEM hNode = this->FindLabel(hParent2, parentName))
            {
                for (auto id : attachedTriggers)
                {
                    if (parentTrigger == id)
                    {
                        if (HTREEITEM hNode = this->FindLabel(hParent, parentName))
                        {
                            ppmfc::CString pTrigger2 = Translations::TranslateOrDefault("Sort.DetectedLoopedTrigger", "Detected Looped Trigger!");
                            TreeViewTexts.push_back(pTrigger2);
                            hParent = hNode;
                            TVINSERTSTRUCT tvis;
                            tvis.hInsertAfter = TVI_SORT;
                            tvis.hParent = hParent;
                            tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
                            tvis.item.pszText = TreeViewTexts.back().m_pchData;
                            tvis.item.lParam = (LPARAM)TreeViewTexts.back().m_pchData;
                            TreeView_InsertItem(this->GetHwnd(), &tvis);

                            return;
                        }
                    }

                }

                auto pTrigger2 = CINI::CurrentDocument->GetString("Triggers", parentTrigger, "");
                auto RET2 = STDHelpers::SplitString(pTrigger2);
                if (RET2.size() > 2)
                {
                    pTrigger2 = RET2[2];

                    ppmfc::CString pszText = ppmfc::CString(Translations::TranslateOrDefault("Sort.TriggerAttachedTo", "Trigger Attached To:")) + " " + pTrigger2 + " (" + parentTrigger + ")";
                    TreeViewTexts.push_back(pszText);

                    hParent = hNode;
                    TVINSERTSTRUCT tvis;
                    tvis.hInsertAfter = TVI_SORT;
                    tvis.hParent = hParent;
                    tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
                    tvis.item.pszText = TreeViewTexts.back().m_pchData;
                    tvis.item.lParam = (LPARAM)parentTrigger.m_pchData;
                    TreeView_InsertItem(this->GetHwnd(), &tvis);

                    ppmfc::CString element = parentTrigger;
                    attachedTriggers.push_back(element);

                    AddAttachedTriggerReverse(hParent, parentTrigger, TreeViewTexts.back());
                }
            }
        }


}


std::vector<ppmfc::CString> TriggerSort::GetGroup(ppmfc::CString triggerId, ppmfc::CString& name) const
{
    auto pSrc = CINI::CurrentDocument->GetString("Triggers", triggerId, "");

    auto ret = STDHelpers::SplitString(pSrc, 2);
    pSrc = ret[2];
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

void TriggerSort::AddTrigger(std::vector<ppmfc::CString> group, ppmfc::CString name, ppmfc::CString id) const
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

        if (ExtConfigs::TriggerList_AttachedTriggers)
        if (HTREEITEM hNode = this->FindLabel(hParent, text))
        {
            auto hParent2 = hParent;
            attachedTriggers.clear();
            ppmfc::CString element = id;
            attachedTriggers.push_back(element);
            AddAttachedTrigger(hParent2, element, text);

            hParent2 = hParent;
            attachedTriggers.clear();
            element = id;
            attachedTriggers.push_back(element);
            AddAttachedTriggerReverse(hParent2, element, text);
        }
    }
}

void TriggerSort::AddTrigger(ppmfc::CString triggerId) const
{
    if (this->IsVisible())
    {
        ppmfc::CString name;
        auto group = this->GetGroup(triggerId, name);
        this->AddTrigger(group, name, triggerId);
    }
}

void TriggerSort::DeleteTrigger(ppmfc::CString triggerId, HTREEITEM hItemParent) const
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

// just mannual update
//DEFINE_HOOK(4FA450, CTriggerFrame_Update_TriggerSort, 7)
//{
//    
    //if (TriggerSort::Instance.IsVisible())
    //{
    //    TriggerSort::Instance.LoadAllTriggers();
    //}
    //    
    //if (TeamSort::Instance.IsVisible())
    //{
    //    TeamSort::Instance.LoadAllTriggers();
    //}
    //if (WaypointSort::Instance.IsVisible())
    //{
    //    WaypointSort::Instance.LoadAllTriggers();
    //}

//    return 0;
//}