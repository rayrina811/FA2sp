#include "MultiSelection.h"

#include <CFinalSunDlg.h>
#include <CIsoView.h>
#include <Helpers/Macro.h>
#include <Drawing.h>
#include <CTileTypeClass.h>
#include <CLoading.h>

#include "../Ext/CIsoView/Body.h"
#include "../Ext/CMapData/Body.h"

#include <algorithm>
#include <span>

// #include "../Source/CIsoView.h"

#include "../FA2sp.h"
#include "../Ext/CFinalSunDlg/Body.h"
#include <Miscs/Miscs.h>
#include "../Helpers/STDHelpers.h"
#include "../Helpers/Translations.h"
#include "Palettes.h"

std::set<MapCoord> MultiSelection::SelectedCoords;
std::set<MapCoord> MultiSelection::SelectedCoordsTemp;
MapCoord MultiSelection::LastAddedCoord;
bool MultiSelection::IsSquareSelecting = false;
bool MultiSelection::ShiftKeyIsDown = false;
bool MultiSelection::IsPasting = false;
BGRStruct MultiSelection::ColorHolder[0x1000];
std::vector<CellDataExt> MultiSelection::CopiedCells;
std::vector<MapCoord> MultiSelection::MultiPastedCoords;
int MultiSelection::CopiedX;
int MultiSelection::CopiedY;
bool MultiSelection::AddBuildingOptimize = false;
bool MultiSelection::SelectCellsChanged = false;
std::map<int, int>  MultiSelection::CopiedCellsBuilding;
CMultiSelectionOptionDlg MultiSelection::dlg;


bool MultiSelection::AddCoord(int X, int Y)
{
    if (X < 0 || Y < 0 || X > CMapData::Instance().MapWidthPlusHeight || Y > CMapData::Instance().MapWidthPlusHeight)
        return false;

    if (X == -1 || Y == -1)
        return false;

    MapCoord coords{ X,Y };
    auto itr = SelectedCoords.find(coords);
    if (itr == SelectedCoords.end())
    {
        SelectedCoords.insert(itr, coords);

        if (SelectedCoords.size() == 1)
            SelectedCoordsTemp.clear();

        auto itr2 = SelectedCoordsTemp.find(coords);
        if (itr2 == SelectedCoordsTemp.end())
        {
          SelectedCoordsTemp.insert(itr, coords);
        }
        return true;
    }

    return false;
}

bool MultiSelection::RemoveCoord(int X, int Y)
{
    if (X < 0 || Y < 0 || X > CMapData::Instance().MapWidthPlusHeight || Y > CMapData::Instance().MapWidthPlusHeight)
        return false;

    if (X == -1 || Y == -1)
        return false;

    MapCoord coords{ X,Y };
    auto itr = SelectedCoords.find(coords);
    if (itr != SelectedCoords.end())
    {
        SelectedCoords.erase(itr);
        auto itr2 = SelectedCoordsTemp.find(coords);
        if (itr2 != SelectedCoordsTemp.end())
        {
            SelectedCoordsTemp.erase(itr2);
        }
        return true;
    }

    return false;
}

size_t MultiSelection::GetCount()
{
    return SelectedCoords.size();
}
size_t MultiSelection::GetCount2()
{
    return SelectedCoordsTemp.size();
}

inline void MultiSelection::Clear()
{
    SelectedCoords.clear();

    MultiSelection::LastAddedCoord.X = -1;
    MultiSelection::LastAddedCoord.Y = -1;
}

inline void MultiSelection::ClearT()
{
    SelectedCoordsTemp.clear();

    MultiSelection::LastAddedCoord.X = -1;
    MultiSelection::LastAddedCoord.Y = -1;
}


void MultiSelection::Clear2()
{
    SelectedCoords.clear();

    MultiSelection::LastAddedCoord.X = -1;
    MultiSelection::LastAddedCoord.Y = -1;
}

void MultiSelection::ReverseStatus(int X, int Y)
{
    MapCoord mapCoord{ X,Y };
    auto itr = SelectedCoords.find(mapCoord);
    if (itr == SelectedCoords.end())
    {
        AddCoord(X, Y);
    }
    else
    {
        RemoveCoord(X, Y);
    }
        
    MultiSelection::SelectCellsChanged = true;
}

bool MultiSelection::IsSelected(int X, int Y)
{
    return SelectedCoords.find(MapCoord{ X,Y }) != SelectedCoords.end();
}

void MultiSelection::FindConnectedTiles(std::unordered_set<int>& process, int startX, int startY,
    std::unordered_set<int>& tileSet, bool firstRun)
{
    const auto cell = CMapData::Instance->GetCellAt(startX, startY);
    int ground = CMapDataExt::GetSafeTileIndex(cell->TileIndex);

    if (firstRun)
    {
        if (ground >= CMapDataExt::TileDataCount)
            return;
        tileSet.insert(CMapDataExt::TileData[ground].TileSet);
        if (dlg.ConsiderLAT)
        {
            for (int latidx = 0; latidx < CMapDataExt::Tile_to_lat.size(); ++latidx)
            {
                int& iSmoothSet = CMapDataExt::Tile_to_lat[latidx][0];
                int& iLatSet = CMapDataExt::Tile_to_lat[latidx][1];
                if (iSmoothSet == *tileSet.begin())
                {
                    tileSet.insert(iLatSet);
                }
                else if (iLatSet == *tileSet.begin())
                {
                    tileSet.insert(iSmoothSet);
                }
            }
        }
        if (!dlg.Connected)
        {
            for (int j = 0; j < CMapData::Instance->CellDataCount; j++) {
                int x = CMapData::Instance->GetXFromCoordIndex(j);
                int y = CMapData::Instance->GetYFromCoordIndex(j);
                if (!CMapData::Instance->IsCoordInMap(x, y))
                    continue;
                auto scCell = CMapData::Instance->GetCellAt(x, y);
                int scGround = CMapDataExt::GetSafeTileIndex(scCell->TileIndex);
                if (scGround >= CMapDataExt::TileDataCount)
                    continue;
                if (dlg.SameTileSet && tileSet.find(CMapDataExt::TileData[scGround].TileSet) == tileSet.end())
                    continue;
                if (dlg.SameHeight && scCell->Height != cell->Height)
                    continue;
                if (dlg.SameBaiscHeight && cell->Height - CMapDataExt::TileData[ground].TileBlockDatas[cell->TileSubIndex].Height
                    != scCell->Height - CMapDataExt::TileData[scGround].TileBlockDatas[scCell->TileSubIndex].Height)
                    continue;
                process.insert(j);
            }
            return;
        }
    }

    const int loop[5][2] = { {0, 0},{0, -1},{0, 1},{1, 0},{-1, 0} };
    for (auto pair : loop)
    {
        int newX = pair[0] + startX;
        int newY = pair[1] + startY;
        if (!CMapData::Instance->IsCoordInMap(newX, newY)) continue;
        int pos = newX + newY * CMapData::Instance->MapWidthPlusHeight;
        if (process.find(pos) != process.end())
            continue;
        auto scCell = CMapData::Instance->GetCellAt(pos);
        int scGround = CMapDataExt::GetSafeTileIndex(scCell->TileIndex);
        if (scGround >= CMapDataExt::TileDataCount)
            continue;
        if (dlg.SameTileSet && tileSet.find(CMapDataExt::TileData[scGround].TileSet) == tileSet.end())
            continue;
        if (dlg.SameHeight && scCell->Height != cell->Height)
            continue;
        if (dlg.SameBaiscHeight && cell->Height - CMapDataExt::TileData[ground].TileBlockDatas[cell->TileSubIndex].Height
            != scCell->Height - CMapDataExt::TileData[scGround].TileBlockDatas[scCell->TileSubIndex].Height)
            continue;
        
        process.insert(pos);
        FindConnectedTiles(process, newX, newY, tileSet, false);
    }
}

void MultiSelection::Copy()
{
    std::vector<MyClipboardData> data;
    for (const auto& coords : SelectedCoords)
    {
        if (coords.X < 0 || coords.Y < 0 || coords.X > CMapData::Instance().MapWidthPlusHeight || coords.Y > CMapData::Instance().MapWidthPlusHeight)
            continue;

        auto pCell = CMapData::Instance->GetCellAt(coords.X, coords.Y);
        MyClipboardData item = {};
        item.X = coords.X;
        item.Y = coords.Y;
        item.Overlay = pCell->Overlay;
        item.OverlayData = pCell->OverlayData;
        item.TileIndex = pCell->TileIndex;
        item.TileIndexHiPart = pCell->TileIndexHiPart;
        item.TileSubIndex = pCell->TileSubIndex;
        item.Height = pCell->Height;
        item.IceGrowth = pCell->IceGrowth;
        item.Flag = pCell->Flag;
        data.push_back(item);
    }

    auto hGlobal = GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE, 12 + sizeof(MyClipboardData) * data.size());
    if (hGlobal == NULL)
    {
        MessageBox(NULL, "Failed to allocate global memory!", "Error", MB_OK);
        return;
    }

    auto pBuffer = GlobalLock(hGlobal);
    if (pBuffer == nullptr)
    {
        MessageBox(NULL, "Failed to lock hGlobal handle!", "Error", MB_OK);
        return;
    }
    while (GlobalUnlock(hGlobal))
        ;

    reinterpret_cast<int*>(pBuffer)[0] = 0; // Flag indicate this is multi-selection
    reinterpret_cast<size_t*>(pBuffer)[1] = data.size();
    reinterpret_cast<int*>(pBuffer)[2] = CLoading::Instance->TheaterIdentifier;
    memcpy(reinterpret_cast<char*>(pBuffer) + 12, data.data(), sizeof(MyClipboardData) * data.size());
    
    OpenClipboard(CFinalSunApp::Instance->m_pMainWnd->m_hWnd);
    EmptyClipboard();
    if (FALSE == SetClipboardData(CFinalSunApp::Instance->ClipboardFormat, hGlobal))
        MessageBox(NULL, "Failed to set clipboard data", "Error", 0);
    CloseClipboard();
}


void MultiSelection::Paste(int X, int Y, int nBaseHeight, MyClipboardData* data, size_t length, bool obj = false)
{
    MultiPastedCoords.clear();
    if (X < 0 || Y < 0 || X > CMapData::Instance().MapWidthPlusHeight || Y > CMapData::Instance().MapWidthPlusHeight)
        return;
    std::span<MyClipboardData> cells {data, data + length};
    
    RECT bounds
    { 
        std::numeric_limits<LONG>::max(), 
        std::numeric_limits<LONG>::max(), 
        std::numeric_limits<LONG>::min(),
        std::numeric_limits<LONG>::min() 
    };
    for (const auto& cell : cells)
    {
        if (cell.X < bounds.left)
            bounds.left = cell.X;
        if (cell.X > bounds.right)
            bounds.right = cell.X;
        if (cell.Y < bounds.top)
            bounds.top = cell.Y;
        if (cell.Y > bounds.bottom)
            bounds.bottom = cell.Y;
    }
    const MapCoord center = { (int)std::ceil(double(bounds.left + bounds.right) / 2.0), (int)std::ceil(double(bounds.top + bounds.bottom) / 2.0) };

    CIsoViewExt::CopyStart.X = bounds.left - center.X + X;
    CIsoViewExt::CopyStart.Y = bounds.top - center.Y + Y;
    CIsoViewExt::CopyEnd.X = bounds.right + 1 - center.X + X;
    CIsoViewExt::CopyEnd.Y = bounds.bottom + 1 - center.Y + Y;
    CMapData::Instance->SaveUndoRedoData(true, CIsoViewExt::CopyStart.X, CIsoViewExt::CopyStart.Y, CIsoViewExt::CopyEnd.X + 1, CIsoViewExt::CopyEnd.Y + 1);

    auto lowest_height = std::numeric_limits<unsigned char>::min();
    for (const auto& cell : cells)
    {
        int offset_x = cell.X - center.X;
        int offset_y = cell.Y - center.Y;

        if (X + offset_x < 0 || Y + offset_y < 0 || X + offset_x > CMapData::Instance().MapWidthPlusHeight || Y + offset_y > CMapData::Instance().MapWidthPlusHeight)
            continue;

        const auto pCell = CMapData::Instance->TryGetCellAt(X + offset_x, Y + offset_y);
        if (pCell->Height < lowest_height)
            lowest_height = pCell->Height;
    }

    nBaseHeight += lowest_height;
    for (const auto& cell : cells)
    {
        int offset_x = cell.X - center.X;
        int offset_y = cell.Y - center.Y;

        if (X + offset_x < 0 || Y + offset_y < 0 || X + offset_x > CMapData::Instance().MapWidthPlusHeight || Y + offset_y > CMapData::Instance().MapWidthPlusHeight)
            continue;

        auto nCellIndex = CMapData::Instance->GetCoordIndex(X + offset_x, Y + offset_y);
        if (nCellIndex < 0 || nCellIndex >= CMapData::Instance->CellDataCount)
            continue;

        auto pCell = CMapData::Instance->GetCellAt(nCellIndex);
        
        if (CIsoViewExt::PasteOverlays)
        {
            CMapData::Instance->DeleteTiberium(pCell->Overlay, pCell->OverlayData);
            pCell->Overlay = cell.Overlay;
            pCell->OverlayData = cell.OverlayData;
            CMapData::Instance->AddTiberium(pCell->Overlay, pCell->OverlayData);
        }

        if (CIsoViewExt::PasteGround)
        {
            pCell->TileIndex = cell.TileIndex;
            pCell->TileIndexHiPart = cell.TileIndexHiPart;
            pCell->TileSubIndex = cell.TileSubIndex;

            pCell->Height = std::clamp(cell.Height + nBaseHeight, 0, 14);

            pCell->IceGrowth = cell.IceGrowth;
            pCell->Flag = cell.Flag;

        }

        CMapData::Instance->UpdateMapPreviewAt(X + offset_x, Y + offset_y);
        MultiPastedCoords.push_back(MapCoord{ X + offset_x, Y + offset_y });
    }
}

DEFINE_HOOK(456EFC, CIsoView_OnMouseMove_MultiSelect_SelectStatus, 6)
{
    if (!ExtConfigs::EnableMultiSelection)
        return 0;

    GET_STACK(UINT, eFlags, STACK_OFFS(0x3D528, -0x4));
    REF_STACK(const CPoint, point, STACK_OFFS(0x3D528, -0x8));

    if (CIsoView::CurrentCommand->Command == 0x1D && (eFlags & MK_LBUTTON))
    {
        {
            auto coord = CIsoView::GetInstance()->GetCurrentMapCoord(point);
            if (CIsoView::CurrentCommand->Type == 0)
                if (MultiSelection::AddCoord(coord.X, coord.Y))
                {
                    CIsoView::GetInstance()->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
                }
            if (CIsoView::CurrentCommand->Type == 1)
                if (MultiSelection::RemoveCoord(coord.X, coord.Y))
                {
                    CIsoView::GetInstance()->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
                }
            if (CIsoView::CurrentCommand->Type == 2)
            {
                MultiSelection::Clear();
                CIsoView::GetInstance()->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
            }
            if (CIsoView::CurrentCommand->Type == 7)
            {
                std::unordered_set<int> coords;
                std::unordered_set<int> tilesets;
                MultiSelection::FindConnectedTiles(coords, coord.X, coord.Y, tilesets, true);
                for (auto& coord : coords)
                {
                    int x = CMapData::Instance->GetXFromCoordIndex(coord);
                    int y = CMapData::Instance->GetYFromCoordIndex(coord);
                    MultiSelection::AddCoord(x, y);
                }
                CIsoView::GetInstance()->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
            }
            if (CIsoView::CurrentCommand->Type == 8)
            {
                std::unordered_set<int> coords;
                std::unordered_set<int> tilesets;
                MultiSelection::FindConnectedTiles(coords, coord.X, coord.Y, tilesets, true);
                for (auto& coord : coords)
                {
                    int x = CMapData::Instance->GetXFromCoordIndex(coord);
                    int y = CMapData::Instance->GetYFromCoordIndex(coord);
                    MultiSelection::RemoveCoord(x, y);
                }
                CIsoView::GetInstance()->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
            }               
            return 0x456EB6;
        }
    }

    return 0;
}

DEFINE_HOOK(469470, CIsoView_OnKeyDown, 5)
{
    if (!ExtConfigs::EnableMultiSelection)
        return 0;

    GET(CIsoView*, pThis, ECX);
    GET_STACK(UINT, nChar, 0x4);

    if (nChar == 'D')
    {
        if (CIsoView::ControlKeyIsDown)
			MultiSelection::Clear();
		else
			CFinalSunApp::Instance->FlatToGround = !CFinalSunApp::Instance->FlatToGround;

        pThis->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    }
    else if (nChar == 'A')
        pThis->KeyboardAMode = !pThis->KeyboardAMode;
    else if (nChar == VK_CONTROL)
        CIsoView::ControlKeyIsDown = true;
    else if (nChar == VK_SHIFT)
        MultiSelection::ShiftKeyIsDown = true;

    R->EAX(pThis->Default());

    return 0x4694A9;
}

DEFINE_HOOK(46BC30, CIsoView_OnKeyUp, 5)
{
    if (!ExtConfigs::EnableMultiSelection)
        return 0;

    GET(CIsoView*, pThis, ECX);
    GET_STACK(UINT, nChar, 0x4);

    if (nChar == VK_CONTROL)
        CIsoView::ControlKeyIsDown = false;
    else if (nChar == VK_SHIFT)
        MultiSelection::ShiftKeyIsDown = false;

    R->EAX(pThis->Default());

    return 0x46BC46;
}

DEFINE_HOOK(433DA0, CFinalSunDlg_Tools_RaiseSingleTile, 5)
{
    if (!ExtConfigs::EnableMultiSelection)
        return 0;

    GET(CFinalSunDlg*, pThis, ECX);

    if (CMapData::Instance->MapWidthPlusHeight)
    {
        if (MultiSelection::GetCount())
        {
            CMapData::Instance->SaveUndoRedoData(true, 0, 0, 0, 0);
            MultiSelection::ApplyForEach(
                [](CellData& cell) {
                    if (cell.Height < 14)
                        ++cell.Height;
                }
            );
            pThis->MyViewFrame.pIsoView->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
            CMapData::Instance->SaveUndoRedoData(true, 0, 0, 0, 0);
            CMapData::Instance->DoUndo();
        }
        else
        {
            if (CIsoView::CurrentCommand->Command != FACurrentCommand::RaiseSingleTile && !ExtConfigs::SkipBrushSizeChangeOnTools)
            {
                pThis->BrushSize.nCurSel = 0;
                pThis->BrushSize.UpdateData(FALSE);
                pThis->MyViewFrame.pIsoView->BrushSizeX = 1;
                pThis->MyViewFrame.pIsoView->BrushSizeY = 1;
            }
            pThis->PlaySound(CFinalSunDlg::FASoundType::Normal);
            CIsoView::CurrentCommand->Command = FACurrentCommand::RaiseSingleTile;
        }
    }
    else
        pThis->PlaySound(CFinalSunDlg::FASoundType::Error);

    return 0x433DB7;
}

DEFINE_HOOK(433D30, CFinalSunDlg_Tools_LowerSingleTile, 5)
{
    if (!ExtConfigs::EnableMultiSelection)
        return 0;

    GET(CFinalSunDlg*, pThis, ECX);

    if (CMapData::Instance->MapWidthPlusHeight)
    {
        if (MultiSelection::GetCount())
        {
            CMapData::Instance->SaveUndoRedoData(true, 0, 0, 0, 0);
            MultiSelection::ApplyForEach(
                [](CellData& cell) {
                    if (cell.Height > 0)
                        --cell.Height;
                }
            );
            pThis->MyViewFrame.pIsoView->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
            CMapData::Instance->SaveUndoRedoData(true, 0, 0, 0, 0);
            CMapData::Instance->DoUndo();
        }
        else
        {
            if (CIsoView::CurrentCommand->Command != FACurrentCommand::LowerSingleTile && !ExtConfigs::SkipBrushSizeChangeOnTools)
            {
                pThis->BrushSize.nCurSel = 0;
                pThis->BrushSize.UpdateData(FALSE);
                pThis->MyViewFrame.pIsoView->BrushSizeX = 1;
                pThis->MyViewFrame.pIsoView->BrushSizeY = 1;
            }
            pThis->PlaySound(CFinalSunDlg::FASoundType::Normal);
            CIsoView::CurrentCommand->Command = FACurrentCommand::LowerSingleTile;
        }
    }
    else
        pThis->PlaySound(CFinalSunDlg::FASoundType::Error);

    return 0x433D47;
}

DEFINE_HOOK(433F70, CFinalSunDlg_Tools_HideSingleField, 5)
{
    if (!ExtConfigs::EnableMultiSelection)
        return 0;

    GET(CFinalSunDlg*, pThis, ECX);

    if (CMapData::Instance->MapWidthPlusHeight)
    {
        if (MultiSelection::GetCount())
        {
            MultiSelection::ApplyForEach(
                [](CellData& cell) {
                    cell.Flag.IsHiddenCell = true;
                }
            );
            pThis->MyViewFrame.pIsoView->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
        }
        else
            CIsoView::CurrentCommand->Command = FACurrentCommand::HideSingleField;

        pThis->PlaySound(CFinalSunDlg::FASoundType::Normal);
    }
    else
        pThis->PlaySound(CFinalSunDlg::FASoundType::Error);

    return 0x433F83;
}

DEFINE_HOOK(435F10, CFinalSunDlg_Tools_Copy, 7)
 {
    MultiSelection::MultiPastedCoords.clear();
     GET(CFinalSunDlg*, pThis, ECX);
 
     pThis->PlaySound(CFinalSunDlg::FASoundType::Normal);
 
     if (ExtConfigs::EnableMultiSelection && MultiSelection::GetCount())
         MultiSelection::Copy();
     else
     {
         MultiSelection::ClearT();
         CIsoView::CurrentCommand->Command = FACurrentCommand::TileCopy;
     }  
     if (MultiSelection::GetCount())
     {
         auto& mapData = CMapData::Instance();
         MultiSelection::CopiedCells.clear();

         OpenClipboard(CFinalSunApp::Instance->m_pMainWnd->m_hWnd);
         HANDLE hData = GetClipboardData(CFinalSunApp::Instance->ClipboardFormat);
         auto ptr = GlobalLock(hData);

         if (ptr)
         {
             if (reinterpret_cast<int*>(ptr)[0] == 0) // Multi-selection
             {
                 const auto length = reinterpret_cast<size_t*>(ptr)[1];
                 const int identifier = reinterpret_cast<int*>(ptr)[2];
                 if (identifier == CLoading::Instance->TheaterIdentifier)
                 {
                     const auto p = reinterpret_cast<MultiSelection::MyClipboardData*>(reinterpret_cast<char*>(ptr) + 12);

                     std::span<MultiSelection::MyClipboardData> cells{ p, p + length };

                     RECT bounds
                     {
                         std::numeric_limits<LONG>::max(),
                         std::numeric_limits<LONG>::max(),
                         std::numeric_limits<LONG>::min(),
                         std::numeric_limits<LONG>::min()
                     };
                     for (const auto& cell : cells)
                     {
                         if (cell.X < bounds.left)
                             bounds.left = cell.X;
                         if (cell.X > bounds.right)
                             bounds.right = cell.X;
                         if (cell.Y < bounds.top)
                             bounds.top = cell.Y;
                         if (cell.Y > bounds.bottom)
                             bounds.bottom = cell.Y;
                     }

                     MultiSelection::CopiedX = bounds.right + 1 - bounds.left;
                     MultiSelection::CopiedY = bounds.bottom + 1 - bounds.top;

                     int bi = 0;
                     MultiSelection::CopiedCellsBuilding.clear();
                     for (int i = 0; i < mapData.CellDataCount; i++)
                         MultiSelection::CopiedCellsBuilding[i] = -1;
                     if (auto pSection = CMapData::Instance->INI.GetSection("Structures"))
                     {
                         for (auto& pair : pSection->GetEntities())
                         {
                             auto atoms = STDHelpers::SplitString(pair.second);
                             if (atoms.size() > 4)
                             {
                                 int x = atoi(atoms[4]);
                                 int y = atoi(atoms[3]);
                                 auto dwpos = y * CMapData::Instance().MapWidthPlusHeight + x;
                                 if (dwpos < mapData.CellDataCount)
                                 {
                                     MultiSelection::CopiedCellsBuilding[dwpos] = bi;
                                 }
                             }
                             bi++;
                         }
                     }

                     for (int i = bounds.left; i < bounds.left + MultiSelection::CopiedX; i++)
                     {
                         for (int j = bounds.top; j < bounds.top + MultiSelection::CopiedY; j++)
                         {
                             auto dwpos = j * CMapData::Instance().MapWidthPlusHeight + i;
                             if (dwpos < mapData.CellDataCount)
                             {
                                 CellDataExt& cellExt = CMapDataExt::CellDataExts[dwpos];
                                 CellData& cell = mapData.CellDatas[dwpos];
                                 cellExt.X = i;
                                 cellExt.Y = j;

                                 if (MultiSelection::CopiedCellsBuilding[dwpos] > -1)
                                 {
                                     CBuildingData copyObj;
                                     CMapDataExt::GetBuildingDataByIniID(MultiSelection::CopiedCellsBuilding[dwpos], copyObj);
                                     cellExt.BuildingData = copyObj;
                                 }
                                 for (int subpos = 0; subpos < 3; subpos++)
                                 {
                                     if (cell.Infantry[subpos] != -1)
                                     {
                                         CInfantryData copyObj;
                                         mapData.GetInfantryData(cell.Infantry[subpos], copyObj);
                                         cellExt.InfantryData[subpos] = copyObj;
                                     }
                                 }
                                 if (cell.Unit != -1)
                                 {
                                     CUnitData copyObj;
                                     mapData.GetUnitData(cell.Unit, copyObj);
                                     cellExt.UnitData = copyObj;
                                 }
                                 if (cell.Aircraft != -1)
                                 {
                                     CAircraftData copyObj;
                                     mapData.GetAircraftData(cell.Aircraft, copyObj);
                                     cellExt.AircraftData = copyObj;
                                 }
                                 if (cell.Terrain != -1)
                                 {
                                     cellExt.TerrainData = Variables::GetRulesMapValueAt("TerrainTypes", cell.TerrainType);
                                 }
                                 if (cell.Smudge != -1)
                                 {
                                     cellExt.SmudgeData = Variables::GetRulesMapValueAt("SmudgeTypes", cell.SmudgeType);
                                 }

                                 MultiSelection::CopiedCells.push_back(cellExt);
                             }
                             else
                             {
                                 CellDataExt& cellExt = CMapDataExt::CellDataExts[dwpos];
                                 cellExt.X = i;
                                 cellExt.Y = j;
                                 MultiSelection::CopiedCells.push_back(cellExt);
                             }
                         }
                     }
                 }
             }

         }
         GlobalUnlock(hData);
         CloseClipboard();
     }

     return 0x435F24;
 }
DEFINE_HOOK(46174D, CIsoView_OnMouseClick_Copy, 5)
{
    MultiSelection::MultiPastedCoords.clear();
    if (!MultiSelection::GetCount())
    {
        GET(int, X1, EDX);
        GET(int, Y1, EAX);
        GET(int, X2, EDI);
        GET(int, Y2, ESI);
        auto& mapData = CMapData::Instance();
        
        MultiSelection::CopiedCells.clear();
        MultiSelection::CopiedX = X2 - X1;
        MultiSelection::CopiedY = Y2 - Y1;

        int bi = 0;
        MultiSelection::CopiedCellsBuilding.clear();
        for (int i = 0; i < mapData.CellDataCount; i++)
            MultiSelection::CopiedCellsBuilding[i] = -1;
        if (auto pSection = CMapData::Instance->INI.GetSection("Structures"))
        {
            for (auto& pair : pSection->GetEntities())
            {
                auto atoms = STDHelpers::SplitString(pair.second);
                if (atoms.size() > 4)
                {
                    int x = atoi(atoms[4]);
                    int y = atoi(atoms[3]);
                    auto dwpos = y * CMapData::Instance().MapWidthPlusHeight + x;
                    if (dwpos < mapData.CellDataCount)
                    {
                        MultiSelection::CopiedCellsBuilding[dwpos] = bi;
                    }
                }
                bi++;
            }
        }

        for (int i = X1; i < X1 + MultiSelection::CopiedX; i++)
        {
            for (int j = Y1; j < Y1 + MultiSelection::CopiedY; j++)
            {
                auto dwpos = j * CMapData::Instance().MapWidthPlusHeight + i;
                if (dwpos < mapData.CellDataCount)
                {
                    CellDataExt& cellExt = CMapDataExt::CellDataExts[dwpos];
                    CellData& cell = mapData.CellDatas[dwpos];
                    cellExt.X = i;
                    cellExt.Y = j;

                    if (MultiSelection::CopiedCellsBuilding[dwpos] > -1)
                    {
                        CBuildingData copyObj;
                        CMapDataExt::GetBuildingDataByIniID(MultiSelection::CopiedCellsBuilding[dwpos], copyObj);
                        cellExt.BuildingData = copyObj;
                    }
                    for (int subpos = 0; subpos < 3; subpos++)
                    {
                        if (cell.Infantry[subpos] != -1)
                        {
                            CInfantryData copyObj;
                            mapData.GetInfantryData(cell.Infantry[subpos], copyObj);
                            cellExt.InfantryData[subpos] = copyObj;
                        }
                    }
                    if (cell.Unit != -1)
                    {
                        CUnitData copyObj;
                        mapData.GetUnitData(cell.Unit, copyObj);
                        cellExt.UnitData = copyObj;
                    }
                    if (cell.Aircraft != -1)
                    {
                        CAircraftData copyObj;
                        mapData.GetAircraftData(cell.Aircraft, copyObj);
                        cellExt.AircraftData = copyObj;
                    }
                    if (cell.Terrain != -1)
                    {
                        cellExt.TerrainData = Variables::GetRulesMapValueAt("TerrainTypes", cell.TerrainType);
                    }
                    if (cell.Smudge != -1)
                    {
                        cellExt.SmudgeData = Variables::GetRulesMapValueAt("SmudgeTypes", cell.SmudgeType);
                    }

                    MultiSelection::CopiedCells.push_back(cellExt);
                }
                else
                {
                    CellDataExt& cellExt = CMapDataExt::CellDataExts[dwpos];
                    cellExt.X = i;
                    cellExt.Y = j;
                    MultiSelection::CopiedCells.push_back(cellExt);
                }
            }
        }
    }

    return 0;
}
DEFINE_HOOK(435F3A, CFinalSunDlg_CopyWholeMap, 5)
{
    MultiSelection::MultiPastedCoords.clear();
    if (!MultiSelection::GetCount())
    {
        auto& mapData = CMapData::Instance();

        int bi = 0;
        MultiSelection::CopiedCells.clear();
        MultiSelection::CopiedX = CMapData::Instance().MapWidthPlusHeight;
        MultiSelection::CopiedY = CMapData::Instance().MapWidthPlusHeight;

        MultiSelection::CopiedCellsBuilding.clear();
        for (int i = 0; i < mapData.CellDataCount; i++)
            MultiSelection::CopiedCellsBuilding[i] = -1;
        if (auto pSection = CMapData::Instance->INI.GetSection("Structures"))
        {
            for (auto& pair : pSection->GetEntities())
            {
                auto atoms = STDHelpers::SplitString(pair.second);
                if (atoms.size() > 4)
                {
                    int x = atoi(atoms[4]);
                    int y = atoi(atoms[3]);
                    auto dwpos = y * CMapData::Instance().MapWidthPlusHeight + x;
                    if (dwpos < mapData.CellDataCount)
                    {
                        MultiSelection::CopiedCellsBuilding[dwpos] = bi;
                    }
                }
                bi++;
            }
        }


        for (int i = 0; i < CMapData::Instance().MapWidthPlusHeight; i++)
        {
            for (int j = 0; j < CMapData::Instance().MapWidthPlusHeight; j++)
            {
                CellDataExt& cellExt = CMapDataExt::CellDataExts[i + j * CMapData::Instance().MapWidthPlusHeight];
                CellData& cell = mapData.CellDatas[i + j * CMapData::Instance().MapWidthPlusHeight];
                cellExt.X = i;
                cellExt.Y = j;

                auto dwpos = j * CMapData::Instance().MapWidthPlusHeight + i;
                if (MultiSelection::CopiedCellsBuilding[dwpos] > -1)
                {
                    CBuildingData copyObj;
                    CMapDataExt::GetBuildingDataByIniID(MultiSelection::CopiedCellsBuilding[dwpos], copyObj);
                    cellExt.BuildingData = copyObj;
                }
                for (int subpos = 0; subpos < 3; subpos++)
                {
                    if (cell.Infantry[subpos] != -1)
                    {
                        CInfantryData copyObj;
                        mapData.GetInfantryData(cell.Infantry[subpos], copyObj);
                        cellExt.InfantryData[subpos] = copyObj;
                    }
                }
                if (cell.Unit != -1)
                {
                    CUnitData copyObj;
                    mapData.GetUnitData(cell.Unit, copyObj);
                    cellExt.UnitData = copyObj;
                }
                if (cell.Aircraft != -1)
                {
                    CAircraftData copyObj;
                    mapData.GetAircraftData(cell.Aircraft, copyObj);
                    cellExt.AircraftData = copyObj;
                }
                if (cell.Terrain != -1)
                {
                    cellExt.TerrainData = Variables::GetRulesMapValueAt("TerrainTypes", cell.TerrainType);
                }
                if (cell.Smudge != -1)
                {
                    cellExt.SmudgeData = Variables::GetRulesMapValueAt("SmudgeTypes", cell.SmudgeType);
                }

                MultiSelection::CopiedCells.push_back(cellExt);
            }
        }

    }
    return 0;
}

DEFINE_HOOK(4C3B6B, CMapData_Paste_ChangeAltImage, 9)
{
    GET(int, pos, ESI);
    GET(int, tileIndex, ECX);
    if (CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(tileIndex)].TileSet == CMapDataExt::BridgeSet
        || CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(tileIndex)].TileSet == CMapDataExt::WoodBridgeSet)
    {
        pos = pos >> 6;
        auto cell = CMapData::Instance->GetCellAt(pos);
        cell->Flag.AltIndex = 0;
    }
    return 0;
}
DEFINE_HOOK(4C3A43, CMapData_Paste_Overlay, 6)
{
    if (!CIsoViewExt::PasteOverlays)
        return 0x4C3A75;
    return 0;
}
DEFINE_HOOK(4C3A75, CMapData_Paste_Ground, 7)
{
    if (!CIsoViewExt::PasteGround)
        return 0x4C3BA9;
    return 0;
}

DEFINE_HOOK(4616A2, CIsoView_OnMouseClick_Paste, 5)
{
    if (!MultiSelection::CopiedCells.empty())
    {
        auto point = CIsoView::GetInstance()->GetCurrentMapCoord(CIsoView::GetInstance()->MouseCurrentPosition);
        MapCoord startP;
        auto& mapData = CMapData::Instance();

        startP.X = point.X - MultiSelection::CopiedX / 2;
        startP.Y = point.Y - MultiSelection::CopiedY / 2;

        int index = 0;
        MultiSelection::AddBuildingOptimize = true;

        if (CIsoViewExt::PasteOverriding)
        {
            for (int i = startP.X; i < startP.X + MultiSelection::CopiedX; i++)
            {
                for (int j = startP.Y; j < startP.Y + MultiSelection::CopiedY; j++)
                {
                    auto length = CMapData::Instance().MapWidthPlusHeight;
                    auto dwpos = j * length + i;
                    if ( CMapData::Instance->IsCoordInMap(i, j) && dwpos < mapData.CellDataCount)
                    {
                        if (index >= MultiSelection::CopiedCells.size())
                        {
                            index++;
                            continue;
                        }
                        auto& cell = MultiSelection::CopiedCells[index];
                        auto& tCell = mapData.CellDatas[dwpos];

                        if (MultiSelection::GetCount2())
                        {
                            bool found = false;
                            for (auto& coord : MultiSelection::SelectedCoordsTemp)
                            {
                                if (coord.X == cell.X && coord.Y == cell.Y)
                                {

                                    found = true;
                                    break;
                                }
                            }
                            if (!found)
                            {
                                index++;
                                continue;
                            }
                        }


                        if (CIsoViewExt::PasteInfantries)
                            for (int subpos = 0; subpos < 3; subpos++)
                                if (tCell.Infantry[subpos] != -1)
                                    mapData.DeleteInfantryData(tCell.Infantry[subpos]);
                        if (CIsoViewExt::PasteUnits)
                            if (tCell.Unit != -1)
                                mapData.DeleteUnitData(tCell.Unit);
                        if (CIsoViewExt::PasteAircrafts)
                            if (tCell.Aircraft != -1)
                                mapData.DeleteAircraftData(tCell.Aircraft);
                        if (CIsoViewExt::PasteStructures)
                            if (tCell.Structure != -1)
                                mapData.DeleteBuildingData(tCell.Structure);
                        if (CIsoViewExt::PasteTerrains)
                            if (tCell.Terrain != -1)
                                mapData.DeleteTerrainData(tCell.Terrain);
                        if (CIsoViewExt::PasteSmudges)
                            if (tCell.Smudge != -1)
                                mapData.DeleteSmudgeData(tCell.Smudge);        
                    }
                    index++;
                }
            }
            CMapData::Instance->UpdateFieldStructureData(FALSE);
        }
        
        index = 0;
        for (int i = startP.X; i < startP.X + MultiSelection::CopiedX; i++)
        {
            for (int j = startP.Y; j < startP.Y + MultiSelection::CopiedY; j++)
            {
                auto length = CMapData::Instance().MapWidthPlusHeight;
                auto dwpos = j * length + i;
                if ( CMapData::Instance->IsCoordInMap(i, j) && dwpos < mapData.CellDataCount)
                {
                    if (index >= MultiSelection::CopiedCells.size())
                    {
                        index++;
                        continue;
                    }
                    auto& cell = MultiSelection::CopiedCells[index];
                    auto& tCell = mapData.CellDatas[mapData.GetCoordIndex(cell.X, cell.Y)];

                    if (MultiSelection::GetCount2())
                    {
                        bool found = false;
                        for (auto& coord : MultiSelection::SelectedCoordsTemp)
                        {
                            if (coord.X == cell.X && coord.Y == cell.Y)
                            {
                                found = true;
                                break;
                            }
                        }
                        if (!found)
                        {
                            index++;
                            continue;
                        }
                    }

                    if (CIsoViewExt::PasteInfantries)
                        for (int subpos = 0; subpos < 3; subpos++)
                        {
                            if (cell.InfantryData[subpos].House != "")
                            {
                                CInfantryData copyObj = cell.InfantryData[subpos];

                                char bufferx[10];
                                _itoa(i, bufferx, 10);
                                char buffery[10];
                                _itoa(j, buffery, 10);

                                copyObj.X = bufferx;
                                copyObj.Y = buffery;
                                CMapData::Instance->SetInfantryData(&copyObj, NULL, NULL, 0, -1);
                            }
                        }
                    if (CIsoViewExt::PasteUnits)
                        if (cell.UnitData.House != "")
                        {
                            CUnitData copyObj = cell.UnitData;

                            char bufferx[10];
                            _itoa(i, bufferx, 10);
                            char buffery[10];
                            _itoa(j, buffery, 10);

                            copyObj.X = bufferx;
                            copyObj.Y = buffery;
                            CMapData::Instance->SetUnitData(&copyObj, NULL, NULL, 0, "");
                        }
                    if (CIsoViewExt::PasteAircrafts)
                        if (cell.AircraftData.House != "")
                        {
                            CAircraftData copyObj = cell.AircraftData;

                            char bufferx[10];
                            _itoa(i, bufferx, 10);
                            char buffery[10];
                            _itoa(j, buffery, 10);

                            copyObj.X = bufferx;
                            copyObj.Y = buffery;
                            CMapData::Instance->SetAircraftData(&copyObj, NULL, NULL, 0, "");
                        }

                    if (CIsoViewExt::PasteStructures)
                        if (cell.BuildingData.House != "")
                        {
                            CBuildingData copyObj = cell.BuildingData;

                            char bufferx[10];
                            _itoa(i, bufferx, 10);
                            char buffery[10];
                            _itoa(j, buffery, 10);

                            copyObj.X = bufferx;
                            copyObj.Y = buffery;
                            CMapData::Instance->SetBuildingData(&copyObj, NULL, NULL, 0, "");
                        }
                    if (CIsoViewExt::PasteTerrains)
                        if (cell.TerrainData != "")
                        {
                            auto pCell = CMapData::Instance->GetCellAt(i, j);
                            if (pCell->Terrain > -1) {
                                CMapData::Instance->DeleteTerrainData(pCell->Terrain);
                            }
                            CMapData::Instance->SetTerrainData(cell.TerrainData, CMapData::Instance->GetCoordIndex(i, j));

                        }
                    if (CIsoViewExt::PasteSmudges)
                        if (cell.SmudgeData != "")
                        {
                            CSmudgeData smudge;
                            smudge.X = j;
                            smudge.Y = i;
                            smudge.Flag = 0;
                            smudge.TypeID = cell.SmudgeData;
                            auto pCell = CMapData::Instance->GetCellAt(i, j);
                            if (pCell->Smudge > -1) {
                                CMapData::Instance->DeleteSmudgeData(pCell->Smudge);
                            }
                            CMapData::Instance->SetSmudgeData(&smudge);
                            CMapData::Instance->UpdateFieldSmudgeData(false);
                        }
                }
                index++;
            }
        }
        CMapData::Instance->UpdateFieldStructureData(FALSE);
        MultiSelection::AddBuildingOptimize = false;
    }
    return 0;
}

DEFINE_HOOK(459FFB, CIsoView_OnMouseMove_Paste_Snapshot, 6)
{
    return 0x45A00F;
}

bool OnLButtonDownPasted = false;
DEFINE_HOOK(46168E, CIsoView_OnLButtonDown_Paste_Snapshot, 6)
{
    OnLButtonDownPasted = true;
    return 0x4616A2;
}
DEFINE_HOOK(4616BA, CIsoView_OnLButtonDown_Paste_Snapshot_2, 6)
{
    if (OnLButtonDownPasted)
    {
        OnLButtonDownPasted = false;
        // redo data is lost because of OnMouseMove
        //CMapData::Instance->SaveUndoRedoData(true, CIsoViewExt::CopyStart.X, CIsoViewExt::CopyStart.Y, CIsoViewExt::CopyEnd.X + 1, CIsoViewExt::CopyEnd.Y + 1);
        //CMapData::Instance->DoUndo();
        return 0x4616D8;
    }
    
    return 0;
}

DEFINE_HOOK(4C38B0, CMapData_Paste_GetCoords_width, 6)
{
    CIsoViewExt::CopyStart.X = R->Stack<int>(STACK_OFFS(0x58, -0x4));
    CIsoViewExt::CopyStart.Y = R->Stack<int>(STACK_OFFS(0x58, -0x8));
    CIsoViewExt::CopyEnd.X = R->ECX<int>();
    return 0;
}

DEFINE_HOOK(4C38C1, CMapData_Paste_GetCoords_height, 8)
{
    CIsoViewExt::CopyEnd.Y = R->EDX<int>();
    CIsoViewExt::CopyStart.X -= CIsoViewExt::CopyEnd.X / 2;
    CIsoViewExt::CopyStart.Y -= CIsoViewExt::CopyEnd.Y / 2;
    CIsoViewExt::CopyEnd.X += CIsoViewExt::CopyStart.X - 1;
    CIsoViewExt::CopyEnd.Y += CIsoViewExt::CopyStart.Y - 1;

    CMapData::Instance->SaveUndoRedoData(true, CIsoViewExt::CopyStart.X, CIsoViewExt::CopyStart.Y, CIsoViewExt::CopyEnd.X + 1, CIsoViewExt::CopyEnd.Y + 1);
    return 0;
}

DEFINE_HOOK(4C3850, CMapData_PasteAt, 8)
{
    if (!ExtConfigs::EnableMultiSelection)
        return 0;

    GET_STACK(const int, X, 0x4);
    GET_STACK(const int, Y, 0x8);
    GET_STACK(const char, nBaseHeight, 0xC);

    OpenClipboard(CFinalSunApp::Instance->m_pMainWnd->m_hWnd);
    HANDLE hData = GetClipboardData(CFinalSunApp::Instance->ClipboardFormat);
    auto ptr = GlobalLock(hData);
    
    if (ptr)
    {
        if (reinterpret_cast<int*>(ptr)[0] == 0) // Multi-selection
        {
            const auto length = reinterpret_cast<size_t*>(ptr)[1];
            const int identifier = reinterpret_cast<int*>(ptr)[2];
            if (identifier == CLoading::Instance->TheaterIdentifier)
            {
                const auto p = reinterpret_cast<MultiSelection::MyClipboardData*>(reinterpret_cast<char*>(ptr) + 12);
                if (X < 0 || Y < 0 || X > CMapData::Instance().MapWidthPlusHeight || Y > CMapData::Instance().MapWidthPlusHeight)
                {

                }
                else
                    MultiSelection::Paste(X, Y, nBaseHeight, p, length);
            }
            GlobalUnlock(hData);
            CloseClipboard();
            return 0x4C388B;
        }
        else // Default selection
        {
            GlobalUnlock(hData);
            CloseClipboard();
            return 0;
        }
    }

    CloseClipboard();
    return 0x4C388B;
}

DEFINE_HOOK(4B9F7A, CreateMap_ClearUp_MultiSelection, 5)
{
    if (!ExtConfigs::EnableMultiSelection)
        return 0;

    MultiSelection::Clear();

    return 0;
}

DEFINE_HOOK(49D2C0, LoadMap_ClearUp_MultiSelection, 5)
{
    if (!ExtConfigs::EnableMultiSelection)
        return 0;

    MultiSelection::Clear();

    return 0;
}
