#include "Body.h"

#include <Helpers/Macro.h>

#include <CINI.h>
#include <CMapData.h>
#include <CIsoView.h>

#include <set>
#include <numeric>

#include "../CTileSetBrowserFrame/TabPages/TriggerSort.h"
#include "../../Helpers/STDHelpers.h"
#include "../../FA2sp.h"

DEFINE_HOOK(4FA450, CTriggerFrame_Update, 7)
{
    GET(CTriggerFrameExt*, pThis, ECX);

    int nFinalSel = CB_ERR;
    int nCurSel = pThis->CCBCurrentTrigger.GetCurSel();
    const char* ID = nullptr;
    if (nCurSel != CB_ERR)
        ID = reinterpret_cast<const char*>(pThis->CCBCurrentTrigger.GetItemDataPtr(nCurSel));

    pThis->CCBCurrentTrigger.DeleteAllStrings();

    pThis->CCBCurrentTrigger.SetWindowText("");

    if (auto pSection = CINI::CurrentDocument->GetSection("Triggers"))
    {
        std::vector<std::string> strings;
        std::vector<std::string> IDs;
        std::vector<LPTSTR> m_pchDatas;

        for (auto& pair : pSection->GetEntities())
        {
            auto splits = STDHelpers::SplitString(pair.second, 2);
            int nIdx;
            if (ExtConfigs::DisplayTriggerID)
            {
                strings.push_back(std::string(splits[2]));
                IDs.push_back(std::string(pair.first));
                m_pchDatas.push_back(pair.first.m_pchData);
            }
            else
            {
                nIdx = pThis->CCBCurrentTrigger.AddString(splits[2]);
                pThis->CCBCurrentTrigger.SetItemDataPtr(nIdx, pair.first.m_pchData);
            }
        }
        if (ExtConfigs::DisplayTriggerID)
        {
            std::vector<size_t> indices(strings.size());
            std::vector<std::string> sorted_strings = strings;
            std::iota(indices.begin(), indices.end(), 0);
            std::sort(indices.begin(), indices.end(), [&](size_t i1, size_t i2) {
                return sorted_strings[i1] < sorted_strings[i2];
                });
            for (size_t i = 0; i < strings.size(); ++i) 
            {
                ppmfc::CString tmp;
                tmp.Format("%s (%s)", IDs[indices[i]].c_str(), strings[indices[i]].c_str());

                pThis->CCBCurrentTrigger.InsertString((int)i, tmp);
                pThis->CCBCurrentTrigger.SetItemDataPtr((int)i, m_pchDatas[indices[i]]);
            }
        }
    }

    int nCount = pThis->CCBCurrentTrigger.GetCount();

    if (ID)
    {
        for (int i = 0; i < nCount; ++i)
        {
            if (strcmp(reinterpret_cast<const char*>(pThis->CCBCurrentTrigger.GetItemDataPtr(i)), ID) == 0)
            {
                nFinalSel = i;
                break;
            }
        }
    }

    if (nCurSel == 0) nCurSel = 1;
    if (nFinalSel < 0 && nCount > 0)
        nFinalSel = nCount - 1;

    pThis->CCBCurrentTrigger.SetCurSel(nFinalSel);
    pThis->OnCBCurrentTriggerSelectedChanged();

    return 0x4FAACF;
}

DEFINE_HOOK(4FBA10, CTriggerFrame_OnCBCurrentTriggerEditChanged, 6)
{
    GET(CTriggerFrameExt*, pThis, ECX);

    int nCurSel = pThis->CCBCurrentTrigger.GetCurSel();

    const char* TriggerID = "";
    if (nCurSel != CB_ERR)
    {
        if (auto ID = reinterpret_cast<const char*>(pThis->CCBCurrentTrigger.GetItemDataPtr(nCurSel)))
            TriggerID = ID;
    }

    pThis->TriggerOption.TriggerID = pThis->TriggerEvent.TriggerID = pThis->TriggerAction.TriggerID = TriggerID;
    
    if (pThis->TriggerOption.m_hWnd)
        pThis->TriggerOption.Update();
    if (pThis->TriggerEvent.m_hWnd)
        pThis->TriggerEvent.Update();
    if (pThis->TriggerAction.m_hWnd)
        pThis->TriggerAction.Update();
    
    return 0x4FBAD3;
}

DEFINE_HOOK(4FAAD0, CTriggerFrame_OnBNNewTriggerClicked, 7)
{
    GET(CTriggerFrameExt*, pThis, ECX);

    auto ID = CINI::GetAvailableIndex();
    auto Owner = CMapData::Instance->FindAvailableOwner(0, 1);
    ppmfc::CString Name;
    if (ExtConfigs::NewTriggerPlusID)
    {
        int plusID = atoi(ID) - 1000000;
        char temp2[512];
        sprintf(temp2, ExtConfigs::NewTriggerPlusID_Digits, plusID);
        ppmfc::CString nameplus = temp2;
        nameplus.TrimRight();
        Name =
            CTriggerFrameExt::CreateFromTriggerSort ?
            TriggerSort::Instance.GetCurrentPrefix() + "New Trigger " + nameplus :
            ppmfc::CString("New Trigger " + nameplus);
    }
    else
    {
        Name =
            CTriggerFrameExt::CreateFromTriggerSort ?
            TriggerSort::Instance.GetCurrentPrefix() + "New Trigger" :
            ppmfc::CString("New Trigger");
    }

    
    auto TriggerBuffer = Owner + ",<none>,"+ Name + ",0,1,1,1,0";

    CINI::CurrentDocument->WriteString("Triggers", ID, TriggerBuffer);
    CINI::CurrentDocument->WriteString("Events", ID, "0");
    CINI::CurrentDocument->WriteString("Actions", ID, "0");
    auto TagID = CINI::GetAvailableIndex();
    CINI::CurrentDocument->WriteString("Tags", TagID, "0," + Name + " 1," + ID);
    if (ExtConfigs::DisplayTriggerID)
    {
        if (auto pSection = CINI::CurrentDocument->GetSection("Triggers"))
        {
            std::vector<std::string> strings;
            std::vector<std::string> IDs;

            for (auto& pair : pSection->GetEntities())
            {
                auto splits = STDHelpers::SplitString(pair.second, 2);
                strings.push_back(std::string(splits[2]));
                IDs.push_back(std::string(pair.first));
            }
            strings.push_back(std::string(Name));
            IDs.push_back(std::string(ID));

            std::vector<size_t> indices(strings.size());
            std::vector<std::string> sorted_strings = strings;
            std::iota(indices.begin(), indices.end(), 0);
            std::sort(indices.begin(), indices.end(), [&](size_t i1, size_t i2) {
                return sorted_strings[i1] < sorted_strings[i2];
                });
            for (size_t i = 0; i < strings.size(); ++i)
            {
                if (ID == IDs[indices[i]].c_str())
                {
                    ppmfc::CString tmp;
                    tmp.Format("%s (%s)", ID, Name);

                    pThis->CCBCurrentTrigger.InsertString(i, tmp);
                    pThis->CCBCurrentTrigger.SetItemDataPtr(i, ID.m_pchData);
                    pThis->CCBCurrentTrigger.SetCurSel(i);
                    break;
                }
            }

        }
    }
    else
    {
        int nIndex = pThis->CCBCurrentTrigger.AddString(Name);
        pThis->CCBCurrentTrigger.SetItemDataPtr(nIndex, ID.m_pchData);
        pThis->CCBCurrentTrigger.SetCurSel(nIndex);
    }

    pThis->OnCBCurrentTriggerSelectedChanged();

    TriggerSort::Instance.AddTrigger(ID);

    return 0x4FB1AD;
}

DEFINE_HOOK(4FB1B0, CTriggerFrame_OnBNDelTriggerClicked, 6)
{
    GET(CTriggerFrameExt*, pThis, ECX);

    int nCurSel = pThis->CCBCurrentTrigger.GetCurSel();
    if (nCurSel != CB_ERR)
    {
        if (auto ID = reinterpret_cast<const char*>(pThis->CCBCurrentTrigger.GetItemDataPtr(nCurSel)))
        {
            const char* pMessage =
                "删除触发和关联的标签，请点击是。\n"
                "删除触发但保留标签，请点击否。\n"
                "如果误触了，点击「取消」撤销本次删除操作。\n"
                "注意：选择“是”时，如果地图上有关联的单元标记，它们也会被删除。";

            int nResult = pThis->MessageBox(pMessage, "删除触发", MB_YESNOCANCEL);
            if (nResult == IDYES || nResult == IDNO)
            {
                TriggerSort::Instance.DeleteTrigger(ID);
                CINI::CurrentDocument->DeleteKey("Triggers", ID);
                CINI::CurrentDocument->DeleteKey("Events", ID);
                CINI::CurrentDocument->DeleteKey("Actions", ID);
                if (nResult == IDYES)
                {
                    if (auto pTagsSection = CINI::CurrentDocument->GetSection("Tags"))
                    {
                        std::set<ppmfc::CString> TagsToRemove;
                        for (auto& pair : pTagsSection->GetEntities())
                        {
                            auto splits = STDHelpers::SplitString(pair.second, 2);
                            if (strcmp(splits[2], ID) == 0)
                                TagsToRemove.insert(pair.first);
                        }
                        for (auto& tag : TagsToRemove)
                            CINI::CurrentDocument->DeleteKey("Tags", tag);

                        if (auto pCellTagsSection = CINI::CurrentDocument->GetSection("CellTags"))
                        {
                            std::vector<ppmfc::CString> CellTagsToRemove;
                            for (auto& pair : pCellTagsSection->GetEntities())
                            {
                                if (TagsToRemove.find(pair.second) != TagsToRemove.end())
                                    CellTagsToRemove.push_back(pair.first);
                            }
                            for (auto& celltag : CellTagsToRemove)
                            {
                                CINI::CurrentDocument->DeleteKey("CellTags", celltag);
                                int nCoord = atoi(celltag);
                                int nMapCoord = CMapData::Instance->GetCoordIndex(nCoord % 1000, nCoord / 1000);
                                CMapData::Instance->CellDatas[nMapCoord].CellTag = -1;
                            }
                            CMapData::Instance->UpdateFieldCelltagData(FALSE); //important
                            ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
                        }
                        
                    }
                    
                }
                pThis->CCBCurrentTrigger.DeleteString(nCurSel);
                if (--nCurSel >= 0)
                {
                    pThis->CCBCurrentTrigger.SetCurSel(nCurSel);
                    pThis->OnCBCurrentTriggerSelectedChanged();
                }
            }
        }
    }

    return 0x4FB9F3;
}

DEFINE_HOOK(4FBD10, CTriggerFrame_OnBNPlaceOnMapClicked, 6)
{
    GET(CTriggerFrameExt*, pThis, ECX);

    int nCurSel = pThis->CCBCurrentTrigger.GetCurSel();
    if (nCurSel != CB_ERR)
    {
        if (auto ID = reinterpret_cast<const char*>(pThis->CCBCurrentTrigger.GetItemDataPtr(nCurSel)))
        {
            if (auto pTagsSection = CINI::CurrentDocument->GetSection("Tags"))
            {
                for (auto& pair : pTagsSection->GetEntities())
                {
                    auto splits = STDHelpers::SplitString(pair.second, 2);
                    if (strcmp(splits[2], ID) == 0)
                    {
                        CIsoView::CurrentCommand->Command = 4;
                        CIsoView::CurrentCommand->Type = 4;
                        CIsoView::CurrentCommand->ObjectID = pair.first;
                        break;
                    }
                }
            }
        }
    }

    return 0x4FC17B;
}

DEFINE_HOOK(4FC180, CTriggerFrame_OnBNCloneTriggerClicked, 6)
{
    GET(CTriggerFrameExt*, pThis, ECX);

    int nCurSel = pThis->CCBCurrentTrigger.GetCurSel();
    if (nCurSel != CB_ERR)
    {
        if (auto CurrentID = reinterpret_cast<const char*>(pThis->CCBCurrentTrigger.GetItemDataPtr(nCurSel)))
        {
            auto buffer = CINI::CurrentDocument->GetString("Triggers", CurrentID);
            auto splits = STDHelpers::SplitString(buffer, 7);
            auto& Name = splits[2];
            auto NewID = CINI::GetAvailableIndex();

            ppmfc::CString newName;
            if (ExtConfigs::CloneWithOrderedID)
            {
                const char* splitter = " ";
                auto splitN = STDHelpers::SplitString(Name, splitter);
                auto lastS = std::string(splitN.back());

                if (STDHelpers::is_number(lastS))
                {
                    int lastID = std::stoi(lastS);
                    
                    for (size_t i = 0; i < splitN.size() - 1; ++i) {
                        auto sps = splitN[i];
                        newName += sps + splitter;
                    }
                    char temp_char[512];
                    sprintf(temp_char, ExtConfigs::CloneWithOrderedID_Digits, (lastID + 1));
                    newName += temp_char;
                }
                else
                {
                    const char* splitter2 = "-";
                    splitN = STDHelpers::SplitString(Name, splitter2);
                    lastS = std::string(splitN.back());
                    if (STDHelpers::is_number(lastS))
                    {
                        int lastID = std::stoi(lastS);

                        for (size_t i = 0; i < splitN.size() - 1; ++i) {
                            auto sps = splitN[i];
                            newName += sps + splitter2;
                        }
                        char temp_char[512];
                        sprintf(temp_char, ExtConfigs::CloneWithOrderedID_Digits, (lastID + 1));
                        newName += temp_char;
                    }
                    else
                    {
                        char temp_char[512];
                        sprintf(temp_char, ExtConfigs::CloneWithOrderedID_Digits, 2);
                        newName = Name + " " + temp_char;
                    }
                }
            }
            else
                newName = Name + " Clone";
                

            buffer.Format("%s,%s,%s,%s,%s,%s,%s",
                splits[0], splits[1], newName, splits[3],
                splits[4], splits[5], splits[6]);

            CINI::CurrentDocument->WriteString("Triggers", NewID, buffer);
            CINI::CurrentDocument->WriteString("Events", NewID, CINI::CurrentDocument->GetString("Events", CurrentID));
            CINI::CurrentDocument->WriteString("Actions", NewID, CINI::CurrentDocument->GetString("Actions", CurrentID));
            auto TagID = CINI::GetAvailableIndex();

            ppmfc::CString repeatMode = "0";
            if (auto pSection = CINI::CurrentDocument->GetSection("Tags"))
            {
                auto& section = pSection->GetEntities();
                size_t i = 0;
                for (auto& itr : section)
                {
                    auto splitsT = STDHelpers::SplitString(itr.second, 3);
                    if (strcmp(splitsT[2], CurrentID) == 0)
                        repeatMode = splitsT[0];
                }
            }


            CINI::CurrentDocument->WriteString("Tags", TagID, repeatMode + ",New tag," + NewID);

            int nIndex;
            if (ExtConfigs::DisplayTriggerID)
            {
                if (auto pSection = CINI::CurrentDocument->GetSection("Triggers"))
                {
                    std::vector<std::string> strings;
                    std::vector<std::string> IDs;

                    for (auto& pair : pSection->GetEntities())
                    {
                        auto splits = STDHelpers::SplitString(pair.second, 2);
                        strings.push_back(std::string(splits[2]));
                        IDs.push_back(std::string(pair.first));
                    }
                    strings.push_back(std::string(newName));
                    IDs.push_back(std::string(NewID));

                    std::vector<size_t> indices(strings.size());
                    std::vector<std::string> sorted_strings = strings;
                    std::iota(indices.begin(), indices.end(), 0);
                    std::sort(indices.begin(), indices.end(), [&](size_t i1, size_t i2) {
                        return sorted_strings[i1] < sorted_strings[i2];
                        });
                    for (size_t i = 0; i < strings.size(); ++i)
                    {
                        if (NewID == IDs[indices[i]].c_str())
                        {
                            ppmfc::CString tmp;
                            tmp.Format("%s (%s)", NewID, newName);

                            pThis->CCBCurrentTrigger.InsertString(i, tmp);
                            pThis->CCBCurrentTrigger.SetItemDataPtr(i, NewID.m_pchData);
                            pThis->CCBCurrentTrigger.SetCurSel(i);
                            break;
                        }
                    }

                }
            }
            else
            {
                nIndex = pThis->CCBCurrentTrigger.AddString(newName);
                pThis->CCBCurrentTrigger.SetItemDataPtr(nIndex, NewID.m_pchData);
                pThis->CCBCurrentTrigger.SetCurSel(nIndex);
            }


            pThis->OnCBCurrentTriggerSelectedChanged();

            TriggerSort::Instance.AddTrigger(NewID);
        }
    }

    return 0x4FCFAA;
}