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
bool MultiSelection::ShiftKeyIsDown = false;
bool MultiSelection::IsPasting = false;
BGRStruct MultiSelection::ColorHolder[0x1000];
std::vector<CellDataExt> MultiSelection::CopiedCells;
int MultiSelection::CopiedX;
int MultiSelection::CopiedY;
bool MultiSelection::AddBuildingOptimize = false;
bool MultiSelection::SelectCellsChanged = false;
std::map<int, int>  MultiSelection::CopiedCellsBuilding;


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

inline bool MultiSelection::IsSelected(int X, int Y)
{
    return SelectedCoords.find(MapCoord{ X,Y }) != SelectedCoords.end();
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
        //if (CIsoView::ControlKeyIsDown && (eFlags & MK_LBUTTON))
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
            if (CIsoView::CurrentCommand->Type == 3)
            {
                //MultiSelection::AddCoord(coord.X, coord.Y);
                if (MultiSelection::LastAddedCoord.X > -1 &&
                    MultiSelection::LastAddedCoord.X != coord.X &&
                    MultiSelection::LastAddedCoord.Y != coord.Y
                    )
                {
                    int x1, x2, y1, y2;

                    if (MultiSelection::LastAddedCoord.X < coord.X)
                    {
                        x1 = MultiSelection::LastAddedCoord.X;
                        x2 = coord.X;
                    }
                    else
                    {
                        x1 = coord.X;
                        x2 = MultiSelection::LastAddedCoord.X;
                    }
                    if (MultiSelection::LastAddedCoord.Y < coord.Y)
                    {
                        y1 = MultiSelection::LastAddedCoord.Y;
                        y2 = coord.Y;
                    }
                    else
                    {
                        y1 = coord.Y;
                        y2 = MultiSelection::LastAddedCoord.Y;
                    }

                    for (int i = x1; i <= x2; i++)
                    {
                        for (int j = y1; j <= y2; j++)
                        {
                            MultiSelection::AddCoord(i, j);
                        }
                    }

                    MultiSelection::LastAddedCoord.X = -1;
                    MultiSelection::LastAddedCoord.Y = -1;
                }
                else
                    MultiSelection::LastAddedCoord = coord;
                CIsoView::GetInstance()->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
            }
            if (CIsoView::CurrentCommand->Type == 4)
            {
                MultiSelection::RemoveCoord(coord.X, coord.Y);
                if (MultiSelection::LastAddedCoord.X > -1 &&
                    MultiSelection::LastAddedCoord.X != coord.X &&
                    MultiSelection::LastAddedCoord.Y != coord.Y
                    )
                {
                    int x1, x2, y1, y2;

                    if (MultiSelection::LastAddedCoord.X < coord.X)
                    {
                        x1 = MultiSelection::LastAddedCoord.X;
                        x2 = coord.X;
                    }
                    else
                    {
                        x1 = coord.X;
                        x2 = MultiSelection::LastAddedCoord.X;
                    }
                    if (MultiSelection::LastAddedCoord.Y < coord.Y)
                    {
                        y1 = MultiSelection::LastAddedCoord.Y;
                        y2 = coord.Y;
                    }
                    else
                    {
                        y1 = coord.Y;
                        y2 = MultiSelection::LastAddedCoord.Y;
                    }

                    for (int i = x1; i <= x2; i++)
                    {
                        for (int j = y1; j <= y2; j++)
                        {
                            MultiSelection::RemoveCoord(i, j);
                        }
                    }

                    MultiSelection::LastAddedCoord.X = -1;
                    MultiSelection::LastAddedCoord.Y = -1;
                }
                else
                    MultiSelection::LastAddedCoord = coord;
                CIsoView::GetInstance()->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
            }
            if (CIsoView::CurrentCommand->Type == 2)
            {
                MultiSelection::Clear();
                CIsoView::GetInstance()->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
            }
            if (CIsoView::CurrentCommand->Type == 5)
            {
                const auto cell = CMapData::Instance->GetCellAt(coord.X, coord.Y);
                int tileset = CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)].TileSet;
                if (tileset >= 0 && tileset < CMapDataExt::TileSet_starts.size() - 1) {
                    for (int j = 0; j < CMapData::Instance->CellDataCount; j++) {
                        for (int i = CMapDataExt::TileSet_starts[tileset]; i < CMapDataExt::TileSet_starts[tileset + 1]; i++) {
                            if (CMapDataExt::GetSafeTileIndex(CMapData::Instance->CellDatas[j].TileIndex) == i) {
                                int x = CMapData::Instance->GetXFromCoordIndex(j);
                                int y = CMapData::Instance->GetYFromCoordIndex(j);
                                MultiSelection::AddCoord(x, y);
                                break;
                            }
                        }
                    }
                }
                CIsoView::GetInstance()->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
            }
            if (CIsoView::CurrentCommand->Type == 6)
            {
                const auto cell = CMapData::Instance->GetCellAt(coord.X, coord.Y);
                int tileset = CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)].TileSet;
                if (tileset >= 0 && tileset < CMapDataExt::TileSet_starts.size() - 1) {
                    for (int j = 0; j < CMapData::Instance->CellDataCount; j++) {
                        for (int i = CMapDataExt::TileSet_starts[tileset]; i < CMapDataExt::TileSet_starts[tileset + 1]; i++) {
                            if (CMapDataExt::GetSafeTileIndex(CMapData::Instance->CellDatas[j].TileIndex) == i) {
                                int x = CMapData::Instance->GetXFromCoordIndex(j);
                                int y = CMapData::Instance->GetYFromCoordIndex(j);
                                MultiSelection::RemoveCoord(x, y);
                                break;
                            }
                        }
                    }
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

static bool CIsoView_Draw_MultiSelect(BGRStruct* pColors, int nCount, bool isOverlay)
{
    bool colorChanged = false;
    auto safeColorBtye = [](int x)
        {
            if (x > 255)
                x = 255;
            if (x < 0)
                x = 0;
            return (byte)x;
        };

    switch (CFinalSunDlgExt::CurrentLighting)
    {
    case 31001:
    case 31002:
    case 31003:
    {
        auto& ret = LightingStruct::CurrentLighting;
        if (isOverlay)
        {
            const auto overlay = CMapData::Instance->GetOverlayAt(
                CMapData::Instance->GetCoordIndex(
                    CIsoViewExt::CurrentDrawCellLocation.X, CIsoViewExt::CurrentDrawCellLocation.Y));
            if (CMapDataExt::IsOre(overlay))
            {
                break;
            }
        }
        const auto lamp = LightingSourceTint::ApplyLamp(CIsoViewExt::CurrentDrawCellLocation.X, CIsoViewExt::CurrentDrawCellLocation.Y);
        float oriAmbMult = ret.Ambient - ret.Ground;
        float newAmbMult = ret.Ambient - ret.Ground + ret.Level * CIsoViewExt::CurrentDrawCellLocation.Height + lamp.AmbientTint;
        newAmbMult = std::clamp(newAmbMult, 0.0f, 2.0f);
        float newRed = ret.Red + lamp.RedTint;
        float newGreen = ret.Green + lamp.GreenTint;
        float newBlue = ret.Blue + lamp.BlueTint;
        newRed = std::clamp(newRed, 0.0f, 2.0f);
        newGreen = std::clamp(newGreen, 0.0f, 2.0f);
        newBlue = std::clamp(newBlue, 0.0f, 2.0f);
        for (int i = 0; i < nCount; ++i)
        {
            // divide (ret.Ambient - ret.Ground) to restore AmbientMult in LightingPalette
            // divide ret.Color to restore ColorMult in LightingPalette

            MultiSelection::ColorHolder[i].R = safeColorBtye(pColors[i].R / oriAmbMult / ret.Red * newAmbMult * newRed);
            MultiSelection::ColorHolder[i].G = safeColorBtye(pColors[i].G / oriAmbMult / ret.Green * newAmbMult * newGreen);
            MultiSelection::ColorHolder[i].B = safeColorBtye(pColors[i].B / oriAmbMult / ret.Blue * newAmbMult * newBlue);
        }
        colorChanged = true;
        break;
    }
    default:
        break;
    }


    if (MultiSelection::IsSelected(CIsoViewExt::CurrentDrawCellLocation.X, CIsoViewExt::CurrentDrawCellLocation.Y))
    {
        if (colorChanged)
        {
            for (int i = 0; i < nCount; ++i)
            {
                MultiSelection::ColorHolder[i].R =
                    (MultiSelection::ColorHolder[i].R * 2 + reinterpret_cast<RGBClass*>(&ExtConfigs::MultiSelectionColor)->R) / 3;
                MultiSelection::ColorHolder[i].G =
                    (MultiSelection::ColorHolder[i].G * 2 + reinterpret_cast<RGBClass*>(&ExtConfigs::MultiSelectionColor)->G) / 3;
                MultiSelection::ColorHolder[i].B =
                    (MultiSelection::ColorHolder[i].B * 2 + reinterpret_cast<RGBClass*>(&ExtConfigs::MultiSelectionColor)->B) / 3;
            }
        }
        else
        {
            for (int i = 0; i < nCount; ++i)
            {
                MultiSelection::ColorHolder[i].R =
                    (pColors[i].R * 2 + reinterpret_cast<RGBClass*>(&ExtConfigs::MultiSelectionColor)->R) / 3;
                MultiSelection::ColorHolder[i].G =
                    (pColors[i].G * 2 + reinterpret_cast<RGBClass*>(&ExtConfigs::MultiSelectionColor)->G) / 3;
                MultiSelection::ColorHolder[i].B =
                    (pColors[i].B * 2 + reinterpret_cast<RGBClass*>(&ExtConfigs::MultiSelectionColor)->B) / 3;
            }
        }

        colorChanged = true;
    }

    return colorChanged;
}

DEFINE_HOOK_AGAIN(46F0D6, CIsoView_Draw_MultiSelect_Tile, 7)
DEFINE_HOOK_AGAIN(46F1B7, CIsoView_Draw_MultiSelect_Tile, 7)
DEFINE_HOOK_AGAIN(46F438, CIsoView_Draw_MultiSelect_Tile, 7)
DEFINE_HOOK_AGAIN(46F55F, CIsoView_Draw_MultiSelect_Tile, 7)
DEFINE_HOOK_AGAIN(46FC2F, CIsoView_Draw_MultiSelect_Tile, 7)
DEFINE_HOOK_AGAIN(46FD0A, CIsoView_Draw_MultiSelect_Tile, 7)
DEFINE_HOOK(46FF71, CIsoView_Draw_MultiSelect_Tile, 7)
{
    GET(BGRStruct*, pColors, ESI);
    GET(int, nCount, ECX);

    if (CIsoView_Draw_MultiSelect(pColors, nCount, false))
        R->ESI(MultiSelection::ColorHolder);

    return 0;
}
DEFINE_HOOK_AGAIN(470081, CIsoView_Draw_MultiSelect_Overlay, 7)
DEFINE_HOOK(470710, CIsoView_Draw_MultiSelect_Overlay, 7)
{
    GET(BGRStruct*, pColors, ESI);
    GET(int, nCount, ECX);

    if (CIsoView_Draw_MultiSelect(pColors, nCount, true))
        R->ESI(MultiSelection::ColorHolder);

    return 0;
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
                                     mapData.GetBuildingData(MultiSelection::CopiedCellsBuilding[dwpos], copyObj);
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
    GET(int, X1, EDX);
    GET(int, Y1, EAX);
    GET(int, X2, EDI);
    GET(int, Y2, ESI);

    if (!MultiSelection::GetCount())
    {
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
                        mapData.GetBuildingData(MultiSelection::CopiedCellsBuilding[dwpos], copyObj);
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
                    mapData.GetBuildingData(MultiSelection::CopiedCellsBuilding[dwpos], copyObj);
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

                MultiSelection::CopiedCells.push_back(cellExt);
            }
        }

    }
    //MultiSelection::CopiedCells.clear();
    return 0;
}

//DEFINE_HOOK(4C3850, CMapData_Paste, 8)
//{
//    if (!CIsoViewExt::PasteGround)
//        return 0x4C3C12;
//    return 0;
//}
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
        auto& ini = CMapData::Instance->INI;

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
                            for (auto coord : MultiSelection::SelectedCoordsTemp)
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
                        for (auto coord : MultiSelection::SelectedCoordsTemp)
                        {
                            //MessageBox(NULL, std::to_string(i).c_str(), std::to_string(j).c_str(), 0);
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
                        if (tCell.Terrain != -1)
                        {
                            int id = tCell.Terrain;
                            int type = tCell.TerrainType;
                            ppmfc::CString name;
                            if (auto pTerrain = CINI::Rules().GetSection("TerrainTypes"))
                            {
                                int indexT = 0;
                                for (auto& pT : pTerrain->GetEntities())
                                {
                                    if (indexT == type)
                                    {
                                        name = pT.second;
                                        break;
                                    }

                                    indexT++;
                                }
                            }
                            CMapData::Instance->SetTerrainData(name, CMapData::Instance->GetCoordIndex(i, j));

                        }
                    if (CIsoViewExt::PasteSmudges)
                        if (tCell.Smudge != -1)
                        {
                            int id = tCell.Smudge;
                            int type = tCell.SmudgeType;
                            ppmfc::CString name;
                            if (auto pTerrain = CINI::Rules().GetSection("SmudgeTypes"))
                            {
                                int indexT = 0;
                                for (auto& pT : pTerrain->GetEntities())
                                {
                                    if (indexT == type)
                                    {
                                        name = pT.second;
                                        break;
                                    }

                                    indexT++;
                                }
                            }
                            CSmudgeData smudge;
                            smudge.X = j;
                            smudge.Y = i;//opposite
                            smudge.Flag = 0;
                            smudge.TypeID = name;
                            auto& Map = CMapData::Instance();
                            Map.SetSmudgeData(&smudge);
                            Map.UpdateFieldSmudgeData(false);

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

DEFINE_HOOK(474FE0, CIsoView_Draw_MultiSelectionMoney, 5)
{
    GET(CIsoViewExt*, pThis, EDI);
    GET_STACK(HDC, hDC, STACK_OFFS(0xD18, 0xC68));
    REF_STACK(RECT, rect, STACK_OFFS(0xD18, 0xCCC));
    LEA_STACK(LPDDSURFACEDESC2, lpDesc, STACK_OFFS(0xD18, 0x92C));

    ::SetBkMode(hDC, OPAQUE);
    ::SetBkColor(hDC, RGB(0xFF, 0xFF, 0xFF));


    if (CIsoViewExt::DrawBounds)
    {
        GET_STACK(CIsoViewExt*, pThis, STACK_OFFS(0xD18, 0xCD4));
        LEA_STACK(LPDDSURFACEDESC2, lpDesc, STACK_OFFS(0xD18, 0x92C));


        auto& map = CINI::CurrentDocument();
        auto size = STDHelpers::SplitString(map.GetString("Map", "Size", "0,0,0,0"));
        auto lSize = STDHelpers::SplitString(map.GetString("Map", "LocalSize", "0,0,0,0"));

        int mapwidth = atoi(size[2]);
        int mapheight = atoi(size[3]);

        int mpL = atoi(lSize[0]);
        int mpT = atoi(lSize[1]);
        int mpW = atoi(lSize[2]);
        int mpH = atoi(lSize[3]);

        int y1 = mpT + mpL - 2 + 3;
        int x1 = mapwidth + mpT - mpL - 3 + 3;


        int y2 = mpT + mpL + mpW - 2 + 3;
        int x2 = mapwidth - mpL - mpW + mpT - 3 + 3;

        CIsoView::MapCoord2ScreenCoord_Flat(x1, y1);
        int drawX1 = x1 - R->Stack<float>(STACK_OFFS(0xD18, 0xCB0));
        int drawY1 = y1 - R->Stack<float>(STACK_OFFS(0xD18, 0xCB8));

        //MessageBox(NULL, std::to_string(R->Stack<float>(STACK_OFFS(0xD18, 0xCB0))).c_str(), std::to_string(R->Stack<float>(STACK_OFFS(0xD18, 0xCB8))).c_str(), 0);

        CIsoView::MapCoord2ScreenCoord_Flat(x2, y2);
        int drawX2 = x2 - R->Stack<float>(STACK_OFFS(0xD18, 0xCB0));
        int drawY2 = y2 - R->Stack<float>(STACK_OFFS(0xD18, 0xCB8));

        pThis->DrawTopRealBorder(drawX1, drawY1 - 15, drawX2, drawY2 - 15, RGB(0, 0, 255), false, false, lpDesc);

    }

    if (!MultiSelection::CopiedCells.empty() && CIsoView::CurrentCommand->Command == 21 && MultiSelection::SelectedCoordsTemp.empty())
    {
        GET_STACK(CIsoViewExt*, pThis2, STACK_OFFS(0xD18, 0xCD4));
        auto point = CIsoView::GetInstance()->GetCurrentMapCoord(CIsoView::GetInstance()->MouseCurrentPosition);
        MapCoord startP;
        auto& mapData = CMapData::Instance();


        startP.X = point.X - MultiSelection::CopiedX / 2;
        startP.Y = point.Y - MultiSelection::CopiedY / 2;
        auto length = CMapData::Instance().MapWidthPlusHeight;

        int copyx = MultiSelection::CopiedX;
        int copyy = MultiSelection::CopiedY;

        //&& startP.Y > 0 && startP.X + MultiSelection::CopiedX < length && startP.Y + MultiSelection::CopiedY < length
        while (startP.X < 0)
        {
            startP.X++;
            copyx--;
        }
        while (startP.Y < 0)
        {
            startP.Y++;
            copyy--;
        }
        while (startP.X + copyx > length)
        {
            copyx--;
        }
        while (startP.Y + copyy > length)
        {
            copyy--;
        }
        LEA_STACK(LPDDSURFACEDESC2, lpDesc, STACK_OFFS(0xD18, 0x92C));

        int x = startP.X;
        int y = startP.Y;
        CIsoView::MapCoord2ScreenCoord(x, y);
        int drawX = x - R->Stack<float>(STACK_OFFS(0xD18, 0xCB0));
        int drawY = y - R->Stack<float>(STACK_OFFS(0xD18, 0xCB8));
        pThis2->DrawLockedCellOutline(drawX, drawY, copyy, copyx, ExtConfigs::CursorSelectionBound_Color, false, false, lpDesc);

    }
    else if (!MultiSelection::CopiedCells.empty() && CIsoView::CurrentCommand->Command == 21 && !MultiSelection::SelectedCoordsTemp.empty())
    {
        GET_STACK(CIsoViewExt*, pThis2, STACK_OFFS(0xD18, 0xCD4));
        auto point = CIsoView::GetInstance()->GetCurrentMapCoord(CIsoView::GetInstance()->MouseCurrentPosition);
        MapCoord startP;
        auto& mapData = CMapData::Instance();

        startP.X = point.X - MultiSelection::CopiedX / 2;
        startP.Y = point.Y - MultiSelection::CopiedY / 2;
        auto length = CMapData::Instance().MapWidthPlusHeight;
        LEA_STACK(LPDDSURFACEDESC2, lpDesc, STACK_OFFS(0xD18, 0x92C));

        int idx = 0;
        for (int i = startP.X; i < startP.X + MultiSelection::CopiedX; i++)
        {
            for (int j = startP.Y; j < startP.Y + MultiSelection::CopiedY; j++)
            {
                int x = i;
                int y = j;
                auto dwpos = j * CMapData::Instance().MapWidthPlusHeight + i;
                auto& cell = MultiSelection::CopiedCells[idx];
                if (dwpos < mapData.CellDataCount &&  CMapData::Instance->IsCoordInMap(i, j))
                {
                    bool found = false;
                    for (auto coord : MultiSelection::SelectedCoordsTemp)
                    {
                        if (coord.X == cell.X && coord.Y == cell.Y)
                        {

                            found = true;
                            break;
                        }
                    }
                    if (found)
                    {
                        CIsoView::MapCoord2ScreenCoord(x, y);
                        int drawX = x - R->Stack<float>(STACK_OFFS(0xD18, 0xCB0));
                        int drawY = y - R->Stack<float>(STACK_OFFS(0xD18, 0xCB8));


                        pThis2->DrawLockedCellOutline(drawX, drawY, 1, 1, ExtConfigs::CursorSelectionBound_Color, false, false, lpDesc);

                    }
                }
                idx++;
            }
        }
    }



    int leftIndex = 0;

    if (CIsoViewExt::DrawMoneyOnMap)
    {

        ppmfc::CString buffer;
        buffer.Format(Translations::TranslateOrDefault("MoneyOnMap", "Credits On Map: %d"), CMapData::Instance->MoneyCount);
        ::TextOut(hDC, rect.left + 10, rect.top + 10 + 18 * leftIndex++, buffer, buffer.GetLength());


        if (ExtConfigs::EnableMultiSelection)
        {
            if (MultiSelection::GetCount())
            {
                int nCount = 0;
                auto pExt = CMapDataExt::GetExtension();
                pExt->InitOreValue();
                MultiSelection::ApplyForEach(
                    [&nCount, pExt](CellData& cell) {
                        nCount += pExt->GetOreValueAt(cell);
                    }
                );

                buffer.Format(Translations::TranslateOrDefault("MoneyOnMap.MultiSelection", "MultiSelection Enabled. Selected Credits: %d"), nCount);
                ::TextOut(hDC, rect.left + 10, rect.top + 10 + 18 * leftIndex++, buffer, buffer.GetLength());
            }
        }


    }
    if (CFinalSunApp::Instance().FlatToGround)
    {
        ppmfc::CString buffer;
        buffer.Format(Translations::TranslateOrDefault("FlatToGroundModeEnabled", "2D Mode Enabled"));
        ::TextOut(hDC, rect.left + 10, rect.top + 10 + 18 * leftIndex++, buffer, buffer.GetLength());
    }

    SetTextAlign(hDC, TA_LEFT);

    
    return 0x4750B0;
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
