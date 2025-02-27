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

struct DrawVeterancy
{
	int X;
	int Y;
	int VP;
};

std::vector<DrawVeterancy> drawVeterancies;
LPDDSURFACEDESC2 lpDescDraw;

namespace CIsoViewDrawTemp
{
	float ConditionYellow = 0.67f;
}

namespace CIsoViewDrawTemp
{
	int BuildingIndex;
}

DEFINE_HOOK(46DE00, CIsoView_Draw_InitDrawData, 7)
{
	CIsoViewDrawTemp::ConditionYellow = Variables::Rules.GetSingle("AudioVisual", "ConditionYellow", 0.67f);
	drawVeterancies.clear();
	return 0;
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

//DEFINE_HOOK(45AD81, CIsoView_Draw_CursorSelectionBoundColor, 5)
//{
//	if (ExtConfigs::CursorSelectionBound_AutoColor)
//	{
//		auto point = CIsoView::GetInstance()->GetCurrentMapCoord(CIsoView::GetInstance()->MouseCurrentPosition);
//		auto cell = CMapData::Instance().GetCellAt(point.X + point.Y * CMapData::Instance().MapWidthPlusHeight);
//		R->Stack<COLORREF>(0x0, CIsoViewExt::_cell_hilight_colors[cell->Height]);
//	}
//	else
//		R->Stack<COLORREF>(0x0, ExtConfigs::CursorSelectionBound_Color);
//
//
//		//CIsoView::GetInstance()->Draw();
//
//	return 0;
//}

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

bool thisDraw = false;

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

DEFINE_HOOK(4725CB, CIsoView_Draw_LayerVisible_Basenodes, 8)
{
	if (CIsoViewExt::DrawBasenodes)
	{
		if (!CIsoViewExt::DrawBasenodesFilter)
			return 0;

		if (!CViewObjectsExt::BuildingBrushDlgBNF)
			return 0;

		REF_STACK(CellData, celldata, STACK_OFFS(0xD18, 0xC60));
		if (celldata.BaseNode.BasenodeID != -1)
		{
			char key[10];
			sprintf(key, "%03d", celldata.BaseNode.BasenodeID);

			auto bID = STDHelpers::SplitString(CMapData::Instance->INI.GetString(celldata.BaseNode.House, key))[0];
			auto vec = CViewObjectsExt::ObjectFilterBN;
			if (!vec.empty())
				if (std::find(vec.begin(), vec.end(), bID) == vec.end())
					return 0x472F33;

			auto CheckValue = [&](int nCheckBoxIdx, ppmfc::CString& src, ppmfc::CString& dst)
				{
					if (CViewObjectsExt::BuildingBrushBoolsBNF[nCheckBoxIdx - 1300])
					{
						if (dst == src) return true;
						else return false;
					}
					return true;
				};
			if (celldata.BaseNode.House)
				if (!CheckValue(1300, CViewObjectsExt::BuildingBrushDlgBNF->CString_House, celldata.BaseNode.House))
					return 0x472F33;
		}
		return 0;
	}
	return 0x472F33;
}

DEFINE_HOOK(472F33, CIsoView_Draw_LayerVisible_Units, 9)
{
	thisDraw = false;
	if (CIsoViewExt::DrawUnits)
	{

		if (!CIsoViewExt::DrawUnitsFilter)
		{
			thisDraw = true;
			return 0;
		}
			

		if (!CViewObjectsExt::VehicleBrushDlgF)
		{
			thisDraw = true;
			return 0;
		}

		REF_STACK(CellData, celldata, STACK_OFFS(0xD18, 0xC60));

		if (celldata.Unit != -1)
		{
			CUnitData data;
			CMapData::Instance->GetUnitData(celldata.Unit, data);

			auto vec = CViewObjectsExt::ObjectFilterV;
			if (!vec.empty())
				if (std::find(vec.begin(), vec.end(), data.TypeID) == vec.end())
					return 0x47371A;

			auto CheckValue = [&](int nCheckBoxIdx, ppmfc::CString& src, ppmfc::CString& dst)
				{
					if (CViewObjectsExt::VehicleBrushBoolsF[nCheckBoxIdx - 1300])
					{
						if (dst == src) return true;
						else return false;
					}
					return true;
				};
			if (
				CheckValue(1300, CViewObjectsExt::VehicleBrushDlgF->CString_House, data.House) &&
				CheckValue(1301, CViewObjectsExt::VehicleBrushDlgF->CString_HealthPoint, data.Health) &&
				CheckValue(1302, CViewObjectsExt::VehicleBrushDlgF->CString_State, data.Status) &&
				CheckValue(1303, CViewObjectsExt::VehicleBrushDlgF->CString_Direction, data.Facing) &&
				CheckValue(1304, CViewObjectsExt::VehicleBrushDlgF->CString_VeteranLevel, data.VeterancyPercentage) &&
				CheckValue(1305, CViewObjectsExt::VehicleBrushDlgF->CString_Group, data.Group) &&
				CheckValue(1306, CViewObjectsExt::VehicleBrushDlgF->CString_OnBridge, data.IsAboveGround) &&
				CheckValue(1307, CViewObjectsExt::VehicleBrushDlgF->CString_FollowerID, data.FollowsIndex) &&
				CheckValue(1308, CViewObjectsExt::VehicleBrushDlgF->CString_AutoCreateNoRecruitable, data.AutoNORecruitType) &&
				CheckValue(1309, CViewObjectsExt::VehicleBrushDlgF->CString_AutoCreateYesRecruitable, data.AutoYESRecruitType) &&
				CheckValue(1310, CViewObjectsExt::VehicleBrushDlgF->CString_Tag, data.Tag)
				)
			{
				thisDraw = true;
				return 0;
			}
			else
			{
				thisDraw = false;
				return 0x47371A;
			}
				
		}
		{
			thisDraw = true;
			return 0;
		}
	}
	{
		thisDraw = false;
		return 0x47371A;
	}
}


DEFINE_HOOK(47371A, CIsoView_Draw_LayerVisible_Units_Veteran, 9)
{
	if (thisDraw && CIsoViewExt::DrawVeterancy)
	{
		GET_STACK(CIsoViewExt*, pThis, STACK_OFFS(0xD18, 0xCD4));
		REF_STACK(CellData, celldata, STACK_OFFS(0xD18, 0xC60));
		pThis->lpDDBackBufferSurface->Unlock(nullptr);
		if (celldata.Unit != -1)
		{
			CUnitData data;
			CMapData::Instance->GetUnitData(celldata.Unit, data);

			int X = atoi(data.X);
			int Y = atoi(data.Y);
			int	VP = atoi(data.VeterancyPercentage);

			CIsoView::MapCoord2ScreenCoord(X, Y);
			X -= R->Stack<float>(STACK_OFFS(0xD18, 0xCB0));
			Y -= R->Stack<float>(STACK_OFFS(0xD18, 0xCB8));


			DrawVeterancy temp;
			temp.X = X;
			temp.Y = Y;
			temp.VP = VP;
			drawVeterancies.push_back(temp);
		}

		thisDraw = false;
	}

	return 0;
}

DEFINE_HOOK(47371A, CIsoView_Draw_LayerVisible_Aircrafts, 9)
{
	if (CIsoViewExt::DrawAircrafts)
	{

		if (!CIsoViewExt::DrawAircraftsFilter)
		{
			thisDraw = true;
			return 0;
		}

		if (!CViewObjectsExt::AircraftBrushDlgF)
		{
			thisDraw = true;
			return 0;
		}

		REF_STACK(CellData, celldata, STACK_OFFS(0xD18, 0xC60));

		if (celldata.Aircraft != -1)
		{
			CAircraftData data;
			CMapData::Instance->GetAircraftData(celldata.Aircraft, data);

			auto vec = CViewObjectsExt::ObjectFilterA;
			if (!vec.empty())
				if (std::find(vec.begin(), vec.end(), data.TypeID) == vec.end())
				{
					thisDraw = false;
					return 0x473DA0;
				}

			auto CheckValue = [&](int nCheckBoxIdx, ppmfc::CString& src, ppmfc::CString& dst)
				{
					if (CViewObjectsExt::AircraftBrushBoolsF[nCheckBoxIdx - 1300])
					{
						if (dst == src) return true;
						else return false;
					}
					return true;
				};
			if (
				CheckValue(1300, CViewObjectsExt::AircraftBrushDlgF->CString_House, data.House) &&
				CheckValue(1301, CViewObjectsExt::AircraftBrushDlgF->CString_HealthPoint, data.Health) &&
				CheckValue(1302, CViewObjectsExt::AircraftBrushDlgF->CString_Direction, data.Facing) &&
				CheckValue(1303, CViewObjectsExt::AircraftBrushDlgF->CString_Status, data.Status) &&
				CheckValue(1304, CViewObjectsExt::AircraftBrushDlgF->CString_VeteranLevel, data.VeterancyPercentage) &&
				CheckValue(1305, CViewObjectsExt::AircraftBrushDlgF->CString_Group, data.Group) &&
				CheckValue(1306, CViewObjectsExt::AircraftBrushDlgF->CString_AutoCreateNoRecruitable, data.AutoNORecruitType) &&
				CheckValue(1307, CViewObjectsExt::AircraftBrushDlgF->CString_AutoCreateYesRecruitable, data.AutoYESRecruitType) &&
				CheckValue(1308, CViewObjectsExt::AircraftBrushDlgF->CString_Tag, data.Tag)
				)
			{
				thisDraw = true;
				return 0;
			}
			else
			{
				thisDraw = false;
				return 0x473DA0;
			}
		}
		{
			thisDraw = true;
			return 0;
		}
	}
	{
		thisDraw = false;
		return 0x473DA0;
	}
}
DEFINE_HOOK(473DA0, CIsoView_Draw_LayerVisible_Aircrafts_Veteran, 6)
{
	if (thisDraw && CIsoViewExt::DrawVeterancy)
	{
		GET_STACK(CIsoViewExt*, pThis, STACK_OFFS(0xD18, 0xCD4));
		REF_STACK(CellData, celldata, STACK_OFFS(0xD18, 0xC60));
		pThis->lpDDBackBufferSurface->Unlock(nullptr);
		if (celldata.Aircraft != -1)
		{
			CAircraftData data;
			CMapData::Instance->GetAircraftData(celldata.Aircraft, data);

			int X = atoi(data.X);
			int Y = atoi(data.Y);
			int	VP = atoi(data.VeterancyPercentage);

			CIsoView::MapCoord2ScreenCoord(X, Y);
			X -= R->Stack<float>(STACK_OFFS(0xD18, 0xCB0));
			Y -= R->Stack<float>(STACK_OFFS(0xD18, 0xCB8));

			DrawVeterancy temp;
			temp.X = X;
			temp.Y = Y;
			temp.VP = VP;
			drawVeterancies.push_back(temp);

		}

		thisDraw = false;
	}

	return 0;
}
DEFINE_HOOK(473DAA, CIsoView_Draw_LayerVisible_Infantries, 9)
{
	thisDraw = false;
	if (CIsoViewExt::DrawInfantries)
	{

		if (!CIsoViewExt::DrawInfantriesFilter)
		{
			thisDraw = true;
			return 0;
		}

		if (!CViewObjectsExt::InfantryBrushDlgF)
		{
			thisDraw = true;
			return 0;
		}

		REF_STACK(CellData, celldata, STACK_OFFS(0xD18, 0xC60));
		GET(int, subPos, EBX)

		if (celldata.Infantry[subPos] != -1)
		{
			CInfantryData data;
			CMapData::Instance->GetInfantryData(celldata.Infantry[subPos], data);

			auto vec = CViewObjectsExt::ObjectFilterI;
			if (!vec.empty())
				if (std::find(vec.begin(), vec.end(), data.TypeID) == vec.end())
				{
					thisDraw = false;
					return 0x4741D9;
				}

			auto CheckValue = [&](int nCheckBoxIdx, ppmfc::CString& src, ppmfc::CString& dst)
				{
					if (CViewObjectsExt::InfantryBrushBoolsF[nCheckBoxIdx - 1300])
					{
						if (dst == src) return true;
						else return false;
					}
					return true;
				};
			if (
				CheckValue(1300, CViewObjectsExt::InfantryBrushDlgF->CString_House, data.House) &&
				CheckValue(1301, CViewObjectsExt::InfantryBrushDlgF->CString_HealthPoint, data.Health) &&
				CheckValue(1302, CViewObjectsExt::InfantryBrushDlgF->CString_State, data.Status) &&
				CheckValue(1303, CViewObjectsExt::InfantryBrushDlgF->CString_Direction, data.Facing) &&
				CheckValue(1304, CViewObjectsExt::InfantryBrushDlgF->CString_VerteranStatus, data.VeterancyPercentage) &&
				CheckValue(1305, CViewObjectsExt::InfantryBrushDlgF->CString_Group, data.Group) &&
				CheckValue(1306, CViewObjectsExt::InfantryBrushDlgF->CString_OnBridge, data.IsAboveGround) &&
				CheckValue(1307, CViewObjectsExt::InfantryBrushDlgF->CString_AutoCreateNoRecruitable, data.AutoNORecruitType) &&
				CheckValue(1308, CViewObjectsExt::InfantryBrushDlgF->CString_AutoCreateYesRecruitable, data.AutoYESRecruitType) &&
				CheckValue(1309, CViewObjectsExt::InfantryBrushDlgF->CString_Tag, data.Tag)
				)
			{
				thisDraw = true;
				return 0;
			}
			else
			{
				thisDraw = false;
				return 0x4741D9;
			}
		}


		{
			thisDraw = true;
			return 0;
		}
	}
	{
		thisDraw = false;
		return 0x4741D9;
	}
}
DEFINE_HOOK(4741D9, CIsoView_Draw_LayerVisible_Infantries_Veteran, 8)
{
	if (thisDraw && CIsoViewExt::DrawVeterancy)
	{
		GET(int, subPos, EBX)
		GET_STACK(CIsoViewExt*, pThis, STACK_OFFS(0xD18, 0xCD4));
		REF_STACK(CellData, celldata, STACK_OFFS(0xD18, 0xC60));
		pThis->lpDDBackBufferSurface->Unlock(nullptr);
		if (celldata.Infantry[subPos] != -1)
		{
			CInfantryData data;
			CMapData::Instance->GetInfantryData(celldata.Infantry[subPos], data);

			int X = atoi(data.X);
			int Y = atoi(data.Y);
			int	VP = atoi(data.VeterancyPercentage);
			int	subcell = atoi(data.SubCell);

			CIsoView::MapCoord2ScreenCoord(X, Y);
			X -= R->Stack<float>(STACK_OFFS(0xD18, 0xCB0));
			Y -= R->Stack<float>(STACK_OFFS(0xD18, 0xCB8));

			if (subcell == 2)
				X += 15;
			else if (subcell == 3)
				X -= 15;
			else if (subcell == 4)
				Y += 7;

			DrawVeterancy temp;
			temp.X = X - 5;
			temp.Y = Y - 4;
			temp.VP = VP;
			drawVeterancies.push_back(temp);

			//if (!ImageDataMapHelper::IsImageLoaded(data.TypeID + "0"))
			//{
			//	pThis->DrawBitmap("infantry", X + 3, Y - 5);
			//}
		}

		thisDraw = false;
	}

	return 0;
}
DEFINE_HOOK(4741E7, CIsoView_Draw_LayerVisible_Terrains, 9)
{
	return CIsoViewExt::DrawTerrains ? 0 : 0x474563;
}

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
	else if (static_cast<int>((CIsoViewDrawTemp::ConditionYellow + 0.001f) * 256) > HP)
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
	else if (static_cast<int>((CIsoViewDrawTemp::ConditionYellow + 0.001f) * 256) > HP)
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

DEFINE_HOOK(4748DC, CIsoView_Draw_SkipCelltagAndWaypointDrawing, 9)
{
	return 0x474A91;
}

DEFINE_HOOK(474AE3, CIsoView_Draw_DrawCelltagAndWaypointAndTube_EarlyUnlock, 6)
{
	GET_STACK(CIsoViewExt*, pThis, STACK_OFFS(0xD18, 0xCD4));

	pThis->lpDDBackBufferSurface->Unlock(nullptr);

	if (CIsoViewExt::RockCells)
	{
		LEA_STACK(LPDDSURFACEDESC2, lpDesc, STACK_OFFS(0xD18, 0x92C));


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
		LEA_STACK(LPDDSURFACEDESC2, lpDesc, STACK_OFFS(0xD18, 0x92C));
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
	else if (CTerrainGenerator::UseMultiSelection && MultiSelection::SelectedCoords.size() > 0) 
	{
		LEA_STACK(LPDDSURFACEDESC2, lpDesc, STACK_OFFS(0xD18, 0x92C));
		for (auto& mc : MultiSelection::SelectedCoords)
		{
			int x = mc.X;
			int y = mc.Y;
			CIsoView::MapCoord2ScreenCoord(x, y);
			int drawX = x - R->Stack<float>(STACK_OFFS(0xD18, 0xCB0));
			int drawY = y - R->Stack<float>(STACK_OFFS(0xD18, 0xCB8));

			bool s1 = true;
			bool s2 = true;
			bool s3 = true;
			bool s4 = true;

			for (auto& coord : MultiSelection::SelectedCoords)
			{
				if (coord.X == mc.X - 1 && coord.Y == mc.Y)
				{
					s1 = false;
				}
				if (coord.X == mc.X + 1 && coord.Y == mc.Y)
				{
					s3 = false;
				}
				if (coord.X == mc.X && coord.Y == mc.Y - 1)
				{
					s4 = false;
				}

				if (coord.X == mc.X && coord.Y == mc.Y + 1)
				{
					s2 = false;
				}
			}
			pThis->DrawLockedCellOutline(drawX, drawY, 1, 1, ExtConfigs::TerrainGeneratorColor, false, false, lpDesc, s1, s2, s3, s4);
		}
	}
	for (auto& dv : drawVeterancies)
	{
		if (dv.VP >= 200)
			pThis->DrawBitmap("elite", dv.X, dv.Y);
		else if (dv.VP >= 100)
			pThis->DrawBitmap("veteran", dv.X, dv.Y);
	}

	return pThis ? 0x474AEF : 0x474DB3;
}

DEFINE_HOOK(474B9D, CIsoView_Draw_DrawCelltagAndWaypointAndTube_DrawStuff, 9)
{
	GET_STACK(HDC, hDC, STACK_OFFS(0xD18, 0xC68));

	GET_STACK(CIsoViewExt*, pThis, STACK_OFFS(0xD18, 0xCD4));
	REF_STACK(CellData, celldata, STACK_OFFS(0xD18, 0xC60));

	CIsoViewExt::drawOffsetX = R->Stack<float>(STACK_OFFS(0xD18, 0xCB0));
	CIsoViewExt::drawOffsetY = R->Stack<float>(STACK_OFFS(0xD18, 0xCB8));

	int X = R->Stack<int>(STACK_OFFS(0xD18, 0xCE4)) - R->Stack<float>(STACK_OFFS(0xD18, 0xCB0));
	int Y = R->Stack<int>(STACK_OFFS(0xD18, 0xCD0)) - R->Stack<float>(STACK_OFFS(0xD18, 0xCB8));

	// We had unlocked it already, just blt them now
	if (CIsoViewExt::DrawCelltags && celldata.CellTag != -1)
	{
		if (CIsoViewExt::DrawCellTagsFilter && !CViewObjectsExt::ObjectFilterCT.empty())
		{
			auto AllisNum = [](std::string str)
				{
					for (int i = 0; i < str.size(); i++)
					{
						int tmp = (int)str[i];
						if (tmp >= 48 && tmp <= 57)
						{
							continue;
						}
						else
						{
							return false;
						}
					}
					return true;
				};
			ppmfc::CString id = "";
			if (CMapData::Instance().INI.SectionExists("CellTags"))
				id = CMapData::Instance().INI.GetStringAt("CellTags", celldata.CellTag);

			if (id != "")
			{
				for (auto name : CViewObjectsExt::ObjectFilterCT)
				{
					if (name == id)
					{
						pThis->DrawCelltag(X, Y);
						break;
					}
					if (AllisNum(std::string(name)))
					{
						int n = atoi(name);
						if (n < 1000000)
						{
							ppmfc::CString buffer;
							buffer.Format("%08d", n + 1000000);
							if (buffer == id)
							{
								pThis->DrawCelltag(X, Y);
								break;
							}
						}
					}
					
				}
			}
		}
		else
			pThis->DrawCelltag(X, Y);
		
		
	}
		
	if (CIsoViewExt::DrawWaypoints && celldata.Waypoint != -1)
		pThis->DrawWaypointFlag(X, Y);
	if (CIsoViewExt::DrawTubes && celldata.Tube != -1)
		pThis->DrawTube(&celldata, X, Y);



	auto cellDataExt = CMapDataExt::CellDataExt_FindCell;
	if (cellDataExt.drawCell)
	{
		SetTextColor(hDC, ExtConfigs::Waypoint_Color);
		if (ExtConfigs::Waypoint_Background)
		{
			SetBkMode(hDC, OPAQUE);
			SetBkColor(hDC, ExtConfigs::Waypoint_Background_Color);
		}
		else
			SetBkMode(hDC, TRANSPARENT);
		SetTextAlign(hDC, TA_CENTER);

		int x = cellDataExt.X;
		int y = cellDataExt.Y;

		CIsoView::MapCoord2ScreenCoord(x, y);

		int drawX = x - R->Stack<float>(STACK_OFFS(0xD18, 0xCB0));
		int drawY = y - R->Stack<float>(STACK_OFFS(0xD18, 0xCB8));

		pThis->DrawBitmap("target", drawX - 20, drawY - 11);
	}

	return 0x474D64;
}

DEFINE_HOOK(474DB7, CIsoView_Draw_DrawCelltagAndWaypointAndTube_SkipOriginUnlock, 6)
{
	GET(CIsoViewExt*, pThis, EBX);

	R->EAX(pThis->lpDDBackBufferSurface);
	R->EBP(&pThis->lpDDBackBufferSurface);

	return 0x474DCE;
}

DEFINE_HOOK(474DDF, CIsoView_Draw_WaypointTexts, 5)
{
	GET_STACK(HDC, hDC, STACK_OFFS(0xD18, 0xC68));

	if (CIsoViewExt::DrawBaseNodeIndex)
	{
		SetTextColor(hDC, ExtConfigs::BaseNodeIndex_Color);
		if (ExtConfigs::BaseNodeIndex_Background)
		{
			SetBkMode(hDC, OPAQUE);
			SetBkColor(hDC, ExtConfigs::BaseNodeIndex_Background_Color);
		}
		else
			SetBkMode(hDC, TRANSPARENT);
		SetTextAlign(hDC, TA_CENTER);

		auto& ini = CMapData::Instance->INI;
		if (auto pSection = ini.GetSection("Houses"))
		{
			for (auto pair : pSection->GetEntities())
			{
				int nodeCount = ini.GetInteger(pair.second, "NodeCount", 0);
				if (nodeCount > 0)
				{
					for (int i = 0; i < nodeCount; i++)
					{
						char key[10];
						sprintf(key, "%03d", i);
						auto value = ini.GetString(pair.second, key, "");
						if (value == "")
							continue;
						auto atoms = STDHelpers::SplitString(value);
						if (atoms.size() < 3)
							continue;

						int x = atoi(atoms[2]);
						int y = atoi(atoms[1]);

						CIsoView::MapCoord2ScreenCoord(x, y);

						int ndrawX = x - R->Stack<float>(STACK_OFFS(0xD18, 0xCB0)) + 30;
						int ndrawY = y - R->Stack<float>(STACK_OFFS(0xD18, 0xCB8)) - 15;

						TextOut(hDC, ndrawX, ndrawY, key, strlen(key));
					}
				}
			}
		}
	}
	

	SetTextColor(hDC, ExtConfigs::Waypoint_Color);
	if (ExtConfigs::Waypoint_Background)
	{
		SetBkMode(hDC, OPAQUE);
		SetBkColor(hDC, ExtConfigs::Waypoint_Background_Color);
	}
	else
		SetBkMode(hDC, TRANSPARENT);
	SetTextAlign(hDC, TA_CENTER);

	if (CIsoViewExt::DrawWaypoints)
	{
		GET(CIsoViewExt*, pThis, EBX);

		GET_STACK(int, jMin, STACK_OFFS(0xD18, 0xC10));
		GET_STACK(int, iMin, STACK_OFFS(0xD18, 0xCBC));
		GET_STACK(const int, jMax, STACK_OFFS(0xD18, 0xC64));
		GET_STACK(const int, iMax, STACK_OFFS(0xD18, 0xC18));

		auto pSection = CINI::CurrentDocument->GetSection("Waypoints");
		for (int j = jMin; j < jMax; ++j)
		{
			for (int i = iMin; i < iMax; ++i)
			{
				int X = j, Y = i;

				CIsoView::MapCoord2ScreenCoord(X, Y);
				auto pCell = CMapData::Instance->TryGetCellAt(j, i);

				int drawX = X - R->Stack<float>(STACK_OFFS(0xD18, 0xCB0)) + 30 + ExtConfigs::Waypoint_Text_ExtraOffset.x;
				int drawY = Y - R->Stack<float>(STACK_OFFS(0xD18, 0xCB8)) - 15 + ExtConfigs::Waypoint_Text_ExtraOffset.y;

				if (pCell->Waypoint != -1)
				{
					auto pWP = *pSection->GetKeyAt(pCell->Waypoint);
					auto pointer = CIsoView::GetInstance()->GetCurrentMapCoord(CIsoView::GetInstance()->MouseCurrentPosition);


					TextOut(hDC, drawX, drawY, pWP, strlen(pWP));

				}
			}
		}
	}
	
	SetTextAlign(hDC, TA_LEFT);
	SetTextColor(hDC, RGB(0, 0, 0));

	return CIsoViewExt::DrawBounds ? 0 : 0x474FE0;
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

DEFINE_HOOK(4676CB, CIsoView_OnLeftButtonUp_AddTube, 6)
{
	GET(CIsoViewExt*, pThis, ESI);
	GET(const int, nDestX, EBX);
	GET(const int, nDestY, EDI);

	pThis->AddTube(pThis->StartCell.X, pThis->StartCell.Y, nDestX, nDestY);

	return 0x468548;
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
			if (CMapDataExt::OverlayTypeDatas[nOverlay].Rock)
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

DEFINE_HOOK(475122, CIsoView_Draw_ScrollCursor, 5)
{
	ppmfc::CString newTitle = "scrollcursor.bmp";
	R->Stack(STACK_OFFS(0xD24, 0xCDC), newTitle);
	return 0;
}
DEFINE_HOOK(475119, CIsoView_Draw_ScrollCursor_Position_Y, 5)
{
	GET(int, y, EDX);

	R->EDX(y - 12);
	return 0;
}
DEFINE_HOOK(475122, CIsoView_Draw_ScrollCursor_Position_X, 5)
{
	GET(int, x, EDX);

	R->EDX(x - 18);
	return 0;
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

	CIsoViewExt::FillArea(dwX, dwY, dwID, bSubTile);

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

DEFINE_HOOK(46EAFA, CIsoView_Draw_TileCurrentCoord_1, 5)
{
	CIsoViewExt::CurrentDrawCellLocation.X = R->EBP();
	CIsoViewExt::CurrentDrawCellLocation.Y = R->EBX();
	CIsoViewExt::CurrentDrawCellLocation.Height = CMapData::Instance->TryGetCellAt(R->EBP(), R->EBX())->Height;
	return 0;
}

DEFINE_HOOK(46F6B4, CIsoView_Draw_TileCurrentCoord_2, 6)
{
	GET(CellData*, pCell, ESI);
	GET(int, X, EDI);
	GET(int, Y, EBP);
	CIsoViewExt::CurrentDrawCellLocation.Height = pCell->Height;
	CIsoViewExt::CurrentDrawCellLocation.X = X;
	CIsoViewExt::CurrentDrawCellLocation.Y = Y;
	return 0;
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



//DEFINE_HOOK(463F5E, CIsoView_OnLButtonDown_SkipPlaceTileUndoRedo2, 5)
//{
//	if (!IsPlacingTiles)
//		return 0x4616C2;
//	return 0x4616D8;
//}


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

