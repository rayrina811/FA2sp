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

std::vector<DrawVeterancies> CIsoViewExt::DrawVeterancies;

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

// Fix on wrong infantry facing
DEFINE_HOOK(473E46, CIsoView_UpdatePaint_InfantryFacing, 9)
{
	GET(const int, nFacing, EAX);
	R->EAX(7 - nFacing / 32);
	R->ECX(R->lea_Stack<DWORD>(0x590));
	return 0x473E52;
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

bool skipThisDraw = false;
DEFINE_HOOK(45AD6A, CIsoView_Draw_CursorSkip, 9)
{
    skipThisDraw = true;
	return 0;
}

DEFINE_HOOK(469C60, CIsoView_DrawCellOutline_CursorSkip, 7)
{
    if (skipThisDraw)
    {
        skipThisDraw = false;
        return 0x469E6A;
    }
	return 0;
}

DEFINE_HOOK(45ADD0, CIsoView_Draw_CursorSelectionBoundHeightColor, 6)
{
    R->Stack<COLORREF>(0x8, ExtConfigs::CursorSelectionBound_HeightColor);
    return 0;
}

DEFINE_HOOK(470194, CIsoView_Draw_LayerVisible_Overlay, 8)
{
	return CIsoViewExt::DrawOverlays ? 0 : 0x470772;
}

/*
DEFINE_HOOK(470BC5, CIsoView_Draw_LayerVisible_Structures, 7)
{

	if (CIsoViewExt::DrawStructures)
	{
		thisDraw = true;
		REF_STACK(CellData, celldata, STACK_OFFS(0xD18, 0xC60));
		if (celldata.Structure != -1)
		{
			CBuildingData data;
			CMapData::Instance->GetBuildingData(celldata.Structure, data);

			if (!CIsoViewExt::DrawStructuresFilter)
				return 0;

			if (!CViewObjectsExt::BuildingBrushDlgBF)
				return 0;

			auto vec = CViewObjectsExt::ObjectFilterB;
			if (!vec.empty())
				if (std::find(vec.begin(), vec.end(), data.TypeID) == vec.end())
				{
					thisDraw = false;
					return 0x470E48;
				}
					

			auto CheckValue = [&](int nCheckBoxIdx, ppmfc::CString& src, ppmfc::CString& dst)
				{
					if (CViewObjectsExt::BuildingBrushBoolsBF[nCheckBoxIdx - 1300])
					{
						if (dst == src) return true;
						else return false;
					}
					return true;
				};
			if (
				CheckValue(1300, CViewObjectsExt::BuildingBrushDlgBF->CString_House, data.House) &&
				CheckValue(1301, CViewObjectsExt::BuildingBrushDlgBF->CString_HealthPoint, data.Health) &&
				CheckValue(1302, CViewObjectsExt::BuildingBrushDlgBF->CString_Direction, data.Facing) &&
				CheckValue(1303, CViewObjectsExt::BuildingBrushDlgBF->CString_Sellable, data.AISellable) &&
				CheckValue(1304, CViewObjectsExt::BuildingBrushDlgBF->CString_Rebuildable, data.AIRebuildable) &&
				CheckValue(1305, CViewObjectsExt::BuildingBrushDlgBF->CString_EnergySupport, data.PoweredOn) &&
				CheckValue(1306, CViewObjectsExt::BuildingBrushDlgBF->CString_UpgradeCount, data.Upgrades) &&
				CheckValue(1307, CViewObjectsExt::BuildingBrushDlgBF->CString_Spotlight, data.SpotLight) &&
				CheckValue(1308, CViewObjectsExt::BuildingBrushDlgBF->CString_Upgrade1, data.Upgrade1) &&
				CheckValue(1309, CViewObjectsExt::BuildingBrushDlgBF->CString_Upgrade2, data.Upgrade2) &&
				CheckValue(1310, CViewObjectsExt::BuildingBrushDlgBF->CString_Upgrade3, data.Upgrade3) &&
				CheckValue(1311, CViewObjectsExt::BuildingBrushDlgBF->CString_AIRepairs, data.AIRepairable) &&
				CheckValue(1312, CViewObjectsExt::BuildingBrushDlgBF->CString_ShowName, data.Nominal) &&
				CheckValue(1313, CViewObjectsExt::BuildingBrushDlgBF->CString_Tag, data.Tag)
				)
			{
				return 0;
			}
				
			else
			{
				thisDraw = false;
				return 0x470E48;
			}
		}

		return 0;
	}
	{
		thisDraw = false;
		return 0x470E48;
	}
}
*/


DEFINE_HOOK(472F33, CIsoView_Draw_LayerVisible_Units, 9)
{
	if (CIsoViewExt::DrawUnits)
	{
		REF_STACK(CellData, celldata, STACK_OFFS(0xD18, 0xC60));
		if (celldata.Unit != -1)
		{
			const auto& filter = CIsoViewExt::VisibleUnits;
			if (!CIsoViewExt::DrawUnitsFilter
				|| filter.find(celldata.Unit) != filter.end())
			{
				CUnitData data;
				CMapData::Instance->GetUnitData(celldata.Unit, data);

				int X = atoi(data.X);
				int Y = atoi(data.Y);
				int	VP = atoi(data.VeterancyPercentage);

				CIsoView::MapCoord2ScreenCoord(X, Y);
				X -= R->Stack<float>(STACK_OFFS(0xD18, 0xCB0));
				Y -= R->Stack<float>(STACK_OFFS(0xD18, 0xCB8));

				DrawVeterancies temp;
				temp.X = X;
				temp.Y = Y;
				temp.VP = VP;
				CIsoViewExt::DrawVeterancies.push_back(temp);
				return 0;
			}
		}
	}
	return 0x47371A;
}


DEFINE_HOOK(47371A, CIsoView_Draw_LayerVisible_Aircrafts, 9)
{
	if (CIsoViewExt::DrawAircrafts)
	{
		REF_STACK(CellData, celldata, STACK_OFFS(0xD18, 0xC60));
		if (celldata.Aircraft != -1)
		{
			const auto& filter = CIsoViewExt::VisibleAircrafts;
			if (!CIsoViewExt::DrawAircraftsFilter
				|| filter.find(celldata.Aircraft) != filter.end())
			{
				CAircraftData data;
				CMapData::Instance->GetAircraftData(celldata.Aircraft, data);

				int X = atoi(data.X);
				int Y = atoi(data.Y);
				int	VP = atoi(data.VeterancyPercentage);

				CIsoView::MapCoord2ScreenCoord(X, Y);
				X -= R->Stack<float>(STACK_OFFS(0xD18, 0xCB0));
				Y -= R->Stack<float>(STACK_OFFS(0xD18, 0xCB8));

				DrawVeterancies temp;
				temp.X = X;
				temp.Y = Y;
				temp.VP = VP;
				CIsoViewExt::DrawVeterancies.push_back(temp);
				return 0;
			}
		}
	}
	return 0x473DA0;
}

bool infantryLoopStart = false;
DEFINE_HOOK(473DA0, CIsoView_Draw_Infantry_FirstLoop, 6)
{
	infantryLoopStart = true;
	return 0;
}

// change drawing sequence from 0~2 to 2~0
DEFINE_HOOK(4741D9, CIsoView_Draw_Infantry_DrawSequence, 8)
{
	GET(int, pos, EBX);
	R->EBX(pos - 1);
	R->Stack(STACK_OFFS(0xD18, 0xCE8), pos - 1);

	if (pos - 1 < 0)
		return 0x4741E7;

	return 0x473DAA;
}

DEFINE_HOOK(473DAA, CIsoView_Draw_LayerVisible_Infantries, 9)
{
	GET(int, subPos, EBX);
	if (infantryLoopStart)
	{
		R->EBX(2);
		R->Stack(STACK_OFFS(0xD18, 0xCE8), 2);
		subPos = 2;
		infantryLoopStart = false;
	}

	if (CIsoViewExt::DrawInfantries)
	{
		REF_STACK(CellData, celldata, STACK_OFFS(0xD18, 0xC60));

		if (celldata.Infantry[subPos] != -1)
		{
			const auto& filter = CIsoViewExt::VisibleInfantries;
			if (!CIsoViewExt::DrawInfantriesFilter
				|| filter.find(celldata.Infantry[subPos]) != filter.end())
			{
				CInfantryData data;
				CMapData::Instance->GetInfantryData(celldata.Infantry[subPos], data);

				int X = atoi(data.X);
				int Y = atoi(data.Y);
				int	VP = atoi(data.VeterancyPercentage);

				CIsoView::MapCoord2ScreenCoord(X, Y);
				X -= R->Stack<float>(STACK_OFFS(0xD18, 0xCB0));
				Y -= R->Stack<float>(STACK_OFFS(0xD18, 0xCB8));

				int subCell = atoi(data.SubCell);
				if (subCell == 2)
					X += 15;
				else if (subCell == 3)
					X -= 15;
				else if (subCell == 4)
					Y += 7;

				DrawVeterancies temp;
				temp.X = X - 5;
				temp.Y = Y - 4;
				temp.VP = VP;
				CIsoViewExt::DrawVeterancies.push_back(temp);
				return 0;
			}
		}
	}
	return 0x4741D9;
}

DEFINE_HOOK(4741E7, CIsoView_Draw_LayerVisible_Terrains, 9)
{
	return CIsoViewExt::DrawTerrains ? 0 : 0x474563;
}
/*
DEFINE_HOOK(474563, CIsoView_Draw_LayerVisible_Smudges, 9)
{
	return CIsoViewExt::DrawSmudges ? 0 : 0x4748DC;
}

DEFINE_HOOK(471162, CIsoView_Draw_PowerUp1Loc_PosFix, 5)
{
	REF_STACK(const ppmfc::CString, ID, STACK_OFFS(0xD18, 0xBFC));
	REF_STACK(const ppmfc::CString, BldID, STACK_OFFS(0xD18, 0xC08));
	GET_STACK(int, X, STACK_OFFS(0xD18, 0xCFC));
	GET_STACK(int, Y, STACK_OFFS(0xD18, 0xD00));

	auto pBldData = ImageDataMapHelper::GetImageDataFromMap(CLoadingExt::GetImageName(BldID, 0));
	auto pData = ImageDataMapHelper::GetImageDataFromMap(CLoadingExt::GetImageName(ID, 0));

	ppmfc::CString ArtID; // BldArtID
	if (auto ppImage = Variables::Rules.TryGetString(BldID, "Image")) {
		ppmfc::CString str = *ppImage;
		str.Trim();
		ArtID = str;
	}
	else
		ArtID = BldID;

	X += CINI::Art->GetInteger(ArtID, "PowerUp1LocXX", 0);
	Y += CINI::Art->GetInteger(ArtID, "PowerUp1LocYY", 0);

	X += (pBldData->FullWidth - pData->FullWidth) / 2;
	Y += (pBldData->FullHeight - pData->FullHeight) / 2;

	R->ESI(X);
	R->EDI(Y);

	return 0x47141D;
}

DEFINE_HOOK(471980, CIsoView_Draw_PowerUp2Loc_PosFix, 5)
{
	REF_STACK(const ppmfc::CString, ID, STACK_OFFS(0xD18, 0xBFC));
	REF_STACK(const ppmfc::CString, BldID, STACK_OFFS(0xD18, 0xC08));
	GET_STACK(int, X, STACK_OFFS(0xD18, 0xCFC));
	GET_STACK(int, Y, STACK_OFFS(0xD18, 0xD00));

	auto pBldData = ImageDataMapHelper::GetImageDataFromMap(CLoadingExt::GetImageName(BldID, 0));
	auto pData = ImageDataMapHelper::GetImageDataFromMap(CLoadingExt::GetImageName(ID, 0));

	ppmfc::CString ArtID; // BldArtID
	if (auto ppImage = Variables::Rules.TryGetString(BldID, "Image")) {
		ppmfc::CString str = *ppImage;
		str.Trim();
		ArtID = str;
	}
	else
		ArtID = BldID;

	X += CINI::Art->GetInteger(ArtID, "PowerUp2LocXX", 0);
	Y += CINI::Art->GetInteger(ArtID, "PowerUp2LocYY", 0);

	X += (pBldData->FullWidth - pData->FullWidth) / 2;
	Y += (pBldData->FullHeight - pData->FullHeight) / 2;

	R->ESI(X);
	R->EDI(Y);

	return 0x471BB8;
}

DEFINE_HOOK(4720D3, CIsoView_Draw_PowerUp3Loc_PosFix, 5)
{
	REF_STACK(const ppmfc::CString, ID, STACK_OFFS(0xD18, 0xBFC));
	REF_STACK(const ppmfc::CString, BldID, STACK_OFFS(0xD18, 0xC08));
	GET_STACK(int, X, STACK_OFFS(0xD18, 0xCFC));
	GET_STACK(int, Y, STACK_OFFS(0xD18, 0xD00));

	auto pBldData = ImageDataMapHelper::GetImageDataFromMap(CLoadingExt::GetImageName(BldID, 0));
	auto pData = ImageDataMapHelper::GetImageDataFromMap(CLoadingExt::GetImageName(ID, 0));

	ppmfc::CString ArtID; // BldArtID
	if (auto ppImage = Variables::Rules.TryGetString(BldID, "Image")) {
		ppmfc::CString str = *ppImage;
		str.Trim();
		ArtID = str;
	}
	else
		ArtID = BldID;

	X += CINI::Art->GetInteger(ArtID, "PowerUp3LocXX", 0);
	Y += CINI::Art->GetInteger(ArtID, "PowerUp3LocYY", 0);

	X += (pBldData->FullWidth - pData->FullWidth) / 2;
	Y += (pBldData->FullHeight - pData->FullHeight) / 2;

	R->ESI(X);
	R->EDI(Y);

	return 0x47230B;
}


DEFINE_HOOK(470986, CIsoView_Draw_BuildingImageDataQuery_1, 8)
{
	REF_STACK(ImageDataClass, image, STACK_OFFS(0xD18, 0xAFC));
	REF_STACK(const CBuildingRenderData, data, STACK_OFFS(0xD18, 0xC0C));

	int nFacing = 0;
	if (Variables::Rules.GetBool(data.ID, "Turret"))
		nFacing = 7 - (data.Facing / 32) % 8;

	const int HP = CMapDataExt::CurrentRenderBuildingStrength;
	int status = CLoadingExt::GBIN_NORMAL;
	if (HP == 0)
		status = CLoadingExt::GBIN_RUBBLE;
	else if (static_cast<int>((CMapDataExt::ConditionYellow + 0.001f) * 256) > HP)
		status = CLoadingExt::GBIN_DAMAGED;

	const auto& imageName = CLoadingExt::GetBuildingImageName(data.ID, nFacing, status);
	image = *ImageDataMapHelper::GetImageDataFromMap(imageName);

	CIsoViewDrawTemp::BuildingIndex = R->ESI();

	return 0x4709E1;
}

DEFINE_HOOK(470AE3, CIsoView_Draw_BuildingImageDataQuery_2, 7)
{
	REF_STACK(ImageDataClass, image, STACK_OFFS(0xD18, 0xAFC));
	REF_STACK(const CBuildingRenderData, data, STACK_OFFS(0xD18, 0xC0C));

	int nFacing = 0;
	if (Variables::Rules.GetBool(data.ID, "Turret"))
		nFacing = (7 - data.Facing / 32) % 8;

	const int HP = CMapDataExt::CurrentRenderBuildingStrength;
	int status = CLoadingExt::GBIN_NORMAL;
	if (HP == 0)
		status = CLoadingExt::GBIN_RUBBLE;
	else if (static_cast<int>((CMapDataExt::ConditionYellow + 0.001f) * 256) > HP)
		status = CLoadingExt::GBIN_DAMAGED;

	const auto& imageName = CLoadingExt::GetBuildingImageName(data.ID, nFacing, status);
	image = *ImageDataMapHelper::GetImageDataFromMap(imageName);

	return 0x470B4D;
}

DEFINE_HOOK(4709EE, CIsoView_Draw_ShowBuildingOutline, 6)
{
	GET(CIsoViewExt*, pThis, EDI);
	GET(int, X, EBX);
	GET(int, Y, EBP);
	GET_STACK(int, W, STACK_OFFS(0xD18, 0xCFC));
	GET_STACK(int, H, STACK_OFFS(0xD18, 0xD00));
	GET_STACK(COLORREF, dwColor, STACK_OFFS(0xD18, 0xD04));
	LEA_STACK(LPDDSURFACEDESC2, lpDesc, STACK_OFFS(0xD18, 0x92C));
    lpDescDraw = lpDesc;
	
	const auto& DataExt = CMapDataExt::BuildingDataExts[CIsoViewDrawTemp::BuildingIndex];
	if (DataExt.IsCustomFoundation())
		pThis->DrawLockedLines(*DataExt.LinesToDraw, X, Y, dwColor, false, false, lpDesc);
	else
		pThis->DrawLockedCellOutline(X, Y, W, H, dwColor, false, false, lpDesc);

	return 0x470A38;
}


DEFINE_HOOK(4727B2, CIsoView_Draw_BasenodeOutline_CustomFoundation, B)
{
	CIsoViewDrawTemp::BuildingIndex = R->ESI();
	return 0;
}

DEFINE_HOOK(47280B, CIsoView_Draw_BasenodeOutline, 6)
{
	GET_STACK(CIsoViewExt*, pThis, STACK_OFFS(0xD18, 0xCD4));
	GET(int, X, EBX);
	GET(int, Y, EBP);
	GET_STACK(int, W, STACK_OFFS(0xD18, 0xCFC));
	GET_STACK(int, H, STACK_OFFS(0xD18, 0xD00));
	GET_STACK(COLORREF, dwColor, STACK_OFFS(0xD18, 0xB94));
	LEA_STACK(LPDDSURFACEDESC2, lpDesc, STACK_OFFS(0xD18, 0x92C));

	const auto& DataExt = CMapDataExt::BuildingDataExts[CIsoViewDrawTemp::BuildingIndex];
	if (DataExt.IsCustomFoundation())
	{
		pThis->DrawLockedLines(*DataExt.LinesToDraw, X, Y, dwColor, true, false, lpDesc);
		pThis->DrawLockedLines(*DataExt.LinesToDraw, X + 1, Y, dwColor, true, false, lpDesc);
	}
	else
	{
		pThis->DrawLockedCellOutline(X, Y, W, H, dwColor, true, false, lpDesc);
		pThis->DrawLockedCellOutline(X + 1, Y, W, H, dwColor, true, false, lpDesc);
	}
	
	return 0x472884;
}
*/

DEFINE_HOOK(4748DC, CIsoView_Draw_SkipCelltagAndWaypointDrawing, 9)
{
	return 0x474A91;
}

DEFINE_HOOK(474AE3, CIsoView_Draw_DrawCelltagAndWaypointAndTube_EarlyUnlock, 6)
{
	GET_STACK(CIsoViewExt*, pThis, STACK_OFFS(0xD18, 0xCD4));
	LEA_STACK(LPDDSURFACEDESC2, lpDesc, STACK_OFFS(0xD18, 0x92C));

	pThis->lpDDBackBufferSurface->Unlock(nullptr);
	if (CIsoViewExt::DrawTubes)
	{
		for (const auto& tube : CMapDataExt::Tubes)
		{
			int color = tube.PositiveFacing ? RGB(255, 0, 0) : RGB(0, 0, 255);
			int height = std::min(CMapData::Instance->TryGetCellAt(tube.StartCoord.X, tube.StartCoord.Y)->Height,
				CMapData::Instance->TryGetCellAt(tube.EndCoord.X, tube.EndCoord.Y)->Height);
			height *= 15;
			if (CFinalSunApp::Instance->FlatToGround)
				height = 0;
			for (int i = 0; i < tube.PathCoords.size() - 1; ++i)
			{
				int x1, x2, y1, y2;
				x1 = tube.PathCoords[i].X;
				y1 = tube.PathCoords[i].Y;
				x2 = tube.PathCoords[i + 1].X;
				y2 = tube.PathCoords[i + 1].Y;
				CIsoView::MapCoord2ScreenCoord_Flat(x1, y1);
				CIsoView::MapCoord2ScreenCoord_Flat(x2, y2);
				x1 -= R->Stack<float>(STACK_OFFS(0xD18, 0xCB0));
				y1 -= R->Stack<float>(STACK_OFFS(0xD18, 0xCB8));
				x2 -= R->Stack<float>(STACK_OFFS(0xD18, 0xCB0));
				y2 -= R->Stack<float>(STACK_OFFS(0xD18, 0xCB8));
				if (tube.PositiveFacing)
				{
					x1 += 1;
					y1 += 1;
					x2 += 1;
					y2 += 1;
				}
				else
				{
					x1 -= 1;
					y1 -= 1;
					x2 -= 1;
					y2 -= 1;
				}
				pThis->DrawLine(x1 + 30, y1 - 15 - height, x2 + 30, y2 - 15 - height, color, false, false, lpDesc);
			}
			int x1, x2, y1, y2;
			x1 = tube.StartCoord.X;
			y1 = tube.StartCoord.Y;
			x2 = tube.EndCoord.X;
			y2 = tube.EndCoord.Y;
			CIsoView::MapCoord2ScreenCoord_Flat(x1, y1);
			CIsoView::MapCoord2ScreenCoord_Flat(x2, y2);
			x1 -= R->Stack<float>(STACK_OFFS(0xD18, 0xCB0));
			y1 -= R->Stack<float>(STACK_OFFS(0xD18, 0xCB8));
			x2 -= R->Stack<float>(STACK_OFFS(0xD18, 0xCB0));
			y2 -= R->Stack<float>(STACK_OFFS(0xD18, 0xCB8));
			if (tube.PositiveFacing)
			{
				x1 += 1;
				y1 += 1;
				x2 += 1;
				y2 += 1;
			}
			else
			{
				x1 -= 1;
				y1 -= 1;
				x2 -= 1;
				y2 -= 1;
			}
			pThis->DrawLockedCellOutline(x1, y1 - height, 1, 1, color, true, false, lpDesc);
			pThis->DrawLockedCellOutlineX(x2, y2 - height, 1, 1, color, color, false, false, lpDesc, true);
		}
	}
	if (CIsoViewExt::RockCells)
	{
		auto thisTheater = CINI::CurrentDocument().GetString("Map", "Theater");
		auto Map = &CMapData::Instance();
		for (int i = 0; i < Map->CellDataCount; i++)
		{
			auto cell = &Map->CellDatas[i];
			int x = i % Map->MapWidthPlusHeight;
			int y = i / Map->MapWidthPlusHeight;
			int tileIndex = cell->TileIndex;
			if (tileIndex == 65535)
				tileIndex = 0;


			if (CMapDataExt::TileData && tileIndex < int(CTileTypeClass::InstanceCount()) && cell->TileSubIndex < CMapDataExt::TileData[tileIndex].TileBlockCount)
			{
				auto ttype = CMapDataExt::TileData[tileIndex].TileBlockDatas[cell->TileSubIndex].TerrainType;
				if (ttype == 0x7 || ttype == 0x8 || ttype == 0xf ||
					(cell->Overlay == 0xFF ? false : CMapDataExt::OverlayTypeDatas[cell->Overlay].TerrainRock))
				{
					CIsoView::MapCoord2ScreenCoord(x, y);
					int drawX = x - R->Stack<float>(STACK_OFFS(0xD18, 0xCB0));
					int drawY = y - R->Stack<float>(STACK_OFFS(0xD18, 0xCB8));
					pThis->DrawLockedCellOutlineX(drawX, drawY, 1, 1, RGB(255, 0, 0), RGB(40, 0, 0), true, false, lpDesc);
				}
			}
		}
	}
	if (CTerrainGenerator::RangeFirstCell.X > -1 && CTerrainGenerator::RangeSecondCell.X > -1)
	{
		if (MultiSelection::SelectedCoords.empty())
		{
			int X = CTerrainGenerator::RangeFirstCell.X, Y = CTerrainGenerator::RangeFirstCell.Y;

			if (CMapData::Instance().IsCoordInMap(X, Y))
			{
				int XW = abs(CTerrainGenerator::RangeSecondCell.X - CTerrainGenerator::RangeFirstCell.X) + 1;
				int YW = abs(CTerrainGenerator::RangeSecondCell.Y - CTerrainGenerator::RangeFirstCell.Y) + 1;
				if (X > CTerrainGenerator::RangeSecondCell.X)
					X = CTerrainGenerator::RangeSecondCell.X;
				if (Y > CTerrainGenerator::RangeSecondCell.Y)
					Y = CTerrainGenerator::RangeSecondCell.Y;

				CIsoView::MapCoord2ScreenCoord(X, Y);

				int drawX = X - R->Stack<float>(STACK_OFFS(0xD18, 0xCB0));
				int drawY = Y - R->Stack<float>(STACK_OFFS(0xD18, 0xCB8));

				pThis->DrawLockedCellOutline(drawX, drawY, YW, XW, ExtConfigs::TerrainGeneratorColor, false, false, lpDesc);
			}
		}
		else
		{
			CTerrainGenerator::RangeFirstCell.X = -1;
			CTerrainGenerator::RangeFirstCell.Y = -1;
			CTerrainGenerator::RangeSecondCell.X = -1;
			CTerrainGenerator::RangeSecondCell.Y = -1;
		}
	}
	
	const char* InsigniaVeteran = "FA2spInsigniaVeteran";
	const char* InsigniaElite = "FA2spInsigniaElite";
	auto veteran = ImageDataMapHelper::GetImageDataFromMap(InsigniaVeteran);
	auto elite = ImageDataMapHelper::GetImageDataFromMap(InsigniaElite);
	for (auto& dv : CIsoViewExt::DrawVeterancies)
	{
		if (dv.VP >= 200)
			CIsoViewExt::BlitSHPTransparent(lpDesc, dv.X - elite->FullWidth / 2 + 10, dv.Y + 21 - elite->FullHeight / 2, elite);
		else if (dv.VP >= 100)
			CIsoViewExt::BlitSHPTransparent(lpDesc, dv.X - veteran->FullWidth / 2 + 10, dv.Y + 21 - veteran->FullWidth / 2, veteran);
	}

	return pThis ? 0x474AEF : 0x474DB3;
}

DEFINE_HOOK(474DB7, CIsoView_Draw_DrawCelltagAndWaypointAndTube_SkipOriginUnlock, 6)
{
	GET(CIsoViewExt*, pThis, EBX);

	R->EAX(pThis->lpDDBackBufferSurface);
	R->EBP(&pThis->lpDDBackBufferSurface);

	return 0x474DCE;
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
				auto it = std::find(CLoadingExt::LoadedOverlays.begin(), CLoadingExt::LoadedOverlays.end(), ol.second);
				if (it != CLoadingExt::LoadedOverlays.end()) {
					CLoading::Instance->DrawOverlay(ol.second, oli);
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

DEFINE_HOOK(45ED9E, CIsoView_OnCommand_FoldAnnotations, 5)
{
	GET(int, pos, EAX);
	if (pos < CMapData::Instance->CellDataCount)
	{
		int x = CMapData::Instance->GetXFromCoordIndex(pos);
		int y = CMapData::Instance->GetYFromCoordIndex(pos);
		ppmfc::CString key;
		key.Format("%d", x * 1000 + y);
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
	if (CIsoViewExt::DrawUnits && pThis->CurrentCellObjectIndex < 0)
	{
		pThis->CurrentCellObjectIndex = cell->Unit;
		const auto& filter = CIsoViewExt::VisibleUnits;
		if (CIsoViewExt::DrawUnitsFilter && filter.find(pThis->CurrentCellObjectIndex) == filter.end())
			pThis->CurrentCellObjectIndex = -1;
		pThis->CurrentCellObjectType = 3;
	}
	if (CIsoViewExt::DrawAircrafts && pThis->CurrentCellObjectIndex < 0)
	{
		pThis->CurrentCellObjectIndex = cell->Aircraft;
		const auto& filter = CIsoViewExt::VisibleAircrafts;
		if (CIsoViewExt::DrawAircraftsFilter && filter.find(pThis->CurrentCellObjectIndex) == filter.end())
			pThis->CurrentCellObjectIndex = -1;
		pThis->CurrentCellObjectType = 2;
	}
	if (CIsoViewExt::DrawTerrains && pThis->CurrentCellObjectIndex < 0)
	{
		pThis->CurrentCellObjectIndex = cell->Terrain;
		pThis->CurrentCellObjectType = 4;
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
				CINI::CurrentDocument->DeleteKey("Annotations", key);
			key.Format("%d", X * 1000 + Y);
			CINI::CurrentDocument->WriteString("Annotations", key, value);
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
		((CIsoViewExt*)CIsoView::GetInstance())->ConfirmTube(CIsoView::CurrentCommand->Type == 0);

		CIsoViewExt::IsPressingTube = false;
		::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
		return 0x45EDB6;
	}
	return 0;
}

DEFINE_HOOK(45E880, CIsoView_MapCoord2ScreenCoord_Height, 5)
{
	GET_STACK(int*, X, 0x4);
	GET_STACK(int*, Y, 0x8);
	*X = std::max(0, *X);
	*Y = std::max(0, *Y);
	*X = std::min(CMapData::Instance->MapWidthPlusHeight, *X);
	*Y = std::min(CMapData::Instance->MapWidthPlusHeight, *Y);
	return 0;
}

DEFINE_HOOK(476240, CIsoView_MapCoord2ScreenCoord_Flat, 5)
{
	GET_STACK(int*, X, 0x4);
	GET_STACK(int*, Y, 0x8);
	*X = std::max(0, *X);
	*Y = std::max(0, *Y);
	*X = std::min(CMapData::Instance->MapWidthPlusHeight, *X);
	*Y = std::min(CMapData::Instance->MapWidthPlusHeight, *Y);
	return 0;
}

#define BACK_BUFFER_TO_PRIMARY(hook_addr, hook_name, hook_size, return_addr, special_draw) \
DEFINE_HOOK(hook_addr,hook_name,hook_size) \
{ \
	auto pThis = CIsoView::GetInstance();\
	CRect dr;\
	pThis->GetWindowRect(&dr);\
	if (abs(CIsoViewExt::ScaledFactor - 1.0) <= 0.01) {\
		if (special_draw > -1){\
			CIsoViewExt::SpecialDraw(pThis->lpDDBackBufferSurface, special_draw);\
		}\
		pThis->lpDDPrimarySurface->Blt(&dr, pThis->lpDDBackBufferSurface, &dr, DDBLT_WAIT, 0);\
		return return_addr; \
	}\
	CRect backDr;\
	backDr = dr;\
	backDr.right += backDr.Width() * (CIsoViewExt::ScaledFactor - 1.0);\
	backDr.bottom += backDr.Height() * (CIsoViewExt::ScaledFactor - 1.0);\
	CIsoViewExt::StretchCopySurfaceBilinear(pThis->lpDDBackBufferSurface, backDr,\
		CIsoViewExt::lpDDBackBufferZoomSurface, dr);\
	CIsoViewExt::SpecialDraw(CIsoViewExt::lpDDBackBufferZoomSurface, special_draw);\
	pThis->lpDDPrimarySurface->Blt(&dr, CIsoViewExt::lpDDBackBufferZoomSurface, &dr, DDBLT_WAIT, 0);\
	return return_addr; \
}

BACK_BUFFER_TO_PRIMARY(475150, CIsoView_Draw_BackBufferToPrimary, 6, 0x47517B, 0);
BACK_BUFFER_TO_PRIMARY(459FC8, CIsoView_OnMouseMove_BackBufferToPrimary_1, 6, 0x459FE7, -1);
BACK_BUFFER_TO_PRIMARY(45D05E, CIsoView_OnMouseMove_BackBufferToPrimary_2, 6, 0x45D079, -1);
BACK_BUFFER_TO_PRIMARY(45AEDB, CIsoView_OnMouseMove_BackBufferToPrimary_3, 6, 0x45AEF6, 1);

DEFINE_HOOK(4750E7, CIsoView_Draw_SkipScroll, 5)
{
	return 0x475150;
}

DEFINE_HOOK(460F00, CIsoView_ScreenCoord2MapCoord_Height, 7)
{
	GET_STACK(int*, X, 0x4);
	GET_STACK(int*, Y, 0x8);

	auto pThis = CIsoView::GetInstance();
	CRect dr; 
	GetWindowRect(pThis->GetSafeHwnd(), &dr); 
	*X += (*X - pThis->ViewPosition.x - dr.left) * (CIsoViewExt::ScaledFactor - 1.0);
	*Y += (*Y - pThis->ViewPosition.y - dr.top) * (CIsoViewExt::ScaledFactor - 1.0);
	return 0;
}

DEFINE_HOOK(466890, CIsoView_ScreenCoord2MapCoord_Flat, 8)
{
	GET_STACK(int*, X, 0x4);
	GET_STACK(int*, Y, 0x8);

	auto pThis = CIsoView::GetInstance();
	CRect dr; 
	pThis->GetWindowRect(&dr);
	*X += (*X - pThis->ViewPosition.x - dr.left) * (CIsoViewExt::ScaledFactor - 1.0);
	*Y += (*Y - pThis->ViewPosition.y - dr.top) * (CIsoViewExt::ScaledFactor - 1.0);
	return 0;
}

DEFINE_HOOK(456F37, CIsoView_OnMouseMove_CursorScaled, 8)
{
	auto pThis = CIsoView::GetInstance();
	auto coord = pThis->GetCurrentMapCoord(pThis->MouseCurrentPosition);

	R->EBX(coord.X);
	R->ESI(coord.Y);
	R->Stack(STACK_OFFS(0x3D528, 0x3D4F0), coord.X);
	R->Stack(STACK_OFFS(0x3D528, 0x3D4F8), coord.Y);

	return 0x457207;
}

static CPoint OnLButtonDown_pos;
DEFINE_HOOK(4612F0, CIsoView_OnLButtonDown_Update_Pos, 5)
{
	OnLButtonDown_pos.x = R->Stack<UINT>(0x8);
	OnLButtonDown_pos.y = R->Stack<UINT>(0xC);
	return 0;
}

DEFINE_HOOK(46133D, CIsoView_OnLButtonDown_Scaled_1, 9)
{
	auto coord = CIsoView::GetInstance()->GetCurrentMapCoord(OnLButtonDown_pos);

	R->EDI(coord.X);
	R->Base(-0x50, coord.Y);
	R->Base(-0x80, coord.Y);

	return 0x4615CE;
}

static CPoint OnLButtonUp_pos;
DEFINE_HOOK(466970, CIsoView_OnLButtonUp_Update_Pos, 6)
{
	OnLButtonUp_pos.x = R->Stack<UINT>(0x8);
	OnLButtonUp_pos.y = R->Stack<UINT>(0xC);
	return 0;
}

DEFINE_HOOK(4669A8, CIsoView_OnLButtonUp_Scaled_1, A)
{
	auto coord = CIsoView::GetInstance()->GetCurrentMapCoord(OnLButtonUp_pos);

	R->Stack(STACK_OFFS(0x214, 0x204), coord.X);
	R->EDI(coord.Y);

	return 0x466C6E;
}

DEFINE_HOOK(45AFFC, CIsoView_OnMouseMove_Drag_skip, 7)
{
	return 0x45CD6D;
}

DEFINE_HOOK(4751FA, CIsoView_BltToBackBuffer_Scaled, 7)
{
	GET_STACK(CRect, viewRect, STACK_OFFS(0xC0, 0x8C));

	if (CIsoViewExt::ScaledFactor > 1.0)
	{
		viewRect.right += viewRect.Width() * (CIsoViewExt::ScaledFactor - 1.0);
		viewRect.bottom += viewRect.Height() * (CIsoViewExt::ScaledFactor - 1.0);
	}

	R->Stack(STACK_OFFS(0xC0, 0x8C), viewRect);

	return 0;
}

DEFINE_HOOK(476419, CIsoView_MoveTo, 7)
{
	auto pThis = CIsoView::GetInstance();

	RECT r;
	pThis->GetWindowRect(&r);

	int& height = CMapData::Instance->Size.Height;
	int& width = CMapData::Instance->Size.Width;

	if (pThis->ViewPosition.x < (height / 2 - 4 - r.left / 60) * 60)
		pThis->ViewPosition.x = (height / 2 - 4 - r.left / 60) * 60;
	if (pThis->ViewPosition.x + r.right * CIsoViewExt::ScaledFactor * 0.8 > (height / 2 + width + 7) * 60)
		pThis->ViewPosition.x = (height / 2 + width + 7) * 60 - r.right * CIsoViewExt::ScaledFactor * 0.8;
	if (pThis->ViewPosition.y < (width / 2 - 10 - r.top / 30) * 30)
		pThis->ViewPosition.y = (width / 2 - 10 - r.top / 30) * 30;
	if (pThis->ViewPosition.y + r.bottom * CIsoViewExt::ScaledFactor * 0.8 > (width / 2 + height + 6) * 30)
		pThis->ViewPosition.y = (width / 2 + height + 6) * 30 - r.bottom * CIsoViewExt::ScaledFactor * 0.8;

	SetScrollPos(pThis->GetSafeHwnd(), SB_VERT, pThis->ViewPosition.y / 30 - width / 2 + 4, TRUE);
	SetScrollPos(pThis->GetSafeHwnd(), SB_HORZ, pThis->ViewPosition.x / 60 - height / 2 + 1, TRUE);

	return 0x476571;
}

//LPDIRECTDRAWSURFACE7 lpDDBackBufferSurfaceZoom = nullptr;
//DEFINE_HOOK(4750E7, CIsoView_Draw_Zoom, 5)
//{
//	auto pThis = reinterpret_cast<CFinalSunDlg*>(CFinalSunApp::Instance->m_pMainWnd)->MyViewFrame.pIsoView;
//	auto hWnd = pThis->m_hWnd;
//
//	if (!lpDDBackBufferSurfaceZoom)
//		pThis->lpDD7->DuplicateSurface(pThis->lpDDTempBufferSurface, &lpDDBackBufferSurfaceZoom);
//
//	RECT dr;
//	RECT newDr;
//	GetWindowRect(hWnd ,&dr);
//	newDr = dr;
//	newDr.right = newDr.right - (newDr.right - newDr.left) * 0.25f;
//	newDr.bottom = newDr.bottom - (newDr.bottom - newDr.top) * 0.25f;	
//
//	lpDDBackBufferSurfaceZoom->Blt(&dr, pThis->lpDDBackBufferSurface, &newDr, DDBLT_WAIT, 0);
//	pThis->lpDDBackBufferSurface->Blt(&dr, lpDDBackBufferSurfaceZoom, &dr, DDBLT_WAIT, 0);
//
//	return 0;
//}

