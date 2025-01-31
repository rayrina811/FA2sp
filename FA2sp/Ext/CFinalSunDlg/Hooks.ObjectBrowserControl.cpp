#include "Body.h"

#include <FA2PP.h>
#include <Helpers/Macro.h>

#include "../../FA2sp.h"
#include "../../Miscs/TheaterInfo.h"

#include <CBrushSize.h>
#include <CTileTypeClass.h>
#include <CIsoView.h>
#include "../../Helpers/STDHelpers.h"
#include "../CIsoView/Body.h"
#include "../CMapData/Body.h"
#include "../../ExtraWindow/CTerrainGenerator/CTerrainGenerator.h"

DEFINE_HOOK(51CD20, CViewObjects_Redraw, 7)
{
    if (ExtConfigs::BrowserRedraw)
    {
        CViewObjectsExt::RedrawCalledCount++;
        if (CViewObjectsExt::RedrawCalledCount % 2 == 0)
            return 0x523173;
        GET(CViewObjectsExt*, pThis, ECX);
        pThis->Redraw();
        return 0x523173;
    }
    return 0;
}
DEFINE_HOOK(51B54E, CViewObjects_SkipWaterBrushSize, 6)
{
    return 0x51B5AF;
}

DEFINE_HOOK(51AF40, CViewObjects_OnSelectChanged, 7)
{
    GET_STACK(LPNMTREEVIEW, pNM, 0x4);

    return
        ((CViewObjectsExt*)&CFinalSunDlg::Instance->ViewObjects)->UpdateEngine(pNM->itemNew.lParam) ?
        0x51CC8B :
        0;
}

DEFINE_HOOK(461766, CIsoView_OnLButtonDown_PropertyBrush, 5)
{
    GET(const int, X, EDI);
    GET(const int, Y, ESI);

    if (CIsoView::CurrentCommand->Command == 0x17)
    {
        CViewObjectsExt::ApplyPropertyBrush(X, Y);

        return 0x466860;
    }    
    else if (CIsoView::CurrentCommand->Command == 0x18)
    {
        CViewObjectsExt::ApplyInfantrySubCell(X, Y);

        return 0x466860;
    }          
    else if (CIsoView::CurrentCommand->Command == 0x1A)
    {
        CViewObjectsExt::MoveBaseNode(X, Y);

        return 0x466860;
    } 
    else if (CIsoView::CurrentCommand->Command == 0x1E)
    {
        CViewObjectsExt::PlaceConnectedTile_OnLButtonDown(X, Y);

        return 0x466860;
    }
    else if (CIsoView::CurrentCommand->Command == 0x1F)
    {
        if (CTerrainGenerator::RangeFirstCell.X < 0) {
            CTerrainGenerator::RangeFirstCell.X = X;
            CTerrainGenerator::RangeFirstCell.Y = Y;
            return 0x466860;
        }
        else if (CTerrainGenerator::RangeSecondCell.X < 0) {
            CTerrainGenerator::RangeSecondCell.X = X;
            CTerrainGenerator::RangeSecondCell.Y = Y;
            CTerrainGenerator::OnSetRangeDone();
            return 0x466860;
        }
    }

    return 0;
}

int infantryLoop = 0;
// skip to use our own method;
DEFINE_HOOK(45CD22, CIsoView_OnMouseMove_SkipPlaceObjectAt1, 9)
{
    GET(const int, X, EDI);
    GET(const int, Y, EBX);
    auto& Map = CMapData::Instance();
    auto cell = Map.TryGetCellAt(X, Y);

    if (CViewObjectsExt::PlacingRandomRock >= 0)
    {
        CIsoView::GetInstance()->DrawMouseAttachedStuff(X, Y);
        return 0x45CD2B;
    }
    if (CViewObjectsExt::PlacingRandomSmudge >= 0)
    {
        CSmudgeData smudge;
        smudge.X = Y;
        smudge.Y = X;//opposite
        smudge.Flag = 0;
        smudge.TypeID = CIsoView::CurrentCommand->ObjectID;

        if (cell->Smudge > -1)
            Map.DeleteSmudgeData(cell->Smudge);

        auto& rules = Variables::Rules;
        //check overlapping
        int width = rules.GetInteger(smudge.TypeID, "Width", 1);
        int height = rules.GetInteger(smudge.TypeID, "Height", 1);
        for (auto& thisSmudge : Map.SmudgeDatas)
        {
            if (thisSmudge.X <= 0 || thisSmudge.Y <= 0 || thisSmudge.Flag)
                continue;
            int thisWidth = rules.GetInteger(thisSmudge.TypeID, "Width", 1);
            int thisHeight = rules.GetInteger(thisSmudge.TypeID, "Height", 1);
            int thisX = thisSmudge.Y;
            int thisY = thisSmudge.X;//opposite
            for (int i = 0; i < thisWidth; i++)
                for (int j = 0; j < thisHeight; j++)
                    for (int k = 0; k < width; k++)
                        for (int l = 0; l < height; l++)
                            if (thisY + i == Y + k && thisX + j == X + l)
                                return 0x45CD2B;

        }

        Map.SetSmudgeData(&smudge);
        Map.UpdateFieldSmudgeData(false);
        return 0x45CD2B;
    }   
    if (CViewObjectsExt::PlacingRandomTerrain >= 0)
    {
        if (cell->Terrain > -1)
            Map.DeleteTerrainData(cell->Terrain);
        Map.SetTerrainData(CIsoView::CurrentCommand->ObjectID, Map.GetCoordIndex(X, Y));
        return 0x45CD2B;
    }  
    if (CViewObjectsExt::PlacingRandomInfantry >= 0)
    {
        CInfantryData newTechno;
        if (ExtConfigs::InfantrySubCell_Edit_Place && ExtConfigs::InfantrySubCell_Edit)
        {
            int curIdx = CIsoViewExt::GetSelectedSubcellInfantryIdx(X, Y);
            if (curIdx > -1)
                Map.DeleteInfantryData(curIdx);
            newTechno.SubCell.Format("%d", CIsoViewExt::GetSelectedSubcellInfantryIdx(X, Y, true));
        }
        else
        {
            if (cell->Infantry[infantryLoop] > -1)
            {
                Map.DeleteInfantryData(cell->Infantry[infantryLoop]);
                infantryLoop++;
                if (infantryLoop >= 3)
                    infantryLoop = 0;
            }
                

            newTechno.SubCell = "-1";
        }

        newTechno.Status = "Guard";
        newTechno.Tag = "None";
        newTechno.Facing = "64";
        if (CViewObjectsExt::PlacingRandomRandomFacing)
            newTechno.Facing = STDHelpers::GetRandomFacing();
        newTechno.VeterancyPercentage = "0";
        newTechno.Group = "-1";
        newTechno.IsAboveGround = "0";
        newTechno.AutoNORecruitType = "0";
        newTechno.AutoYESRecruitType = "0";
        newTechno.Health = "256";
        newTechno.House = CIsoView::CurrentHouse(); 
        newTechno.TypeID = CIsoView::CurrentCommand->ObjectID;
        newTechno.X.Format("%d", X);
        newTechno.Y.Format("%d", Y);

        Map.SetInfantryData(&newTechno, nullptr, nullptr, 0, -1);
        return 0x45CD2B;
    }
    if (CViewObjectsExt::PlacingRandomVehicle >= 0)
    {
        CUnitData newTechno;

        if (cell->Unit > -1)
            Map.DeleteUnitData(cell->Unit);

        newTechno.Status = "Guard";
        newTechno.Tag = "None";
        newTechno.Facing = "64";
        if (CViewObjectsExt::PlacingRandomRandomFacing)
            newTechno.Facing = STDHelpers::GetRandomFacing();
        newTechno.VeterancyPercentage = "0";
        newTechno.Group = "-1";
        newTechno.IsAboveGround = "0";
        newTechno.AutoNORecruitType = "0";
        newTechno.AutoYESRecruitType = "0";
        newTechno.Health = "256";
        newTechno.FollowsIndex = "-1";
        newTechno.House = CIsoView::CurrentHouse();
        newTechno.TypeID = CIsoView::CurrentCommand->ObjectID;
        newTechno.X.Format("%d", X);
        newTechno.Y.Format("%d", Y);

        Map.SetUnitData(&newTechno, nullptr, nullptr, 0, "");
        return 0x45CD2B;
    }
    if (CViewObjectsExt::PlacingRandomStructure >= 0)
    {
        CBuildingData newTechno;

        if (cell->Structure > -1)
        {
            CBuildingData building;
            Map.GetBuildingData(cell->Structure, building);
            if (atoi(building.X) == X && (atoi(building.Y) == Y)) // replace onlyif coord is the same
                Map.DeleteBuildingData(cell->Structure);
            else
                return 0x45CD2B;
        }
           
        newTechno.Tag = "None";
        newTechno.Facing = "0";
        newTechno.AISellable = "1";
        newTechno.AIRebuildable = "0";
        newTechno.SpotLight = "0";
        newTechno.AIRepairable = "0";
        if (CViewObjectsExt::PlacingRandomStructureAIRepairs)
            newTechno.AIRepairable = "1";
        newTechno.Nominal = "0";
        newTechno.PoweredOn = "1";
        newTechno.Upgrade1 = "None";
        newTechno.Upgrade2 = "None";
        newTechno.Upgrade3 = "None";
        newTechno.Upgrades = "0";
        newTechno.Health = "256";
        newTechno.House = CIsoView::CurrentHouse();
        newTechno.TypeID = CIsoView::CurrentCommand->ObjectID;
        if (CViewObjectsExt::PlacingRandomRandomFacing)
            newTechno.Facing = STDHelpers::GetRandomFacing();
        newTechno.X.Format("%d", X);
        newTechno.Y.Format("%d", Y);

        Map.SetBuildingData(&newTechno, nullptr, nullptr, 0, "");
        return 0x45CD2B;
    }
    if (CViewObjectsExt::PlacingRandomAircraft >= 0)
    {
        CAircraftData newTechno;

        if (cell->Aircraft > -1)
            Map.DeleteAircraftData(cell->Aircraft);

        newTechno.Status = "Guard";
        newTechno.Tag = "None";
        newTechno.Facing = "64";
        if (CViewObjectsExt::PlacingRandomRandomFacing)
            newTechno.Facing = STDHelpers::GetRandomFacing();
        newTechno.VeterancyPercentage = "0";
        newTechno.Group = "-1";
        newTechno.AutoNORecruitType = "0";
        newTechno.AutoYESRecruitType = "0";
        newTechno.Health = "256";
        newTechno.House = CIsoView::CurrentHouse();
        newTechno.TypeID = CIsoView::CurrentCommand->ObjectID;
        newTechno.X.Format("%d", X);
        newTechno.Y.Format("%d", Y);

        Map.SetAircraftData(&newTechno, nullptr, nullptr, 0, "");
        return 0x45CD2B;
    }
    if (CViewObjectsExt::PlacingWall >= 0)
    {
        int overlay = CViewObjectsExt::PlacingWall / 5;
        int damageLevel = CViewObjectsExt::PlacingWall % 5 - 1;
        if (damageLevel == 3)
            damageLevel = -2;

        for (int i = 0; i < CFinalSunDlg::Instance->MyViewFrame.pIsoView->BrushSizeX; i++)
            for (int j = 0; j < CFinalSunDlg::Instance->MyViewFrame.pIsoView->BrushSizeY; j++)
                CMapDataExt::PlaceWallAt(Map.GetCoordIndex(X + i, Y + j), overlay, damageLevel);
        
        return 0x45CD2B;
    }

    CIsoView::GetInstance()->DrawMouseAttachedStuff(X, Y);
    return 0x45CD2B;
}

DEFINE_HOOK(4F0A40, CTerrainDlg_OnSelchangeTileset, 7)
{
    CViewObjectsExt::InitializeOnUpdateEngine();

    return 0;
}
DEFINE_HOOK(4F3C00, CTileSetBrowserView_OnLButtonDown, 7)
{
    CViewObjectsExt::InitializeOnUpdateEngine();
    return 0;
}

//DEFINE_HOOK(4F17B0, CTerrainDlg_OnSelchangeOverlay, 5)
//{
//    CViewObjectsExt::PlacingWall = -1;
//    return 0;
//}

DEFINE_HOOK(457207, CIsoView_OnMouseMove_Cliff, 5)
{
    auto point = CIsoView::GetInstance()->GetCurrentMapCoord(CIsoView::GetInstance()->MouseCurrentPosition);
    if (CIsoView::CurrentCommand->Command == 0x1E)
    {
        CViewObjectsExt::PlaceConnectedTile_OnMouseMove(point.X, point.Y);
    }
    if (CViewObjectsExt::PlacingRandomRock >= 0)
    {
        if (CIsoView::CurrentCommand->Command != 0x1)
        {
            CViewObjectsExt::PlacingRandomRock = -1;
            return 0;
        }
        std::vector<int> randomList;

        if (auto pSection = CINI::FAData().GetSection("PlaceRandomOverlayList"))
        {
            if (auto pSection2 = CINI::FAData().GetSection(*pSection->GetValueAt(CViewObjectsExt::PlacingRandomRock)))
            {
                for (auto pKey : pSection2->GetEntities())
                {
                    if (pKey.first != "Name" && pKey.first != "BannedTheater")
                    {
                        randomList.push_back(atoi(pKey.second));
                    }
                }
            }
        }
        CIsoView::CurrentCommand->Command = 0x1; // place
        CIsoView::CurrentCommand->Type = 6;
        CIsoView::CurrentCommand->Param = 33;
        CIsoView::CurrentCommand->Overlay = STDHelpers::RandomSelectInt(randomList);
        CIsoView::CurrentCommand->OverlayData = 0;
    }
    if (CViewObjectsExt::PlacingRandomTerrain >= 0)
    {
        if (CIsoView::CurrentCommand->Command != 0x1)
        {
            CViewObjectsExt::PlacingRandomTerrain = -1;
            return 0;
        }
        std::vector<ppmfc::CString> randomList;

        if (auto pSection = CINI::FAData().GetSection("PlaceRandomTreeObList"))
        {
            if (auto pSection2 = CINI::FAData().GetSection(*pSection->GetValueAt(CViewObjectsExt::PlacingRandomTerrain)))
            {
                for (auto pKey : pSection2->GetEntities())
                {
                    if (pKey.first != "Name" && pKey.first != "BannedTheater")
                    {
                        randomList.push_back(pKey.second);
                    }
                }
            }
        }
        CIsoView::CurrentCommand->Command = 0x1; // place
        CIsoView::CurrentCommand->Type = 5;
        CIsoView::CurrentCommand->ObjectID = STDHelpers::RandomSelect(randomList);
    }
    if (CViewObjectsExt::PlacingRandomSmudge >= 0)
    {
        if (CIsoView::CurrentCommand->Command != 0x1)
        {
            CViewObjectsExt::PlacingRandomSmudge = -1;
            return 0;
        }
        std::vector<ppmfc::CString> randomList;

        if (auto pSection = CINI::FAData().GetSection("PlaceRandomSmudgeList"))
        {
            if (auto pSection2 = CINI::FAData().GetSection(*pSection->GetValueAt(CViewObjectsExt::PlacingRandomSmudge)))
            {
                for (auto pKey : pSection2->GetEntities())
                {
                    if (pKey.first != "Name" && pKey.first != "BannedTheater")
                    {
                        randomList.push_back(pKey.second);
                    }
                }
            }
        }
        auto smudgeTypeID = STDHelpers::RandomSelect(randomList);
        CIsoView::CurrentCommand->Command = 0x1; // place
        CIsoView::CurrentCommand->Type = 8;
        CIsoView::CurrentCommand->ObjectID = smudgeTypeID;
    }
    if (CViewObjectsExt::PlacingRandomInfantry >= 0)
    {
        CViewObjectsExt::PlacingRandomRandomFacing = false;
        if (CIsoView::CurrentCommand->Command != 0x1)
        {
            CViewObjectsExt::PlacingRandomInfantry = -1;
            return 0;
        }
        std::vector<ppmfc::CString> randomList;

        if (auto pSection = CINI::FAData().GetSection("PlaceRandomInfantryObList"))
        {
            auto section = *pSection->GetValueAt(CViewObjectsExt::PlacingRandomInfantry);
            if (auto pSection2 = CINI::FAData().GetSection(section))
            {
                for (auto pKey : pSection2->GetEntities())
                {
                    if (pKey.first != "Name" && pKey.first != "BannedTheater" && pKey.first != "RandomFacing")
                    {
                        randomList.push_back(pKey.second);
                    }
                }
                CViewObjectsExt::PlacingRandomRandomFacing = CINI::FAData().GetBool(section, "RandomFacing");
            }
        }
        auto TypeID = STDHelpers::RandomSelect(randomList);
        CIsoView::CurrentCommand->Command = 0x1; // place
        CIsoView::CurrentCommand->Type = 1;
        CIsoView::CurrentCommand->ObjectID = TypeID;
    }
    if (CViewObjectsExt::PlacingRandomStructure >= 0)
    {
        CViewObjectsExt::PlacingRandomRandomFacing = false;
        if (CIsoView::CurrentCommand->Command != 0x1)
        {
            CViewObjectsExt::PlacingRandomStructure = -1;
            return 0;
        }
        std::vector<ppmfc::CString> randomList;

        if (auto pSection = CINI::FAData().GetSection("PlaceRandomBuildingObList"))
        {
            auto section = *pSection->GetValueAt(CViewObjectsExt::PlacingRandomStructure);
            if (auto pSection2 = CINI::FAData().GetSection(section))
            {
                for (auto pKey : pSection2->GetEntities())
                {
                    if (pKey.first != "Name" && pKey.first != "BannedTheater" && pKey.first != "RandomFacing" && pKey.first != "AIRepairs" )
                    {
                        randomList.push_back(pKey.second);
                    }
                }
                CViewObjectsExt::PlacingRandomRandomFacing = CINI::FAData().GetBool(section, "RandomFacing");
                CViewObjectsExt::PlacingRandomStructureAIRepairs = CINI::FAData().GetBool(section, "AIRepairs");
            }
        }
        auto TypeID = STDHelpers::RandomSelect(randomList);
        CIsoView::CurrentCommand->Command = 0x1; // place
        CIsoView::CurrentCommand->Type = 2;
        CIsoView::CurrentCommand->ObjectID = TypeID;
    }
    if (CViewObjectsExt::PlacingRandomAircraft >= 0)
    {
        CViewObjectsExt::PlacingRandomRandomFacing = false;
        if (CIsoView::CurrentCommand->Command != 0x1)
        {
            CViewObjectsExt::PlacingRandomAircraft = -1;
            return 0;
        }
        std::vector<ppmfc::CString> randomList;

        if (auto pSection = CINI::FAData().GetSection("PlaceRandomAircraftObList"))
        {
            auto section = *pSection->GetValueAt(CViewObjectsExt::PlacingRandomAircraft);
            if (auto pSection2 = CINI::FAData().GetSection(section))
            {
                for (auto pKey : pSection2->GetEntities())
                {
                    if (pKey.first != "Name" && pKey.first != "BannedTheater" && pKey.first != "RandomFacing")
                    {
                        randomList.push_back(pKey.second);
                    }
                }
                CViewObjectsExt::PlacingRandomRandomFacing = CINI::FAData().GetBool(section, "RandomFacing");
            }
        }
        auto TypeID = STDHelpers::RandomSelect(randomList);
        CIsoView::CurrentCommand->Command = 0x1; // place
        CIsoView::CurrentCommand->Type = 3;
        CIsoView::CurrentCommand->ObjectID = TypeID;
    }    
    if (CViewObjectsExt::PlacingRandomVehicle >= 0)
    {
        CViewObjectsExt::PlacingRandomRandomFacing = false;
        if (CIsoView::CurrentCommand->Command != 0x1)
        {
            CViewObjectsExt::PlacingRandomVehicle = -1;
            return 0;
        }
        std::vector<ppmfc::CString> randomList;

        if (auto pSection = CINI::FAData().GetSection("PlaceRandomVehicleObList"))
        {
            auto section = *pSection->GetValueAt(CViewObjectsExt::PlacingRandomVehicle);
            if (auto pSection2 = CINI::FAData().GetSection(section))
            {
                for (auto pKey : pSection2->GetEntities())
                {
                    if (pKey.first != "Name" && pKey.first != "BannedTheater" && pKey.first != "RandomFacing")
                    {
                        randomList.push_back(pKey.second);
                    }
                }
                CViewObjectsExt::PlacingRandomRandomFacing = CINI::FAData().GetBool(section, "RandomFacing");
            }
        }
        auto TypeID = STDHelpers::RandomSelect(randomList);
        CIsoView::CurrentCommand->Command = 0x1; // place
        CIsoView::CurrentCommand->Type = 4;
        CIsoView::CurrentCommand->ObjectID = TypeID;
    }
    if (CViewObjectsExt::PlacingWall >= 0)
    {
        if (CIsoView::CurrentCommand->Command != 0x1)
        {
            CViewObjectsExt::PlacingWall = -1;
            return 0;
        }

        CIsoView::CurrentCommand->Command = 0x1; // place
        CIsoView::CurrentCommand->Type = 6;
        CIsoView::CurrentCommand->Param = 33;
        CIsoView::CurrentCommand->Overlay = CViewObjectsExt::PlacingWall / 5;
        //CFinalSunDlg::Instance->BrushSize.nCurSel = 0;
        //CFinalSunDlg::Instance->BrushSize.UpdateData(FALSE);
        //CFinalSunDlg::Instance->MyViewFrame.pIsoView->BrushSizeX = 1;
        //CFinalSunDlg::Instance->MyViewFrame.pIsoView->BrushSizeY = 1;
        if (CViewObjectsExt::PlacingWall % 5 == 0)
        {
            CIsoView::CurrentCommand->Command = 0;
        }
        else if (CViewObjectsExt::PlacingWall % 5 == 4)
        {
            CIsoView::CurrentCommand->OverlayData = 0;
        }
        else
            CIsoView::CurrentCommand->OverlayData = (CViewObjectsExt::PlacingWall % 5 - 1) * 16;
    }

    return 0;
}
DEFINE_HOOK(4347B8, CFinalSunDlg_OnEditUndo, 6)
{
    if (CIsoView::CurrentCommand->Command == 0x1E)
    {
        if (!CViewObjectsExt::CliffConnectionCoordRecords.empty())
        {
            CViewObjectsExt::CliffConnectionCoord = CViewObjectsExt::CliffConnectionCoordRecords.back();
            CViewObjectsExt::CliffConnectionCoordRecords.pop_back();
        }
        if (!CViewObjectsExt::LastPlacedCTRecords.empty())
        {
            CViewObjectsExt::LastPlacedCT = CViewObjectsExt::LastPlacedCTRecords.back();
            CViewObjectsExt::LastPlacedCTRecords.pop_back();
        }
        if (!CViewObjectsExt::LastCTTileRecords.empty())
        {
            CViewObjectsExt::LastCTTile = CViewObjectsExt::LastCTTileRecords.back();
            CViewObjectsExt::LastCTTileRecords.pop_back();
        }
        if (!CViewObjectsExt::LastHeightRecords.empty())
        {
            CViewObjectsExt::CliffConnectionHeight = CViewObjectsExt::LastHeightRecords.back();
            CViewObjectsExt::LastHeightRecords.pop_back();
            CViewObjectsExt::NextCTHeightOffset = 0;
            CViewObjectsExt::HeightChanged = false;
        }
        CViewObjectsExt::LastSuccessfulIndex = -1;
    }
    
    return 0;
}


DEFINE_HOOK(45BF73, CIsoView_OnMouseMove_PropertyBrush, 9)
{
    GET(const int, X, EDI);
    GET(const int, Y, EBX);

    if (CIsoView::CurrentCommand->Command == 0x17)
    {
        CViewObjectsExt::ApplyPropertyBrush(X, Y);

        return 0x45CD6D;
    }    
    else if (CIsoView::CurrentCommand->Command == 0x18)
    {
        CViewObjectsExt::ApplyInfantrySubCell(X, Y);

        return 0x45CD6D;
    }        
    else if (CIsoView::CurrentCommand->Command == 0x1A)
    {
        CViewObjectsExt::MoveBaseNode(X, Y);
        
        return 0x45CD6D;
    }         


    return CIsoView::CurrentCommand->Command == FACurrentCommand::WaypointHandle ? 0x45BF7C : 0x45C168;
}

// Add a house won't update indices, so there might be hidden risks if not reloading the map.
// That's why these hooks are not used.
//DEFINE_HOOK_AGAIN(40A5CB, CINIEditor_Update, 6)
//DEFINE_HOOK_AGAIN(44EB1C, CHouses_ONBNDeleteHouseClicked_UpdateTreeview, 7)
//DEFINE_HOOK(44E320, CHouses_ONBNAddHouseClicked_UpdateTreeview, 7)
//{
//    GlobalVars::Dialogs::CFinalSunDlg->MyViewFrame.pObjectBrowserControl->Update();
//    return 0;
//}

//DEFINE_HOOK(51AFB8, ObjectBrowserControl_OnSelectedChanged, 6)
//{
//    GET_STACK(ObjectBrowserControlExt*, pThis, 0x10);
//    GET(int, nData, ECX);
//    return pThis->UpdateEngine(nData);
//}

//void PrintNode(CTreeCtrl* pTree, HTREEITEM hNode)
//{
//    static int depth = 0;
//    if (pTree->ItemHasChildren(hNode))
//    {
//        HTREEITEM hNextItem;
//        HTREEITEM hChildItem = pTree->GetChildItem(hNode);
//
//        while (hChildItem != NULL)
//        {
//            CString spaces;
//            for (int i = 0; i < depth; ++i)
//                spaces += "    ";
//            Logger::Debug("%s%s %d\n", spaces, pTree->GetItemText(hChildItem), pTree->GetItemData(hChildItem));
//            if (pTree->ItemHasChildren(hChildItem))
//            {
//                ++depth;
//                PrintNode(pTree, hChildItem);
//                --depth;
//            }
//            hNextItem = pTree->GetNextItem(hChildItem, TVGN_NEXT);
//            hChildItem = hNextItem;
//        }
//    }
//}
//
//DEFINE_HOOK(523139, ObjectBrowserControl_Redraw_End, 5)
//{
//    GET(CTreeCtrl*, pTree, ESI);
//    auto hRoot = pTree->GetSelectedItem();
//    
//    PrintNode(pTree, hRoot);
//    
//    return 0;
//}