#include "Body.h"
#include "../../FA2sp.h"

#include <Drawing.h>
#include <CINI.h>
#include <CMapData.h>

#include "../../Miscs/MultiSelection.h"
#include "../../Helpers/STDHelpers.h"

#include "../CLoading/Body.h"
#include "../CMapData/Body.h"

#include "../../Source/CIsoView.h"
#include "../../Helpers/Translations.h"
#include "../CFinalSunDlg/Body.h"
#include <Miscs/Miscs.h>
#include "../../ExtraWindow/CTerrainGenerator/CTerrainGenerator.h"
#include "../../Miscs/Palettes.h"
#include <CShpFile.h>
#include <CMixFile.h>

namespace CIsoViewDrawTemp
{
	int BuildingIndex;
}

/*
* FinalAlert 2 coordinate system just reversed the game's one
* Which means game's (X, Y) is (Y, X) in FinalAlert 2.
* Therefore we just replace the display function here but
* keep use FinalAlert's coordinate system in our codes.
*/
DEFINE_HOOK(45AEFF, CIsoView_OnMouseMove_UpdateCoordinateYXToXY, B)
{
	GET_STACK(int, nPointX, 0x30);
	GET_STACK(int, nPointY, 0x38);
	REF_STACK(ppmfc::CString, lpBuffer, STACK_OFFS(0x3D540, 0x3D4FC));

	lpBuffer.Format("%d / %d - ", nPointX, nPointY);

	R->EBX(nPointX);
	R->EDI(nPointY);

	return 0x45AF76;
}

DEFINE_HOOK(46CB96, CIsoView_DrawMouseAttachedStuff_OverlayAutoConnectionFix, 5)
{
	GET_STACK(CIsoView*, pThis, STACK_OFFS(0x94, 0x84));

	pThis->AutoConnectOverlayAt(R->EBP(), R->EBX());

	return 0x46CC86;
}

DEFINE_HOOK(469A69, CIsoView_CalculateOverlayConnection_OverlayAutoConnectionFix, 8)
{
	GET(unsigned char, nOverlayIndex, ESI);
	GET(bool, bConnectAsWall, ECX);

	enum { CanConnect = 0x469A71, NotAWall = 0x469B07 };

	if (bConnectAsWall)
		return CanConnect;

	if (nOverlayIndex >= 0 && nOverlayIndex < 255)
	{
		char lpKey[4];
		_itoa(nOverlayIndex, lpKey, 10);
		auto pRegName = CINI::Rules->GetString("OverlayTypes", lpKey, "");
		bool bWall = CINI::Rules->GetBool(pRegName, "Wall", false);
		if (bWall)
			return CanConnect;
	}

	return NotAWall;
}

DEFINE_HOOK(459F4F, CIsoView_Draw_CopySelectionBoundColor, 6)
{
	R->Stack<COLORREF>(0x0, ExtConfigs::CopySelectionBound_Color);
	return 0;
}

DEFINE_HOOK(470194, CIsoView_Draw_LayerVisible_Overlay, 8)
{
	return CIsoViewExt::DrawOverlays ? 0 : 0x470772;
}

DEFINE_HOOK(46BDFA, CIsoView_DrawMouseAttachedStuff_Structure, 5)
{
	GET_STACK(const int, X, STACK_OFFS(0x94, -0x4));
	GET_STACK(const int, Y, STACK_OFFS(0x94, -0x8));
	
	const int nMapCoord = CMapData::Instance->GetCoordIndex(X, Y);
	const auto& cell = CMapData::Instance->CellDatas[nMapCoord];
	if (cell.Structure < 0)
		CMapData::Instance->SetBuildingData(nullptr, CIsoView::CurrentCommand->ObjectID, CIsoView::CurrentHouse(), nMapCoord, "");

	return 0x46BF98;
}

DEFINE_HOOK(470502, CIsoView_Draw_OverlayOffset, 5)
{
	REF_STACK(const CellData, cell, STACK_OFFS(0xD18, 0xC60));
	GET(int, nOffset, EAX);

	const int nOverlay = cell.Overlay;
	const unsigned char nOverlayData = cell.OverlayData;

	if (nOverlay == 0xA7)
		nOffset -= 45;
	else if (
		nOverlay != 0x18 && nOverlay != 0x19 && // BRIDGE1, BRIDGE2
		nOverlay != 0x3B && nOverlay != 0x3C && // RAILBRDG1, RAILBRDG2
		nOverlay != 0xED && nOverlay != 0xEE // BRIDGEB1, BRIDGEB2
		)
	{
		if (nOverlay >= 0x27 && nOverlay <= 0x36) // Tracks
			nOffset += 15;
		else if (nOverlay >= 0x4A && nOverlay <= 0x65) // LOBRDG 1-28
			nOffset += 15;
		else if (nOverlay >= 0xCD && nOverlay <= 0xEC) // LOBRDGB 1-4
			nOffset += 15;
		else if (nOverlay < CMapDataExt::OverlayTypeDatas.size())
		{
			if (CMapDataExt::OverlayTypeDatas[nOverlay].Rock 
				//|| CMapDataExt::OverlayTypeDatas[nOverlay].TerrainRock // for compatibility of blockages
				|| CMapDataExt::OverlayTypeDatas[nOverlay].RailRoad)
				nOffset += 15;
		}
	}
	else
	{
		if (nOverlayData >= 0x9 && nOverlayData <= 0x11)
			nOffset -= 16;
		else
			nOffset -= 1;
	}

	R->EAX(nOffset);

	return 0x470574;
}

DEFINE_HOOK(457E9D, CIsoView_OnMouseMove_AutoLAT, 6)
{
    CMapDataExt::SmoothAll();
    return 0x459AA8;
}

DEFINE_HOOK(46242B, CIsoView_OnLBButtonDown_AutoLAT, 6)
{
    CMapDataExt::SmoothAll();
    return 0x463F19;
}

DEFINE_HOOK(45EC07, CIsoView_OnCommand_Skip_wParamCheck, 9)
{
	return 0x45EC10;
}

DEFINE_HOOK(45CDB8, CIsoView_OnMouseMove_SkipOverlayUndoRedo1, 6)
{
	if (!ExtConfigs::UndoRedo_HoldPlaceOverlay)
		return 0;
	return 0x45CDCC;
}

DEFINE_HOOK(45CD4F, CIsoView_OnMouseMove_SkipOverlayUndoRedo2, 6)
{
	if (!ExtConfigs::UndoRedo_HoldPlaceOverlay)
		return 0;
	return 0x45CD6D;
}

DEFINE_HOOK(46684D, CIsoView_OnLButtonDown_OverlayUndoRedo, 6)
{
	if (!ExtConfigs::UndoRedo_HoldPlaceOverlay)
		return 0;

	if (CIsoView::CurrentCommand->Command == 0x1 && CIsoView::CurrentCommand->Type == 6)
	{
		CMapData::Instance->SaveUndoRedoData(true, 0, 0, 0, 0);
	}

	return 0;
}

bool IsPlacingTiles = false;
DEFINE_HOOK(46855D, CIsoView_OnLButtonUp_OverlayUndoRedo, 5)
{
	IsPlacingTiles = false;

	if (!ExtConfigs::UndoRedo_HoldPlaceOverlay)
		return 0;

	if (CIsoView::CurrentCommand->Command == 0x1 && CIsoView::CurrentCommand->Type == 6)
	{
		CMapData::Instance->SaveUndoRedoData(true, 0, 0, 0, 0);
		CMapData::Instance->DoUndo();
	}

	return 0;
}

DEFINE_HOOK(45B545, CIsoView_OnMouseMove_SkipPlaceTileUndoRedo_Notify, 7)
{
	if (!IsPlacingTiles || !ExtConfigs::UndoRedo_ShiftPlaceTile)
		CMapData::Instance->SaveUndoRedoData(true, 0, 0, 0, 0);
	IsPlacingTiles = true;

	return 0;
}

DEFINE_HOOK(461A37, CIsoView_OnLButtonDown_SkipPlaceTileUndoRedo1, 7)
{
	if (!IsPlacingTiles || !ExtConfigs::UndoRedo_ShiftPlaceTile)
		return 0;
	return 0x461A5B;
}

DEFINE_HOOK(46D620, CIsoView_FillArea, 9)
{
	GET_STACK(int, dwX, 0x4);
	GET_STACK(int, dwY, 0x8);
	GET_STACK(int, dwID, 0xC);
	GET_STACK(byte, bSubTile, 0x10);

	CIsoViewExt::FillArea(dwX, dwY, dwID, bSubTile, dwX, dwY);

	return 0x46D808;
}

DEFINE_HOOK(461A37, CIsoView_PlaceTile_FixUndo, 7)
{
	GET(CIsoView*, pIsoView, EBX);
	GET(int, x, EDI);
	GET(int, ym6, ECX);
	int y = ym6 + 6;
	if (CIsoView::CurrentCommand->Type >= CMapDataExt::TileDataCount || CIsoView::CurrentCommand->Type < 0) {
		return 0;
	}
	auto tiledata = CMapDataExt::TileData[CIsoView::CurrentCommand->Type];

	CMapData::Instance->SaveUndoRedoData(true,
		x - tiledata.Height - 4,
		y - tiledata.Width - 4,
		x - tiledata.Height + pIsoView->BrushSizeX * tiledata.Height + 7,
		y - tiledata.Width + pIsoView->BrushSizeY * tiledata.Width + 7);
	return 0x461A5B;
}

DEFINE_HOOK(461C3E, CIsoView_OnLButtonDown_PlaceTile_SkipHide, 6)
{
	if (!ExtConfigs::PlaceTileSkipHide)
		return 0;

	GET_BASE(int, X, -0x64);
	GET(int, Y, EDX);
	const auto cell = CMapData::Instance->TryGetCellAt(X, Y);
	return cell->IsHidden() ? 0x4622F2 : 0;
}

DEFINE_HOOK(457648, CIsoView_OnMouseMove_PlaceTile_SkipHide, B)
{
	if (!ExtConfigs::PlaceTileSkipHide)
		return 0;

	GET_STACK(int, X, STACK_OFFS(0x3D528, 0x3D4E0));
	GET(int, Y, EAX);
	const auto cell = CMapData::Instance->TryGetCellAt(X, Y);
	return cell->IsHidden() ? 0x457D11 : 0;
}

DEFINE_HOOK(469410, CIsoView_ReInitializeDDraw_ReloadFA2SPHESettings, 6)
{
	auto currentLighting = CFinalSunDlgExt::CurrentLighting;
	Logger::Debug("CIsoView::ReInitializeDDraw(): About to call InitializeAllHdmEdition()\n");
	CMapDataExt::InitializeAllHdmEdition(false, false);
	CViewObjectsExt::Redraw_ConnectedTile(nullptr);
	CFinalSunDlgExt::CurrentLighting = currentLighting;
	// fix desert bug
	auto thisTheater = CINI::CurrentDocument().GetString("Map", "Theater");
	if (thisTheater == "DESERT")
	{
		CTileTypeClass::Instance = &CTileTypeInfo::Desert->Datas;
		CTileTypeClass::InstanceCount = &CTileTypeInfo::Desert->Count;
		CMapDataExt::TileData = CTileTypeInfo::Desert().Datas;
		CMapDataExt::TileDataCount = CTileTypeInfo::Desert().Count;

		if (CFinalSunDlgExt::CurrentLighting == 31000)
		{
			CLoading::Instance()->FreeTMPs();
			CLoading::Instance()->InitTMPs();
		}
	}
	if (CFinalSunDlgExt::CurrentLighting != 31000)
	{
		CheckMenuRadioItem(*CFinalSunDlg::Instance->GetMenu(), 31000, 31003, CFinalSunDlgExt::CurrentLighting, MF_UNCHECKED);
		PalettesManager::ManualReloadTMP = true;
		PalettesManager::CacheAndTintCurrentIso();
		CLoading::Instance->FreeTMPs();
		CLoading::Instance->InitTMPs();
		int oli = 0;
		if (const auto& section = Variables::GetRulesMapSection("OverlayTypes"))
		{
			for (const auto& ol : *section)
			{
				if (CLoadingExt::IsOverlayLoaded(ol.second)) {
					CLoading::Instance->DrawOverlay(ol.second, oli);
					CIsoView::GetInstance()->UpdateDialog(false);
				}
				oli++;
			}
		}

		PalettesManager::RestoreCurrentIso();
		PalettesManager::ManualReloadTMP = false;
		LightingSourceTint::CalculateMapLamps();

		CFinalSunDlg::Instance->MyViewFrame.RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
		auto tmp = CIsoView::CurrentCommand->Command;
		if (CFinalSunDlg::Instance->MyViewFrame.pTileSetBrowserFrame->View.CurrentMode == 1) {
			HWND hParent = CFinalSunDlg::Instance->MyViewFrame.pTileSetBrowserFrame->DialogBar.GetSafeHwnd();
			HWND hTileComboBox = ::GetDlgItem(hParent, 1366);
			::SendMessage(hParent, WM_COMMAND, MAKEWPARAM(1366, CBN_SELCHANGE), (LPARAM)hTileComboBox);
			CIsoView::CurrentCommand->Command = tmp;
		}
	}

	return 0;
}

DEFINE_HOOK(46A362, CIsoView_UpdateStatusBar_BuildingID, 6)
{
	R->ESI(CMapDataExt::StructureIndexMap[R->ESI()]);
	return 0;
}

DEFINE_HOOK(461766, CIsoView_OnLButtonDown_DragObjects, 5)
{
	auto pThis = CIsoView::GetInstance();
	if (CIsoView::CurrentCommand->Command != 0 || pThis->LeftButtonDoubleClick_8C != FALSE || pThis->Drag == TRUE)
	{
		return 0x461964;
	}
	int pos = CMapData::Instance->GetCoordIndex(pThis->StartCell.X, pThis->StartCell.Y);
	auto cell = CMapData::Instance->TryGetCellAt(pos);
	pThis->CurrentCellObjectIndex = -1;
	pThis->CurrentCellObjectType = -1;
	if (CIsoViewExt::DrawInfantries)
	{
		const auto& filter = CIsoViewExt::VisibleInfantries;
		if (!ExtConfigs::InfantrySubCell_Edit)
		{
			pThis->CurrentCellObjectIndex = CMapDataExt::GetInfantryAt(pos);
		}
		else
		{
			pThis->CurrentCellObjectIndex = CIsoViewExt::GetSelectedSubcellInfantryIdx(pThis->StartCell.X, pThis->StartCell.Y);
		}
		if (CIsoViewExt::DrawInfantriesFilter && filter.find(pThis->CurrentCellObjectIndex) == filter.end())
			pThis->CurrentCellObjectIndex = -1;
		pThis->CurrentCellObjectType = 0;
	}
	if (CIsoViewExt::DrawAircrafts && pThis->CurrentCellObjectIndex < 0)
	{
		pThis->CurrentCellObjectIndex = cell->Aircraft;
		const auto& filter = CIsoViewExt::VisibleAircrafts;
		if (CIsoViewExt::DrawAircraftsFilter && filter.find(pThis->CurrentCellObjectIndex) == filter.end())
			pThis->CurrentCellObjectIndex = -1;
		pThis->CurrentCellObjectType = 2;
	}
	if (CIsoViewExt::DrawUnits && pThis->CurrentCellObjectIndex < 0)
	{
		pThis->CurrentCellObjectIndex = cell->Unit;
		const auto& filter = CIsoViewExt::VisibleUnits;
		if (CIsoViewExt::DrawUnitsFilter && filter.find(pThis->CurrentCellObjectIndex) == filter.end())
			pThis->CurrentCellObjectIndex = -1;
		pThis->CurrentCellObjectType = 3;
	}
	if (CIsoViewExt::DrawStructures && pThis->CurrentCellObjectIndex < 0)
	{
		pThis->CurrentCellObjectIndex = cell->Structure;
		pThis->CurrentCellObjectType = 1;
		if (cell->Structure < CMapDataExt::StructureIndexMap.size())
		{
			auto StrINIIndex = CMapDataExt::StructureIndexMap[cell->Structure];
			if (StrINIIndex != -1)
			{
				const auto& filter = CIsoViewExt::VisibleStructures;
				if (CIsoViewExt::DrawStructuresFilter && filter.find(StrINIIndex) == filter.end())
					pThis->CurrentCellObjectIndex = -1;
				else
				{
					const auto& objRender = CMapDataExt::BuildingRenderDatasFix[StrINIIndex];
					pThis->StartCell.X = objRender.X;
					pThis->StartCell.Y = objRender.Y;
				}
			}
			else
			{
				pThis->CurrentCellObjectIndex = -1;
			}
		}
		else
		{
			pThis->CurrentCellObjectIndex = -1;
		}
	}
	if (CIsoViewExt::DrawTerrains && pThis->CurrentCellObjectIndex < 0)
	{
		pThis->CurrentCellObjectIndex = cell->Terrain;
		pThis->CurrentCellObjectType = 4;
	}
	if (CIsoViewExt::DrawWaypoints && pThis->CurrentCellObjectIndex < 0)
	{
		pThis->CurrentCellObjectIndex = cell->Waypoint;
		pThis->CurrentCellObjectType = 6;
	}
	if (CIsoViewExt::DrawCelltags && pThis->CurrentCellObjectIndex < 0)
	{
		pThis->CurrentCellObjectIndex = cell->CellTag;
		pThis->CurrentCellObjectType = 5;
	}
	if (CIsoViewExt::DrawBasenodes && pThis->CurrentCellObjectIndex < 0)
	{
		if (cell->BaseNode.BasenodeID > -1)
		{
			auto& cellExt = CMapDataExt::CellDataExts[pos];
			const auto& node = cellExt.BaseNodes[0];
			pThis->CurrentCellObjectIndex = cell->BaseNode.BasenodeID;
			CIsoViewExt::CurrentCellObjectHouse = cell->BaseNode.House;
			pThis->StartCell.X = node.X;
			pThis->StartCell.Y = node.Y;
			pThis->CurrentCellObjectType = 8;
		}
	}
	if (CIsoViewExt::DrawSmudges && pThis->CurrentCellObjectIndex < 0)
	{
		pThis->CurrentCellObjectIndex = cell->Smudge;
		pThis->CurrentCellObjectType = 9;
	}
	if (pThis->CurrentCellObjectIndex < 0)
	{
		if (CMapDataExt::HasAnnotation(pos))
		{
			pThis->CurrentCellObjectIndex = pos;
			pThis->CurrentCellObjectType = 7;
		}
	}
	if (pThis->CurrentCellObjectIndex < 0)
	{
		return 0x461964;
	}
	pThis->Drag = TRUE;
	pThis->Moved = FALSE;
	return 0x46686A;
}

UINT nLButtonUpFlags = 0;
DEFINE_HOOK(466970, CIsoView_OnLButtonUp_GetnFlags, 6)
{
	nLButtonUpFlags = R->Stack<UINT>(0x4);
	return 0;
}

DEFINE_HOOK(466DDE, CIsoView_OnLButtonUp_DragOthers, 7)
{
	auto isoView = CIsoView::GetInstance();
	auto& m_id = isoView->CurrentCellObjectIndex;
	auto& m_type = isoView->CurrentCellObjectType;

	//annotation
	if (m_type == 7)
	{
		int X = R->EBX();
		int	Y = R->EDI();

		int oldX = CMapData::Instance->GetXFromCoordIndex(m_id);
		int oldY = CMapData::Instance->GetYFromCoordIndex(m_id);
		ppmfc::CString key;
		key.Format("%d", oldX * 1000 + oldY);
		if (CINI::CurrentDocument->KeyExists("Annotations", key))
		{
			auto value = CINI::CurrentDocument->GetString("Annotations", key);
			if (nLButtonUpFlags != MK_SHIFT)
			{
				auto& cellExt = CMapDataExt::CellDataExts[CMapData::Instance->GetCoordIndex(oldX, oldY)];
				cellExt.HasAnnotation = false;
				CINI::CurrentDocument->DeleteKey("Annotations", key);
			}
			key.Format("%d", X * 1000 + Y);
			CINI::CurrentDocument->WriteString("Annotations", key, value);
			auto& cellExt = CMapDataExt::CellDataExts[CMapData::Instance->GetCoordIndex(X, Y)];
			cellExt.HasAnnotation = true;
		}
		m_id = -1;
		m_type = -1;
	}
	//base nodes
	if (m_type == 8)
	{
		int X = R->EBX();
		int	Y = R->EDI();
		char key[10];
		sprintf(key, "%03d", m_id);
		if (CINI::CurrentDocument->KeyExists(CIsoViewExt::CurrentCellObjectHouse, key))
		{
			auto atoms = STDHelpers::SplitString(CINI::CurrentDocument->GetString(CIsoViewExt::CurrentCellObjectHouse, key), 2);
			ppmfc::CString value;
			value.Format("%s,%d,%d", atoms[0], Y, X);
			if (nLButtonUpFlags == MK_SHIFT)
			{
				int nodeCount = CINI::CurrentDocument->GetInteger(CIsoViewExt::CurrentCellObjectHouse, "NodeCount");
				if (nodeCount < 1000)
				{
					for (int i = 0; i < 1000; ++i)
					{
						sprintf(key, "%03d", i);
						if (!CINI::CurrentDocument->KeyExists(CIsoViewExt::CurrentCellObjectHouse, key))
						{
							break;
						}
					}
					char count[10];
					sprintf(count, "%d", nodeCount + 1);
					CINI::CurrentDocument->WriteString(CIsoViewExt::CurrentCellObjectHouse, "NodeCount", count);
				}
			}
			CINI::CurrentDocument->WriteString(CIsoViewExt::CurrentCellObjectHouse, key, value);
			CMapData::Instance->UpdateFieldBasenodeData(false);
		}
		m_id = -1;
		m_type = -1;
	}
	//smudges
	if (m_type == 9)
	{
		int X = R->EBX();
		int	Y = R->EDI();
		auto smudge = CMapData::Instance->SmudgeDatas[m_id];
		smudge.X = Y;
		smudge.Y = X;
		if (nLButtonUpFlags != MK_SHIFT)
			CMapData::Instance->DeleteSmudgeData(m_id);
		CMapData::Instance->SetSmudgeData(&smudge);
		m_id = -1;
		m_type = -1;
	}
	return 0;
}

DEFINE_HOOK(466E00, CIsoView_OnLButtonUp_DragFacing, 7)
{
	if (nLButtonUpFlags == MK_CONTROL)
	{
		auto Map = &CMapData::Instance();
		auto isoView = CIsoView::GetInstance();
		auto m_id = isoView->CurrentCellObjectIndex;
		auto m_type = isoView->CurrentCellObjectType;

		CBuildingData structure;
		CInfantryData infantry;
		CUnitData unit;
		CAircraftData aircraft;

		int X = R->EBX();
		int	Y = R->EDI();
		MapCoord newMapCoord = { X,Y };

		//order: inf unit air str
		if (m_type == 0)
		{
			Map->GetInfantryData(m_id, infantry);
			auto oldMapCoord = MapCoord{ atoi(infantry.X), atoi(infantry.Y) };
			infantry.Facing = CMapDataExt::GetFacing(oldMapCoord, newMapCoord, infantry.Facing);
			Map->DeleteInfantryData(m_id);
			Map->SetInfantryData(&infantry, NULL, NULL, 0, -1);
		}
		else if (m_type == 3)
		{
			Map->GetUnitData(m_id, unit);
			auto oldMapCoord = MapCoord{ atoi(unit.X), atoi(unit.Y) };
			unit.Facing = CMapDataExt::GetFacing(oldMapCoord, newMapCoord, unit.Facing);
			Map->DeleteUnitData(m_id);
			Map->SetUnitData(&unit, NULL, NULL, 0, "");
		}
		else if (m_type == 2)
		{
			Map->GetAircraftData(m_id, aircraft);
			auto oldMapCoord = MapCoord{ atoi(aircraft.X), atoi(aircraft.Y) };
			aircraft.Facing = CMapDataExt::GetFacing(oldMapCoord, newMapCoord, aircraft.Facing);
			Map->DeleteAircraftData(m_id);
			Map->SetAircraftData(&aircraft, NULL, NULL, 0, "");
		}
		else if (m_type == 1)
		{
			Map->GetBuildingData(m_id, structure);
			auto oldMapCoord = MapCoord{ atoi(structure.X), atoi(structure.Y) };
			structure.Facing = CMapDataExt::GetFacing(oldMapCoord, newMapCoord, structure.Facing);
			CMapDataExt::SkipUpdateBuildingInfo = true;
			Map->DeleteBuildingData(m_id);
			Map->SetBuildingData(&structure, NULL, NULL, 0, "");
		}

		return 0x467682;
	}

	return 0;
}

DEFINE_HOOK(4576C6, CIsoView_OnMouseMove_NoRndForBridge, 6)
{
	GET_STACK(DWORD, dwID, STACK_OFFS(0x3D528, 0x3D450));

	if (dwID < *CTileTypeClass::InstanceCount)
		if (CMapDataExt::TileData[dwID].TileSet == CMapDataExt::WoodBridgeSet)
			return 0x4577F7;

	return 0x4576CC;
}

DEFINE_HOOK(461CDB, CIsoView_OnLButtonDown_NoRndForBridge, 6)
{
	GET(DWORD, dwID6, EDI);
	int dwID = dwID6 >> 6;
	if (dwID < *CTileTypeClass::InstanceCount)
		if (CMapDataExt::TileData[dwID].TileSet == CMapDataExt::WoodBridgeSet)
			return 0x461DEE;

	return 0x461CE1;
}

DEFINE_HOOK(45C0C8, CIsoView_OnMouseMove_PlayerLocation, 6)
{
	auto deleteWaypoint = [](ppmfc::CString key)
		{
			if (auto pSection = CINI::CurrentDocument->GetSection("Waypoints"))
			{
				if (CINI::CurrentDocument->KeyExists("Waypoints", key))
				{
					auto&& value = pSection->GetString(key);
					int x = atoi(value) / 1000;
					int y = atoi(value) % 1000;
					CINI::CurrentDocument->DeleteKey(pSection, key);
					CMapData::Instance->UpdateFieldWaypointData(false);

					if (CMapData::Instance->IsMultiOnly())
					{
						int k, l;
						for (k = -1; k < 2; k++)
							for (l = -1; l < 2; l++)
								CMapData::Instance->UpdateMapPreviewAt(x + k, y + l);
					}
				}
			}
		};
	

	if (CMapData::Instance->IsMultiOnly())
	{
		GET_STACK(int, waypoint, STACK_OFFS(0x3D528, 0x3D518));
		ppmfc::CString key;
		key.Format("%d", waypoint);
		deleteWaypoint(key);
	}
	else
	{
		deleteWaypoint("98");
		deleteWaypoint("99");
	}

	return 0;
}

DEFINE_HOOK(45EBE0, CIsoView_OnCommand_ConfirmTube, 7)
{
	if (CIsoViewExt::IsPressingTube)
	{
		if (CIsoViewExt::TubeNodes.front() == CIsoViewExt::TubeNodes.back())
			return 0;

		((CIsoViewExt*)CIsoView::GetInstance())->ConfirmTube(CIsoView::CurrentCommand->Type == 0);

		CIsoViewExt::IsPressingTube = false;
		::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
		return 0x45EDB6;
	}
	return 0;
}


DEFINE_HOOK(45C850, CIsoView_OnMouseMove_Delete, 5)
{
	auto pIsoView = (CIsoViewExt*)CIsoView::GetInstance();
	auto point = pIsoView->GetCurrentMapCoord(pIsoView->MouseCurrentPosition);
	for (int gx = point.X - pIsoView->BrushSizeX / 2; gx <= point.X + pIsoView->BrushSizeX / 2; gx++)
	{
		for (int gy = point.Y - pIsoView->BrushSizeY / 2; gy <= point.Y + pIsoView->BrushSizeY / 2; gy++)
		{
			if (!CMapDataExt::IsCoordInFullMap(gx, gy))
				continue;

			int nIndex = CMapData::Instance->GetCoordIndex(gx, gy);
			const auto& CellData = CMapData::Instance->CellDatas[nIndex];

			if (CellData.Structure != -1)
				CMapData::Instance->DeleteBuildingData(CellData.Structure);

			if (ExtConfigs::InfantrySubCell_Edit &&
				pIsoView->BrushSizeX == 1 && pIsoView->BrushSizeY == 1)
			{
				int idx = CIsoViewExt::GetSelectedSubcellInfantryIdx(point.X, point.Y);
				if (idx != -1)
				{
					CMapData::Instance->DeleteInfantryData(idx);
				}
			}
			else
			{
				if (CellData.Infantry[0] != -1)
					CMapData::Instance->DeleteInfantryData(CellData.Infantry[0]);
				if (CellData.Infantry[1] != -1)
					CMapData::Instance->DeleteInfantryData(CellData.Infantry[1]);
				if (CellData.Infantry[2] != -1)
					CMapData::Instance->DeleteInfantryData(CellData.Infantry[2]);
			}
			if (CellData.Unit != -1)
				CMapData::Instance->DeleteUnitData(CellData.Unit);

			if (CellData.Aircraft != -1)
				CMapData::Instance->DeleteAircraftData(CellData.Aircraft);
			
			if (CellData.Terrain != -1)
				CMapData::Instance->DeleteTerrainData(CellData.Terrain);
			
			if (CellData.Smudge != -1)
				CMapData::Instance->DeleteSmudgeData(CellData.Smudge);
		}
	}
	::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);

	return 0x45C961;
}

DEFINE_HOOK(45EC1A, CIsoView_OnCommand_HandleProperty, A)
{
	auto pIsoView = CIsoView::GetInstance();
	auto& coord = pIsoView->StartCell;
	auto cell = CMapData::Instance->TryGetCellAt(coord.X, coord.Y);
	int pos = CMapData::Instance->GetCoordIndex(coord.X, coord.Y);

	int index = -1;
	int type = -1;
	if (CIsoViewExt::DrawInfantries)
	{
		const auto& filter = CIsoViewExt::VisibleInfantries;
		if (!ExtConfigs::InfantrySubCell_Edit)
		{
			index = CMapDataExt::GetInfantryAt(pos);
		}
		else
		{
			index = CIsoViewExt::GetSelectedSubcellInfantryIdx(pIsoView->StartCell.X, pIsoView->StartCell.Y);
		}
		if (CIsoViewExt::DrawInfantriesFilter && filter.find(index) == filter.end())
			index = -1;
		type = 0;
	}
	if (CIsoViewExt::DrawAircrafts && index < 0)
	{
		index = cell->Aircraft;
		const auto& filter = CIsoViewExt::VisibleAircrafts;
		if (CIsoViewExt::DrawAircraftsFilter && filter.find(index) == filter.end())
			index = -1;
		type = 2;
	}
	if (CIsoViewExt::DrawUnits && index < 0)
	{
		index = cell->Unit;
		const auto& filter = CIsoViewExt::VisibleUnits;
		if (CIsoViewExt::DrawUnitsFilter && filter.find(index) == filter.end())
			index = -1;
		type = 3;
	}
	if (CIsoViewExt::DrawStructures && index < 0)
	{
		index = cell->Structure;
		type = 1;
		if (cell->Structure < CMapDataExt::StructureIndexMap.size())
		{
			auto StrINIIndex = CMapDataExt::StructureIndexMap[cell->Structure];
			if (StrINIIndex != -1)
			{
				const auto& filter = CIsoViewExt::VisibleStructures;
				if (CIsoViewExt::DrawStructuresFilter && filter.find(StrINIIndex) == filter.end())
					index = -1;
				else
				{
					const auto& objRender = CMapDataExt::BuildingRenderDatasFix[StrINIIndex];
					pIsoView->StartCell.X = objRender.X;
					pIsoView->StartCell.Y = objRender.Y;
				}
			}
			else
			{
				index = -1;
			}
		}
		else
		{
			index = -1;
		}
	}
	if (index > -1 && type > -1)
	{
		if (type == 0 && !ExtConfigs::InfantrySubCell_Edit)
		{
			for (int i = 0; i < 3; ++i)
			{
				if (cell->Infantry[i] > -1)
					pIsoView->HandleProperties(cell->Infantry[i], 0);
			}
		}
		else
			pIsoView->HandleProperties(index, type);
	}

	if (pos < CMapData::Instance->CellDataCount)
	{
		ppmfc::CString key;
		key.Format("%d", coord.X * 1000 + coord.Y);
		if (CINI::CurrentDocument->KeyExists("Annotations", key))
		{
			auto atoms = STDHelpers::SplitString(CINI::CurrentDocument->GetString("Annotations", key), 6);
			ppmfc::CString value;
			bool folded = STDHelpers::IsTrue(atoms[2]);
			for (int i = 0; i < atoms.size(); ++i)
			{
				if (i != 0)
					value += ",";
				if (i != 2)
					value += atoms[i];
				else
				{
					if (folded)
						value += "no";
					else
						value += "yes";
				}
			}
			CINI::CurrentDocument->WriteString("Annotations", key, value);
			::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
		}
	}

	return 0x45ED9E;
}

