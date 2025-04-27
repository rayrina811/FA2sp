#include <Helpers/Macro.h>
#include <Drawing.h>
#include <CPalette.h>
#include <CLoading.h>
#include "../../Miscs/Palettes.h"
#include "../CFinalSunDlg/Body.h"
#include "../CIsoView/Body.h"
#include "../CLoading/Body.h"
#include "../CMapData/Body.h"
#include <Miscs/Miscs.h>
#include "../../Helpers/Translations.h"
#include "../../Miscs/Hooks.INI.h"
#include "../../Miscs/MultiSelection.h"
#include <codecvt>
#include "../../ExtraWindow/CTerrainGenerator/CTerrainGenerator.h"

static int Left, Right, Top, Bottom;
static CRect window;
static MapCoord VisibleCoordTL;
static MapCoord VisibleCoordBR;
static int HorizontalLoopIndex;
static ppmfc::CPoint ViewPosition;

std::unordered_set<short> CIsoViewExt::VisibleStructures;
std::unordered_set<short> CIsoViewExt::VisibleInfantries;
std::unordered_set<short> CIsoViewExt::VisibleUnits;
std::unordered_set<short> CIsoViewExt::VisibleAircrafts;

#define EXTRA_BORDER 15

inline static bool IsCoordInWindow(int X, int Y)
{
	return
		X + Y > VisibleCoordTL.X + VisibleCoordTL.Y - EXTRA_BORDER &&
		X + Y < VisibleCoordBR.X + VisibleCoordBR.Y + CIsoViewExt::EXTRA_BORDER_BOTTOM &&
		X > Y + VisibleCoordBR.X - VisibleCoordBR.Y - EXTRA_BORDER &&
		X < Y + VisibleCoordTL.X - VisibleCoordTL.Y + EXTRA_BORDER;
}

DEFINE_HOOK(46DE00, CIsoView_Draw_Begin, 7)
{
	auto pThis = CIsoView::GetInstance();
	PalettesManager::CalculatedObjectPaletteFiles.clear();
	CIsoViewExt::VisibleStructures.clear();
	CIsoViewExt::VisibleInfantries.clear();
	CIsoViewExt::VisibleUnits.clear();
	CIsoViewExt::VisibleAircrafts.clear();
	CLoadingExt::CurrentFrameImageDataMap.clear();

	RECT rect;
	::GetClientRect(pThis->GetSafeHwnd(), &rect);
	POINT topLeft = { rect.left, rect.top };
	::ClientToScreen(pThis->GetSafeHwnd(), &topLeft);
	double offsetX = 0.016795436849483363 * topLeft.x - 4.664099013466316;
	double offsetY = 0.03362306232114938 * topLeft.y - 2.4360168849787662;
	ViewPosition = pThis->ViewPosition;
	pThis->ViewPosition.x += offsetX;
	pThis->ViewPosition.y += offsetY;

	if (INIIncludes::MapINIWarn)
	{
		if (!CINI::CurrentDocument->GetBool("FA2spVersionControl", "MapIncludeWarned"))
		{
			int result = MessageBox(CIsoView::GetInstance()->GetSafeHwnd(),
				Translations::TranslateOrDefault("MapIncludeWarningMessage",
					"This map contains include INIs. All key value pairs in the INIs will not be saved to the map, nor will be saved to the INIs.\n"
					"If you click 'OK', this warning will no longer pop up in this map."),
				Translations::TranslateOrDefault("Warning", "Warning"),
				MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONEXCLAMATION);

			if (result == IDOK)
				CINI::CurrentDocument->WriteBool("FA2spVersionControl", "MapIncludeWarned", true);
		}
		INIIncludes::MapINIWarn = false;
	}

	return 0;
}

DEFINE_HOOK(46DF20, CIsoView_Draw_BackgroundColor, 6)
{
	R->Stack(STACK_OFFS(0xD20, 0x020), ExtConfigs::DrawMapBackground_Color);
	return 0;
}

DEFINE_HOOK(46DF49, CIsoView_Draw_ScaledBorder, 6)
{
	GET_STACK(CRect, viewRect, STACK_OFFS(0xD18, 0xCCC));
	viewRect.right += viewRect.Width() * (CIsoViewExt::ScaledFactor - 1.0);
	viewRect.bottom += viewRect.Height() * (CIsoViewExt::ScaledFactor - 1.0);
	R->Stack(STACK_OFFS(0xD18, 0xCCC), viewRect);
	return 0;
}

DEFINE_HOOK(46E815, CIsoView_Draw_Optimize_GetBorder, 5)
{
	Left = R->Stack<int>(STACK_OFFS(0xD18, 0xC10)) - EXTRA_BORDER;
	Right = R->Stack<int>(STACK_OFFS(0xD18, 0xC64)) + EXTRA_BORDER;
	Top = R->Stack<int>(STACK_OFFS(0xD18, 0xCBC)) - EXTRA_BORDER;
	Bottom = R->Stack<int>(STACK_OFFS(0xD18, 0xC18)) + CIsoViewExt::EXTRA_BORDER_BOTTOM;
	auto pThis = CIsoView::GetInstance();
	pThis->GetWindowRect(&window);

	double scale = CIsoViewExt::ScaledFactor;
	if (scale < 0.9)
		scale += 0.1;
	if (scale < 0.7)
		scale += 0.1;
	if (scale < 0.5)
		scale += 0.1;
	window.right += window.Width() * (scale - 1.0);
	if (scale < 1.0)
		scale = 1.0;
	window.bottom += window.Height() * (scale - 1.0);

	VisibleCoordTL.X = window.left + pThis->ViewPosition.x;
	VisibleCoordTL.Y = window.top + pThis->ViewPosition.y;
	VisibleCoordBR.X = window.right + pThis->ViewPosition.x;
	VisibleCoordBR.Y = window.bottom + pThis->ViewPosition.y;
	pThis->ScreenCoord2MapCoord_Flat(VisibleCoordTL.X, VisibleCoordTL.Y);
	pThis->ScreenCoord2MapCoord_Flat(VisibleCoordBR.X, VisibleCoordBR.Y);
	if (VisibleCoordBR.X < 0 || VisibleCoordBR.Y < 0)
	{
		VisibleCoordBR.X = CMapData::Instance->Size.Width;
		VisibleCoordBR.Y = CMapData::Instance->MapWidthPlusHeight + 1;
	}
	if (VisibleCoordTL.X < 0 || VisibleCoordTL.Y < 0)
	{
		VisibleCoordTL.X = CMapData::Instance->Size.Width;
		VisibleCoordTL.Y = 0;
	}

	HorizontalLoopIndex = VisibleCoordTL.X + VisibleCoordTL.Y - EXTRA_BORDER;
	return 0;
}

DEFINE_HOOK(46EA64, CIsoView_Draw_MainLoop, 6)
{
	GET_STACK(float, DrawOffsetX, STACK_OFFS(0xD18, 0xCB0));
	GET_STACK(float, DrawOffsetY, STACK_OFFS(0xD18, 0xCB8));
	LEA_STACK(LPDDSURFACEDESC2, lpDesc, STACK_OFFS(0xD18, 0x92C));
	auto pThis = (CIsoViewExt*)CIsoView::GetInstance();
	DDBoundary boundary{ lpDesc->dwWidth, lpDesc->dwHeight, lpDesc->lPitch };
	CIsoViewExt::drawOffsetX = DrawOffsetX;
	CIsoViewExt::drawOffsetY = DrawOffsetY;

	HDC hDC;
	pThis->lpDDBackBufferSurface->GetDC(&hDC);

	std::vector<std::pair<MapCoord, ppmfc::CString>> WaypointsToDraw;
	std::vector<std::pair<MapCoord, ppmfc::CString>> OverlayTextsToDraw;
	std::vector<std::pair<MapCoord, DrawBuildings>> BuildingsToDraw;
	std::vector<DrawVeterancies> DrawVeterancies;

	std::unordered_set<short> DrawnBuildings;
	std::vector<BaseNodeDataExt> DrawnBaseNodes;

	if (CIsoViewExt::DrawInfantries && CIsoViewExt::DrawInfantriesFilter && CViewObjectsExt::InfantryBrushDlgF)
	{
		auto CheckValue = [&](int nCheckBoxIdx, const ppmfc::CString& src, const ppmfc::CString& dst)
			{
				if (CViewObjectsExt::InfantryBrushBoolsF[nCheckBoxIdx - 1300])
				{
					if (dst == src) return true;
					else return false;
				}
				return true;
			};
		short idx = 0;
		for (const auto& data : CMapData::Instance->InfantryDatas)
		{
			const auto& filter = CViewObjectsExt::ObjectFilterI;
			if (filter.empty() || std::find(filter.begin(), filter.end(), data.TypeID) != filter.end())
			{
				if (CheckValue(1300, CViewObjectsExt::InfantryBrushDlgF->CString_House, data.House) &&
					CheckValue(1301, CViewObjectsExt::InfantryBrushDlgF->CString_HealthPoint, data.Health) &&
					CheckValue(1302, CViewObjectsExt::InfantryBrushDlgF->CString_State, data.Status) &&
					CheckValue(1303, CViewObjectsExt::InfantryBrushDlgF->CString_Direction, data.Facing) &&
					CheckValue(1304, CViewObjectsExt::InfantryBrushDlgF->CString_VerteranStatus, data.VeterancyPercentage) &&
					CheckValue(1305, CViewObjectsExt::InfantryBrushDlgF->CString_Group, data.Group) &&
					CheckValue(1306, CViewObjectsExt::InfantryBrushDlgF->CString_OnBridge, data.IsAboveGround) &&
					CheckValue(1307, CViewObjectsExt::InfantryBrushDlgF->CString_AutoCreateNoRecruitable, data.AutoNORecruitType) &&
					CheckValue(1308, CViewObjectsExt::InfantryBrushDlgF->CString_AutoCreateYesRecruitable, data.AutoYESRecruitType) &&
					CheckValue(1309, CViewObjectsExt::InfantryBrushDlgF->CString_Tag, data.Tag))
					CIsoViewExt::VisibleInfantries.insert(idx);
			}
			idx++;
		}
	}
	if (CIsoViewExt::DrawUnits && CIsoViewExt::DrawUnitsFilter && CViewObjectsExt::VehicleBrushDlgF)
	{
		auto CheckValue = [&](int nCheckBoxIdx, const ppmfc::CString& src, const ppmfc::CString& dst)
			{
				if (CViewObjectsExt::VehicleBrushBoolsF[nCheckBoxIdx - 1300])
				{
					if (dst == src) return true;
					else return false;
				}
				return true;
			};
		short idx = 0;
		for (short idx = 0; idx < CINI::CurrentDocument->GetKeyCount("Units"); idx++)
		{
			CUnitData data;
			CMapData::Instance->GetUnitData(idx, data);
			const auto& filter = CViewObjectsExt::ObjectFilterV;
			if (filter.empty() || std::find(filter.begin(), filter.end(), data.TypeID) != filter.end())
			{
				if (CheckValue(1300, CViewObjectsExt::VehicleBrushDlgF->CString_House, data.House) &&
					CheckValue(1301, CViewObjectsExt::VehicleBrushDlgF->CString_HealthPoint, data.Health) &&
					CheckValue(1302, CViewObjectsExt::VehicleBrushDlgF->CString_State, data.Status) &&
					CheckValue(1303, CViewObjectsExt::VehicleBrushDlgF->CString_Direction, data.Facing) &&
					CheckValue(1304, CViewObjectsExt::VehicleBrushDlgF->CString_VeteranLevel, data.VeterancyPercentage) &&
					CheckValue(1305, CViewObjectsExt::VehicleBrushDlgF->CString_Group, data.Group) &&
					CheckValue(1306, CViewObjectsExt::VehicleBrushDlgF->CString_OnBridge, data.IsAboveGround) &&
					CheckValue(1307, CViewObjectsExt::VehicleBrushDlgF->CString_FollowerID, data.FollowsIndex) &&
					CheckValue(1308, CViewObjectsExt::VehicleBrushDlgF->CString_AutoCreateNoRecruitable, data.AutoNORecruitType) &&
					CheckValue(1309, CViewObjectsExt::VehicleBrushDlgF->CString_AutoCreateYesRecruitable, data.AutoYESRecruitType) &&
					CheckValue(1310, CViewObjectsExt::VehicleBrushDlgF->CString_Tag, data.Tag))
					CIsoViewExt::VisibleUnits.insert(idx);
			}
		}
	}
	if (CIsoViewExt::DrawAircrafts && CIsoViewExt::DrawAircraftsFilter && CViewObjectsExt::AircraftBrushDlgF)
	{
		auto CheckValue = [&](int nCheckBoxIdx, const ppmfc::CString& src, const ppmfc::CString& dst)
			{
				if (CViewObjectsExt::AircraftBrushBoolsF[nCheckBoxIdx - 1300])
				{
					if (dst == src) return true;
					else return false;
				}
				return true;
			};
		for (short idx = 0; idx < CINI::CurrentDocument->GetKeyCount("Aircraft"); idx++)
		{
			CAircraftData data;
			CMapData::Instance->GetAircraftData(idx, data);
			const auto& filter = CViewObjectsExt::ObjectFilterA;
			if (filter.empty() || std::find(filter.begin(), filter.end(), data.TypeID) != filter.end())
			{
				if (CheckValue(1300, CViewObjectsExt::AircraftBrushDlgF->CString_House, data.House) &&
					CheckValue(1301, CViewObjectsExt::AircraftBrushDlgF->CString_HealthPoint, data.Health) &&
					CheckValue(1302, CViewObjectsExt::AircraftBrushDlgF->CString_Direction, data.Facing) &&
					CheckValue(1303, CViewObjectsExt::AircraftBrushDlgF->CString_Status, data.Status) &&
					CheckValue(1304, CViewObjectsExt::AircraftBrushDlgF->CString_VeteranLevel, data.VeterancyPercentage) &&
					CheckValue(1305, CViewObjectsExt::AircraftBrushDlgF->CString_Group, data.Group) &&
					CheckValue(1306, CViewObjectsExt::AircraftBrushDlgF->CString_AutoCreateNoRecruitable, data.AutoNORecruitType) &&
					CheckValue(1307, CViewObjectsExt::AircraftBrushDlgF->CString_AutoCreateYesRecruitable, data.AutoYESRecruitType) &&
					CheckValue(1308, CViewObjectsExt::AircraftBrushDlgF->CString_Tag, data.Tag))
					CIsoViewExt::VisibleAircrafts.insert(idx);
			}
		}
	}
	if (CIsoViewExt::DrawStructures && CIsoViewExt::DrawStructuresFilter && CViewObjectsExt::BuildingBrushDlgBF)
	{
		auto CheckValue = [&](int nCheckBoxIdx, const ppmfc::CString& src, const ppmfc::CString& dst)
			{
				if (CViewObjectsExt::BuildingBrushBoolsBF[nCheckBoxIdx - 1300])
				{
					if (dst == src) return true;
					else return false;
				}
				return true;
			};
		for (short idx = 0; idx < CINI::CurrentDocument->GetKeyCount("Structures"); idx++)
		{
			CBuildingData data;
			CMapDataExt::GetBuildingDataByIniID(idx, data);
			const auto& filter = CViewObjectsExt::ObjectFilterB;
			if (filter.empty() || std::find(filter.begin(), filter.end(), data.TypeID) != filter.end())
			{
				if (CheckValue(1300, CViewObjectsExt::BuildingBrushDlgBF->CString_House, data.House) &&
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
					CheckValue(1313, CViewObjectsExt::BuildingBrushDlgBF->CString_Tag, data.Tag))
					CIsoViewExt::VisibleStructures.insert(idx);
			}
		}
	}

	//loop1: tiles
	std::vector<MapCoord> RedrawCoords;
	for (int XplusY = Left + Top; XplusY < Right + Bottom; XplusY++) {
		for (int X = 0; X < XplusY; X++) {
			int Y = XplusY - X;
			if (!IsCoordInWindow(X, Y))
				continue;
			if (!CMapData::Instance->IsCoordInMap(X, Y))
				continue;

			const auto cell = CMapData::Instance->GetCellAt(X, Y);

			CIsoViewExt::CurrentDrawCellLocation.X = X;
			CIsoViewExt::CurrentDrawCellLocation.Y = Y;
			CIsoViewExt::CurrentDrawCellLocation.Height = cell->Height;

			int altImage = cell->Flag.AltIndex;
			int tileIndex = CMapDataExt::GetSafeTileIndex(cell->TileIndex);
			int tileSubIndex = CMapDataExt::GetSafeTileIndex(cell->TileSubIndex);
			if (tileIndex >= CMapDataExt::TileDataCount)
				continue;

			cell->Flag.RedrawTerrain = false;
			for (int i = 1; i <= 2; i++)
			{
				if (CMapData::Instance->IsCoordInMap(X - i, Y - i))
				{
					auto blockedCell = CMapData::Instance->GetCellAt(X - i, Y - i);
					if (cell->Height - blockedCell->Height >= 2 * i
						|| i == 1 && blockedCell->Flag.RedrawTerrain && cell->Height > blockedCell->Height)
						cell->Flag.RedrawTerrain = true;
				}
			}

			if (!cell->Flag.RedrawTerrain)
			{
				if (CFinalSunApp::Instance->FrameMode)
				{
					if (CMapDataExt::TileData[tileIndex].FrameModeIndex != 0xFFFF)
					{
						tileIndex = CMapDataExt::TileData[tileIndex].FrameModeIndex;
					}
					else
					{
						tileIndex = CMapDataExt::TileSet_starts[CMapDataExt::HeightBase] + cell->Height;
						tileSubIndex = 0;
					}
				}

				CTileTypeClass tile = CMapDataExt::TileData[tileIndex];
				int tileSet = tile.TileSet;
				if (tile.AltTypeCount)
				{
					if (altImage > 0)
					{
						tile = altImage < tile.AltTypeCount ? tile.AltTypes[altImage - 1] : tile.AltTypes[tile.AltTypeCount - 1];
					}
				}
				if (tileSubIndex < tile.TileBlockCount && tile.TileBlockDatas[tileSubIndex].ImageData != NULL)
				{
					auto& subTile = tile.TileBlockDatas[tileSubIndex];
					int x = X;
					int y = Y;
					CIsoView::MapCoord2ScreenCoord(x, y);
					x -= DrawOffsetX;
					y -= DrawOffsetY;
					x -= 60;
					y -= 30;

					if (subTile.HasValidImage)
					{
						Palette* pal = CMapDataExt::TileSetPalettes[CMapDataExt::TileData[tileIndex].TileSet];

						CIsoViewExt::BlitTerrain(pThis, lpDesc->lpSurface, window, boundary,
							x + subTile.XMinusExX, y + subTile.YMinusExY, &subTile, pal,
							cell->IsHidden() ? 128 : 255);

						auto& cellExt = CMapDataExt::CellDataExts[CMapData::Instance->GetCoordIndex(X, Y)];
						cellExt.HasAnim = false;
						if (CMapDataExt::TileAnimations.find(tileIndex) != CMapDataExt::TileAnimations.end())
						{
							auto& tileAnim = CMapDataExt::TileAnimations[tileIndex];
							if (tileAnim.AttachedSubTile == tileSubIndex)
							{
								cellExt.HasAnim = true;
							}
						}
						if (CMapDataExt::RedrawExtraTileSets.find(tileSet) != CMapDataExt::RedrawExtraTileSets.end())
							RedrawCoords.push_back(MapCoord{ X,Y });
					}
				}
			}
		}
	}
	for (const auto& coord : RedrawCoords)
	{
		const int& X = coord.X;
		const int& Y = coord.Y;
		const auto cell = CMapData::Instance->GetCellAt(X, Y);

		CIsoViewExt::CurrentDrawCellLocation.X = X;
		CIsoViewExt::CurrentDrawCellLocation.Y = Y;
		CIsoViewExt::CurrentDrawCellLocation.Height = cell->Height;

		int altImage = cell->Flag.AltIndex;
		int tileIndex = CMapDataExt::GetSafeTileIndex(cell->TileIndex);
		int tileSubIndex = CMapDataExt::GetSafeTileIndex(cell->TileSubIndex);

		if (CFinalSunApp::Instance->FrameMode)
		{
			if (CMapDataExt::TileData[tileIndex].FrameModeIndex != 0xFFFF)
			{
				tileIndex = CMapDataExt::TileData[tileIndex].FrameModeIndex;
			}
			else
			{
				tileIndex = CMapDataExt::TileSet_starts[CMapDataExt::HeightBase] + cell->Height;
				tileSubIndex = 0;
			}
		}

		CTileTypeClass tile = CMapDataExt::TileData[tileIndex];
		if (tile.AltTypeCount)
		{
			if (altImage > 0)
			{
				tile = altImage < tile.AltTypeCount ? tile.AltTypes[altImage - 1] : tile.AltTypes[tile.AltTypeCount - 1];
			}
		}
		if (tileSubIndex < tile.TileBlockCount && tile.TileBlockDatas[tileSubIndex].ImageData != NULL)
		{
			auto& subTile = tile.TileBlockDatas[tileSubIndex];
			int x = X;
			int y = Y;
			CIsoView::MapCoord2ScreenCoord(x, y);
			x -= DrawOffsetX;
			y -= DrawOffsetY;
			x -= 60;
			y -= 30;

			if (subTile.HasValidImage)
			{
				Palette* pal = CMapDataExt::TileSetPalettes[CMapDataExt::TileData[tileIndex].TileSet];

				ppmfc::CString extraImageID;
				extraImageID.Format("EXTRAIMAGE\233%d%d%d", tileIndex, tileSubIndex, altImage);
				auto pData = CLoadingExt::GetImageDataFromServer("DUMMY_FILE", extraImageID);
				if (pData->pImageBuffer)
				{
					CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary,
						x + subTile.XMinusExX + 30,
						y + subTile.YMinusExY + 30,
						pData, pal, cell->IsHidden() ? 128 : 255, -2, -10);
				}			
			}
		}
	}

	//loop2: shadows
	bool shadow = CIsoViewExt::DrawShadows && ExtConfigs::InGameDisplay_Shadow;
	const unsigned char Transparency = 128;
	for (int XplusY = Left + Top; XplusY < Right + Bottom; XplusY++) {
		for (int X = 0; X < XplusY; X++) {
			int Y = XplusY - X;

			if (!IsCoordInWindow(X, Y))
				continue;

			if (!CMapData::Instance->IsCoordInMap(X, Y))
				continue;

			int pos = CMapData::Instance->GetCoordIndex(X, Y);
			auto cell = CMapData::Instance->GetCellAt(pos);

			CIsoViewExt::CurrentDrawCellLocation.X = X;
			CIsoViewExt::CurrentDrawCellLocation.Y = Y;
			CIsoViewExt::CurrentDrawCellLocation.Height = cell->Height;

			int x = X;
			int y = Y;
			if (shadow)
			{
				CIsoView::MapCoord2ScreenCoord(x, y);
				x -= DrawOffsetX;
				y -= DrawOffsetY;
			}
			if (cell->Waypoint != -1)
			{
				WaypointsToDraw.push_back(std::make_pair(MapCoord{ X, Y },
					CINI::CurrentDocument->GetKeyAt("Waypoints", cell->Waypoint)));
			}
			if (cell->Structure > -1 && cell->Structure < CMapDataExt::StructureIndexMap.size())
			{
				auto StrINIIndex = CMapDataExt::StructureIndexMap[cell->Structure];
				if (StrINIIndex != -1)
				{
					const auto& filter = CIsoViewExt::VisibleStructures;
					if (!CIsoViewExt::DrawStructuresFilter
						|| filter.find(StrINIIndex) != filter.end())
					{
						const auto& objRender = CMapDataExt::BuildingRenderDatasFix[StrINIIndex];
						if (std::find(DrawnBuildings.begin(), DrawnBuildings.end(), StrINIIndex) == DrawnBuildings.end())
						{
							DrawnBuildings.insert(StrINIIndex);
							MapCoord objCenter;
							const int BuildingIndex = CMapData::Instance->GetBuildingTypeID(objRender.ID);
							const auto& DataExt = CMapDataExt::BuildingDataExts[BuildingIndex];
							objCenter.X = objRender.X + DataExt.Height / 2;
							objCenter.Y = objRender.Y + DataExt.Width / 2;
							if (!CMapData::Instance->IsCoordInMap(objCenter.X, objCenter.Y))
							{
								objCenter.X = objRender.X;
								objCenter.Y = objRender.Y;
							}
							// if objects overlapping with building, draw building earlier
							if (DataExt.IsCustomFoundation())
							{
								for (const auto& block : *DataExt.Foundations)
								{
									MapCoord coord = { X + block.Y, Y + block.X };
									if (!CMapData::Instance->IsCoordInMap(coord.X, coord.Y))
										continue;

									auto buildingCell = CMapData::Instance->GetCellAt(coord.X, coord.Y);
									if (buildingCell->Unit != -1 || buildingCell->Aircraft != -1
										|| buildingCell->Terrain != -1 || buildingCell->Infantry[0] != -1
										|| buildingCell->Infantry[1] != -1 || buildingCell->Infantry[2] != -1)
									{
										objCenter.X = objRender.X;
										objCenter.Y = objRender.Y;
										break;
									}
								}
							}
							else
							{
								for (int dx = 0; dx < DataExt.Height; ++dx)
								{
									for (int dy = 0; dy < DataExt.Width; ++dy)
									{
										MapCoord coord = { X + dx, Y + dy };
										if (!CMapData::Instance->IsCoordInMap(coord.X, coord.Y))
											continue;

										auto buildingCell = CMapData::Instance->GetCellAt(coord.X, coord.Y);
										if (buildingCell->Unit != -1 || buildingCell->Aircraft != -1
											|| buildingCell->Terrain != -1 || buildingCell->Infantry[0] != -1
											|| buildingCell->Infantry[1] != -1 || buildingCell->Infantry[2] != -1)
										{
											objCenter.X = objRender.X;
											objCenter.Y = objRender.Y;
											break;
										}
									}
								}
							}
							BuildingsToDraw.push_back(std::make_pair(MapCoord{ objRender.X, objRender.Y },
								DrawBuildings{ StrINIIndex , (short)objCenter.X, (short)objCenter.Y, (short)BuildingIndex }));

							if (shadow && CIsoViewExt::DrawStructures)
							{
								int x1 = objRender.X;
								int y1 = objRender.Y;
								CIsoView::MapCoord2ScreenCoord(x1, y1);
								x1 -= DrawOffsetX;
								y1 -= DrawOffsetY;

								int nFacing = 0;
								if (Variables::Rules.GetBool(objRender.ID, "Turret") && !Variables::Rules.GetBool(objRender.ID, "TurretAnimIsVoxel"))
									nFacing = 7 - (objRender.Facing / 32) % 8;

								const int HP = objRender.Strength;
								int status = CLoadingExt::GBIN_NORMAL;
								if (HP == 0)
									status = CLoadingExt::GBIN_RUBBLE;
								else if (static_cast<int>((CMapDataExt::ConditionYellow + 0.001f) * 256) > HP)
									status = CLoadingExt::GBIN_DAMAGED;
								const auto& imageName = CLoadingExt::GetBuildingImageName(objRender.ID, nFacing, status, true);

								if (!CLoadingExt::IsObjectLoaded(objRender.ID))
								{
									CLoading::Instance->LoadObjects(objRender.ID);
								}

								auto pData = CLoadingExt::GetImageDataFromServer(objRender.ID, imageName);
								if (pData->pImageBuffer)
								{
									CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary,
										x1 - pData->FullWidth / 2, y1 - pData->FullHeight / 2, pData, NULL, Transparency);
								}
							}
						}
					}
				}
			}
			for (int i = 0; i < 3 && shadow; i++)
			{
				if (cell->Infantry[i] != -1 && CIsoViewExt::DrawInfantries)
				{
					const auto& filter = CIsoViewExt::VisibleInfantries;
					if (!CIsoViewExt::DrawInfantriesFilter
						|| std::find(filter.begin(), filter.end(), cell->Infantry[i]) != filter.end())
					{
						CInfantryData obj;
						CMapData::Instance->GetInfantryData(cell->Infantry[i], obj);
						int nFacing = 7 - (atoi(obj.Facing) / 32) % 8;

						bool water = false;
						const auto& swim = CLoadingExt::SwimableInfantries;
						if (ExtConfigs::InGameDisplay_Water && std::find(swim.begin(), swim.end(), obj.TypeID) != swim.end())
						{
							auto landType = CMapDataExt::GetLandType(cell->TileIndex, cell->TileSubIndex);
							if (landType == LandType::Water || landType == LandType::Beach)
							{
								water = true;
							}
						}
						bool deploy = ExtConfigs::InGameDisplay_Deploy
							&& obj.Status == "Unload" && Variables::Rules.GetBool(obj.TypeID, "Deployer");

						const auto& imageName = CLoadingExt::GetImageName(obj.TypeID, nFacing, true, deploy && !water, water);

						if (!CLoadingExt::IsObjectLoaded(obj.TypeID))
						{
							CLoading::Instance->LoadObjects(obj.TypeID);
						}
						auto pData = CLoadingExt::GetImageDataFromServer(obj.TypeID, imageName);

						if (pData->pImageBuffer)
						{
							int x1 = x;
							int y1 = y;
							switch (atoi(obj.SubCell))
							{
							case 2:
								x1 += 15;
								y1 += 14;
								break;
							case 3:
								x1 -= 15;
								y1 += 14;
								break;
							case 4:
								y1 += 22;
								break;
							default:
								y1 += 15;
								break;
							}
							CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary,
								x1 - pData->FullWidth / 2, y1 - pData->FullHeight / 2, pData, NULL, Transparency);
						}
					}
				}
			}
			if (shadow && cell->Unit != -1 && CIsoViewExt::DrawUnits)
			{
				const auto& filter = CIsoViewExt::VisibleUnits;
				if (!CIsoViewExt::DrawUnitsFilter
					|| std::find(filter.begin(), filter.end(), cell->Unit) != filter.end())
				{
					CUnitData obj;
					CMapData::Instance->GetUnitData(cell->Unit, obj);

					int nFacing = (atoi(obj.Facing) / 32) % 8;

					if (ExtConfigs::InGameDisplay_Water)
					{
						auto landType = CMapDataExt::GetLandType(cell->TileIndex, cell->TileSubIndex);
						if (landType == LandType::Water || landType == LandType::Beach)
						{
							obj.TypeID = Variables::Rules.GetString(obj.TypeID, "WaterImage", obj.TypeID);
						}
					}
					if (ExtConfigs::InGameDisplay_Deploy && obj.Status == "Unload")
					{
						obj.TypeID = Variables::Rules.GetString(obj.TypeID, "UnloadingClass", obj.TypeID);
					}

					const auto& imageName = CLoadingExt::GetImageName(obj.TypeID, nFacing, true);

					if (!CLoadingExt::IsObjectLoaded(obj.TypeID))
					{
						CLoading::Instance->LoadObjects(obj.TypeID);
					}
					auto pData = CLoadingExt::GetImageDataFromServer(obj.TypeID, imageName);

					if (pData->pImageBuffer)
					{
						int x1 = x;
						int y1 = y;
						CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary,
							x1 - pData->FullWidth / 2, y1 - pData->FullHeight / 2 + 15, pData, NULL, Transparency);
					}
				}
			}
			if (shadow && cell->Terrain != -1 && CIsoViewExt::DrawTerrains)
			{
				auto obj = Variables::GetRulesMapValueAt("TerrainTypes", cell->TerrainType);
				const auto& imageName = CLoadingExt::GetImageName(obj, 0, true);

				if (!CLoadingExt::IsObjectLoaded(obj))
				{
					CLoading::Instance->LoadObjects(obj);
				}
				auto pData = CLoadingExt::GetImageDataFromServer(obj, imageName);

				if (pData->pImageBuffer)
				{
					int x1 = x;
					int y1 = y;
					CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary,
						x1 - pData->FullWidth / 2, y1 - pData->FullHeight / 2 + (Variables::Rules.GetBool(obj, "SpawnsTiberium") ? 0 : 12), pData, NULL, Transparency);
				}
			}
			if (shadow && cell->Overlay != 0xFF && CIsoViewExt::DrawOverlays)
			{
				auto obj = Variables::GetRulesMapValueAt("OverlayTypes", cell->Overlay);
				ppmfc::CString imageName;
				imageName.Format("%s%d\233OVERLAYSHADOW", obj, cell->OverlayData);

				if (!CLoadingExt::IsOverlayLoaded(obj))
				{
					CLoading::Instance->DrawOverlay(obj, cell->Overlay);
					CIsoView::GetInstance()->UpdateDialog(false);
				}
				auto pData = CLoadingExt::GetImageDataFromServer("DUMMY_FILE", imageName);

				if (pData->pImageBuffer)
				{
					int x1 = x;
					int y1 = y;
					if (cell->Overlay == 0xA7)
						y1 -= 45;
					else if (
						cell->Overlay != 0x18 && cell->Overlay != 0x19 && // BRIDGE1, BRIDGE2
						cell->Overlay != 0x3B && cell->Overlay != 0x3C && // RAILBRDG1, RAILBRDG2
						cell->Overlay != 0xED && cell->Overlay != 0xEE // BRIDGEB1, BRIDGEB2
						)
					{
						if (cell->Overlay >= 0x27 && cell->Overlay <= 0x36) // Tracks
							y1 += 15;
						else if (cell->Overlay >= 0x4A && cell->Overlay <= 0x65) // LOBRDG 1-28
							y1 += 15;
						else if (cell->Overlay >= 0xCD && cell->Overlay <= 0xEC) // LOBRDGB 1-4
							y1 += 15;
						else if (cell->Overlay < CMapDataExt::OverlayTypeDatas.size())
						{
							if (CMapDataExt::OverlayTypeDatas[cell->Overlay].Rock
								//|| CMapDataExt::OverlayTypeDatas[cell->Overlay].TerrainRock // for compatibility of blockages
								|| CMapDataExt::OverlayTypeDatas[cell->Overlay].RailRoad)
								y1 += 15;
						}
					}
					else
					{
						if (cell->OverlayData >= 0x9 && cell->OverlayData <= 0x11)
							y1 -= 16;
						else
							y1 -= 1;
					}

					CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary,
						x1 - pData->FullWidth / 2, y1 - pData->FullHeight / 2, pData, NULL, Transparency);
				}
			}
		}
	}

	//loop3: objects
	for (int XplusY = Left + Top; XplusY < Right + Bottom; XplusY++) {
		for (int X = 0; X < XplusY; X++) {
			int Y = XplusY - X;
			if (!IsCoordInWindow(X, Y))
				continue;
			if (!CMapData::Instance->IsCoordInMap(X, Y))
				continue;

			const auto cell = CMapData::Instance->GetCellAt(X, Y);
			const auto& cellExt = CMapDataExt::CellDataExts[CMapData::Instance->GetCoordIndex(X, Y)];

			CIsoViewExt::CurrentDrawCellLocation.X = X;
			CIsoViewExt::CurrentDrawCellLocation.Y = Y;
			CIsoViewExt::CurrentDrawCellLocation.Height = cell->Height;

			int x = X;
			int y = Y;
			CIsoView::MapCoord2ScreenCoord(x, y);
			x -= DrawOffsetX;
			y -= DrawOffsetY;

			// tiles
			int altImage = cell->Flag.AltIndex;
			int tileIndex = CMapDataExt::GetSafeTileIndex(cell->TileIndex);
			int tileSubIndex = CMapDataExt::GetSafeTileIndex(cell->TileSubIndex);
			if (tileIndex < CMapDataExt::TileDataCount)
			{
				auto drawTerrainAnim = [&pThis, &lpDesc, &boundary, &cell](int tileIndex, int tileSubIndex, int x, int y)
					{
						if (CMapDataExt::TileAnimations.find(tileIndex) != CMapDataExt::TileAnimations.end())
						{
							auto& tileAnim = CMapDataExt::TileAnimations[tileIndex];
							if (tileAnim.AttachedSubTile == tileSubIndex)
							{
								auto pData = CLoadingExt::GetImageDataFromServer("DUMMY_FILE", tileAnim.ImageName);

								if (pData->pImageBuffer)
								{
									CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary,
										x - pData->FullWidth / 2 + tileAnim.XOffset,
										y - pData->FullHeight / 2 + tileAnim.YOffset + 15,
										pData, NULL, cell->IsHidden() ? 128 : 255, -2, -10);
								}
							}
						}
					};

				if (cell->Flag.RedrawTerrain)
				{
					if (CFinalSunApp::Instance->FrameMode)
					{
						if (CMapDataExt::TileData[tileIndex].FrameModeIndex != 0xFFFF)
						{
							tileIndex = CMapDataExt::TileData[tileIndex].FrameModeIndex;
						}
						else
						{
							tileIndex = CMapDataExt::TileSet_starts[CMapDataExt::HeightBase] + cell->Height;
							tileSubIndex = 0;
						}
					}

					CTileTypeClass tile = CMapDataExt::TileData[tileIndex];
					int tileSet = tile.TileSet;
					if (tile.AltTypeCount)
					{
						if (altImage > 0)
						{
							tile = altImage < tile.AltTypeCount ? tile.AltTypes[altImage - 1] : tile.AltTypes[tile.AltTypeCount - 1];
						}
					}
					if (tileSubIndex < tile.TileBlockCount && tile.TileBlockDatas[tileSubIndex].ImageData != NULL)
					{
						auto& subTile = tile.TileBlockDatas[tileSubIndex];
						int x1 = x;
						int y1 = y;
						x1 -= 60;
						y1 -= 30;

						if (subTile.HasValidImage)
						{
							Palette* pal = CMapDataExt::TileSetPalettes[CMapDataExt::TileData[tileIndex].TileSet];

							CIsoViewExt::BlitTerrain(pThis, lpDesc->lpSurface, window, boundary,
								x1 + subTile.XMinusExX, y1 + subTile.YMinusExY, &subTile, pal,
								cell->IsHidden() ? 128 : 255);

							if (CMapDataExt::RedrawExtraTileSets.find(tileSet) != CMapDataExt::RedrawExtraTileSets.end())
							{
								ppmfc::CString extraImageID;
								extraImageID.Format("EXTRAIMAGE\233%d%d%d", tileIndex, tileSubIndex, altImage);
								auto pData = CLoadingExt::GetImageDataFromServer("DUMMY_FILE", extraImageID);
								if (pData->pImageBuffer)
								{
									CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary,
										x1 + subTile.XMinusExX + 30,
										y1 + subTile.YMinusExY + 30,
										pData, pal, cell->IsHidden() ? 128 : 255, -2, -10);
								}
							}
							drawTerrainAnim(tileIndex, tileSubIndex, x1 + 60, y1 + 30);
						}
					}
				}
				else
				{
					auto& cellExt = CMapDataExt::CellDataExts[CMapData::Instance->GetCoordIndex(X, Y)];
					if (cellExt.HasAnim)
					{
						if (CFinalSunApp::Instance->FrameMode)
						{
							if (CMapDataExt::TileData[tileIndex].FrameModeIndex != 0xFFFF)
							{
								tileIndex = CMapDataExt::TileData[tileIndex].FrameModeIndex;
							}
							else
							{
								tileIndex = CMapDataExt::TileSet_starts[CMapDataExt::HeightBase] + cell->Height;
								tileSubIndex = 0;
							}
						}
						drawTerrainAnim(tileIndex, tileSubIndex, x, y);
					}
				}
			}

			//smudges
			if (cell->Smudge != -1 && CIsoViewExt::DrawSmudges)
			{
				auto obj = Variables::GetRulesMapValueAt("SmudgeTypes", cell->SmudgeType);
				const auto& imageName = CLoadingExt::GetImageName(obj, 0);
				if (!CLoadingExt::IsObjectLoaded(obj))
				{
					CLoading::Instance->LoadObjects(obj);
				}
				auto pData = CLoadingExt::GetImageDataFromServer(obj, imageName);

				if (pData->pImageBuffer)
				{
					CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary,
						x - pData->FullWidth / 2, y - pData->FullHeight / 2, pData, NULL, 255, 0, -1, false);
				}
			}

			//overlays
			if (cell->Overlay != 0xFF && CIsoViewExt::DrawOverlays)
			{
				ImageDataClass* pData = nullptr;
				if (OverlayData::Array[cell->Overlay].Frames[cell->OverlayData])
				{
					pData = OverlayData::Array[cell->Overlay].Frames[cell->OverlayData];
				}
				if (!pData || !pData->pImageBuffer)
				{
					auto obj = Variables::GetRulesMapValueAt("OverlayTypes", cell->Overlay);
					if (!CLoadingExt::IsOverlayLoaded(obj))
					{
						CLoading::Instance()->DrawOverlay(obj, cell->Overlay);
						CIsoView::GetInstance()->UpdateDialog(false);
						if (OverlayData::Array[cell->Overlay].Frames[cell->OverlayData])
						{
							pData = OverlayData::Array[cell->Overlay].Frames[cell->OverlayData];
						}
					}
					if (!pData || !pData->pImageBuffer)
					{
						if (!(cell->Overlay >= 0x4a && cell->Overlay <= 0x65) && !(cell->Overlay >= 0xcd && cell->Overlay <= 0xec))
						{
							char cd[10];
							cd[0] = '0';
							cd[1] = 'x';
							_itoa(cell->Overlay, cd + 2, 16);
							OverlayTextsToDraw.push_back(std::make_pair(MapCoord{ X,Y }, cd));
						}
					}
				}
				if (pData && pData->pImageBuffer)
				{
					int x1 = x;
					int y1 = y;
					if (cell->Overlay == 0xA7)
						y1 -= 45;
					else if (
						cell->Overlay != 0x18 && cell->Overlay != 0x19 && // BRIDGE1, BRIDGE2
						cell->Overlay != 0x3B && cell->Overlay != 0x3C && // RAILBRDG1, RAILBRDG2
						cell->Overlay != 0xED && cell->Overlay != 0xEE // BRIDGEB1, BRIDGEB2
						)
					{
						if (cell->Overlay >= 0x27 && cell->Overlay <= 0x36) // Tracks
							y1 += 15;
						else if (cell->Overlay >= 0x4A && cell->Overlay <= 0x65) // LOBRDG 1-28
							y1 += 15;
						else if (cell->Overlay >= 0xCD && cell->Overlay <= 0xEC) // LOBRDGB 1-4
							y1 += 15;
						else if (cell->Overlay < CMapDataExt::OverlayTypeDatas.size())
						{
							if (CMapDataExt::OverlayTypeDatas[cell->Overlay].Rock
								//|| CMapDataExt::OverlayTypeDatas[cell->Overlay].TerrainRock // for compatibility of blockages
								|| CMapDataExt::OverlayTypeDatas[cell->Overlay].RailRoad)
								y1 += 15;
						}
					}
					else
					{
						if (cell->OverlayData >= 0x9 && cell->OverlayData <= 0x11)
							y1 -= 16;
						else
							y1 -= 1;
					}
					CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary,
						x1 - pData->FullWidth / 2, y1 - pData->FullHeight / 2, pData, NULL, 255, 0, 500 + cell->Overlay, false);
				}
			}

			//buildings
			for (const auto& [mc, draw] : BuildingsToDraw)
			{
				if ((draw.x == X && draw.y == Y))
				{
					int pos = CMapData::Instance->GetCoordIndex(X, Y);
					const auto& objRender = CMapDataExt::BuildingRenderDatasFix[draw.index];
					int x = mc.X;
					int y = mc.Y;
					CIsoView::MapCoord2ScreenCoord(x, y);
					x -= DrawOffsetX;
					y -= DrawOffsetY;

					if (CFinalSunApp::Instance->ShowBuildingCells)
					{
						const auto& DataExt = CMapDataExt::BuildingDataExts[draw.buildingIndex];
						if (DataExt.IsCustomFoundation())
							pThis->DrawLockedLines(*DataExt.LinesToDraw, x, y, objRender.HouseColor, false, false, lpDesc);
						else
							pThis->DrawLockedCellOutline(x, y, DataExt.Width, DataExt.Height, objRender.HouseColor, false, false, lpDesc);
					}

					if (CIsoViewExt::DrawStructures)
					{
						int nFacing = 0;
						if (Variables::Rules.GetBool(objRender.ID, "Turret"))
							nFacing = 7 - (objRender.Facing / 32) % 8;

						const int HP = objRender.Strength;
						int status = CLoadingExt::GBIN_NORMAL;
						if (HP == 0)
							status = CLoadingExt::GBIN_RUBBLE;
						else if (static_cast<int>((CMapDataExt::ConditionYellow + 0.001f) * 256) > HP)
							status = CLoadingExt::GBIN_DAMAGED;
						const auto& imageName = CLoadingExt::GetBuildingImageName(objRender.ID, nFacing, status);

						if (!CLoadingExt::IsObjectLoaded(objRender.ID))
						{
							CLoading::Instance->LoadObjects(objRender.ID);
						}
						auto pData = CLoadingExt::GetImageDataFromServer(objRender.ID, imageName);

						if (pData->pImageBuffer)
						{
							auto& isoset = CMapDataExt::TerrainPaletteBuildings;
							auto& dam_rubble = CMapDataExt::DamagedAsRubbleBuildings;
							CIsoViewExt::BlitSHPTransparent_Building(pThis, lpDesc->lpSurface, window, boundary,
								x - pData->FullWidth / 2, y - pData->FullHeight / 2, pData, NULL, 255,
								objRender.HouseColor, -1, status == CLoadingExt::GBIN_RUBBLE &&
								dam_rubble.find(objRender.ID) == dam_rubble.end()
								&& imageName != CLoadingExt::GetBuildingImageName(objRender.ID, nFacing, CLoadingExt::GBIN_DAMAGED),
								isoset.find(objRender.ID) != isoset.end());

							for (int upgrade = 0; upgrade < objRender.PowerUpCount; ++upgrade)
							{
								const auto& upg = upgrade == 0 ? objRender.PowerUp1 : (upgrade == 1 ? objRender.PowerUp2 : objRender.PowerUp3);
								const auto& upgXX = upgrade == 0 ? "PowerUp1LocXX" : (upgrade == 1 ? "PowerUp2LocXX" : "PowerUp3LocXX");
								const auto& upgYY = upgrade == 0 ? "PowerUp1LocYY" : (upgrade == 1 ? "PowerUp2LocYY" : "PowerUp3LocYY");
								if (upg.GetLength() == 0)
									continue;

								auto pUpgData = CLoadingExt::GetImageDataFromServer(upg, CLoadingExt::GetImageName(upg, 0));
								if ((!pUpgData || !pUpgData->pImageBuffer) && !CLoadingExt::IsObjectLoaded(upg))
								{
									CLoading::Instance->LoadObjects(upg);
								}
								if (pUpgData && pUpgData->pImageBuffer)
								{
									auto ArtID = CLoadingExt::GetArtID(objRender.ID);

									int x1 = x;
									int y1 = y;
									x1 += CINI::Art->GetInteger(ArtID, upgXX, 0);
									y1 += CINI::Art->GetInteger(ArtID, upgYY, 0);
									CIsoViewExt::BlitSHPTransparent_Building(pThis, lpDesc->lpSurface, window, boundary,
										x1 - pUpgData->FullWidth / 2, y1 - pUpgData->FullHeight / 2, pUpgData, NULL, 255,
										objRender.HouseColor, -1, false, isoset.find(objRender.ID) != isoset.end());
								}
							}
							if (ExtConfigs::InGameDisplay_AlphaImage && CIsoViewExt::DrawAlphaImages)
							{
								if (auto pAIFile = Variables::Rules.TryGetString(objRender.ID, "AlphaImage"))
								{
									ppmfc::CString AIFile = *pAIFile;
									AIFile.Trim();
									auto pAIData = CLoadingExt::GetImageDataFromServer(objRender.ID, AIFile + "\233ALPHAIMAGE");

									if (pAIData && pAIData->pImageBuffer)
									{
										CIsoViewExt::BlitSHPTransparent_AlphaImage(pThis, lpDesc->lpSurface, window, boundary,
											x - pAIData->FullWidth / 2, y - pAIData->FullHeight / 2 + 15, pAIData);
									}
								}
							}
						}
					}
					break;
				}
			}

			//nodes
			if (CIsoViewExt::DrawBasenodes)
			{
				if (!cellExt.BaseNodes.empty())
				{
					auto pThis = (CIsoViewExt*)CIsoView::GetInstance();
					DDBoundary boundary{ lpDesc->dwWidth, lpDesc->dwHeight, lpDesc->lPitch };

					for (const auto& node : cellExt.BaseNodes)
					{
						if (std::find(DrawnBaseNodes.begin(), DrawnBaseNodes.end(), node) == DrawnBaseNodes.end())
						{
							DrawnBaseNodes.push_back(node);

							int X = node.X;
							int Y = node.Y;
							CIsoView::MapCoord2ScreenCoord(X, Y);
							X -= DrawOffsetX;
							Y -= DrawOffsetY;

							if (CIsoViewExt::DrawBasenodesFilter && CViewObjectsExt::BuildingBrushDlgBNF)
							{
								const auto& filter = CViewObjectsExt::ObjectFilterBN;
								auto CheckValue = [&](int nCheckBoxIdx, ppmfc::CString& src, const ppmfc::CString& dst)
									{
										if (CViewObjectsExt::BuildingBrushBoolsBNF[nCheckBoxIdx - 1300])
										{
											if (dst == src) return true;
											else return false;
										}
										return true;
									};
								if (filter.empty() || std::find(filter.begin(), filter.end(), node.ID) != filter.end())
								{
									if (!CheckValue(1300, CViewObjectsExt::BuildingBrushDlgBNF->CString_House, node.House))
										continue;
								}
								else
								{
									continue;
								}
							}
							const auto& imageName = CLoadingExt::GetBuildingImageName(node.ID, 0, 0);
							auto color = Miscs::GetColorRef(node.House);

							if (!CLoadingExt::IsObjectLoaded(node.ID))
							{
								CLoading::Instance->LoadObjects(node.ID);
							}
							auto pData = CLoadingExt::GetImageDataFromServer(node.ID, imageName);
							int cellStr = -1;
							if (cell->Structure > -1 && cell->Structure < CMapDataExt::StructureIndexMap.size())
								cellStr = CMapDataExt::StructureIndexMap[cell->Structure];
							if (CFinalSunApp::Instance->ShowBuildingCells || cellStr != -1)
							{
								const int BuildingIndex = CMapData::Instance->GetBuildingTypeID(node.ID);
								const auto& DataExt = CMapDataExt::BuildingDataExts[BuildingIndex];
								if (DataExt.IsCustomFoundation())
								{
									pThis->DrawLockedLines(*DataExt.LinesToDraw, X, Y, color, true, false, lpDesc);
									pThis->DrawLockedLines(*DataExt.LinesToDraw, X + 1, Y, color, true, false, lpDesc);
								}
								else
								{
									pThis->DrawLockedCellOutline(X, Y, DataExt.Width, DataExt.Height, color, true, false, lpDesc);
									pThis->DrawLockedCellOutline(X + 1, Y, DataExt.Width, DataExt.Height, color, true, false, lpDesc);
								}
							}

							if (pData->pImageBuffer)
							{
								auto& isoset = CMapDataExt::TerrainPaletteBuildings;
								CIsoViewExt::BlitSHPTransparent_Building(pThis, lpDesc->lpSurface, window, boundary,
									X - pData->FullWidth / 2, Y - pData->FullHeight / 2, pData, NULL, 128,
									color, -1, false, isoset.find(node.ID) != isoset.end());
							}
						}
					}
				}
			}

			//units
			if (cell->Unit != -1 && CIsoViewExt::DrawUnits)
			{
				const auto& filter = CIsoViewExt::VisibleUnits;
				if (!CIsoViewExt::DrawUnitsFilter
					|| std::find(filter.begin(), filter.end(), cell->Unit) != filter.end())
				{
					CUnitData obj;
					CMapData::Instance->GetUnitData(cell->Unit, obj);

					int nFacing = (atoi(obj.Facing) / 32) % 8;

					if (ExtConfigs::InGameDisplay_Water)
					{
						auto landType = CMapDataExt::GetLandType(cell->TileIndex, cell->TileSubIndex);
						if (landType == LandType::Water || landType == LandType::Beach)
						{
							obj.TypeID = Variables::Rules.GetString(obj.TypeID, "WaterImage", obj.TypeID);
						}
					}
					if (ExtConfigs::InGameDisplay_Deploy && obj.Status == "Unload")
					{
						obj.TypeID = Variables::Rules.GetString(obj.TypeID, "UnloadingClass", obj.TypeID);
					}

					const auto& imageName = CLoadingExt::GetImageName(obj.TypeID, nFacing);

					if (!CLoadingExt::IsObjectLoaded(obj.TypeID))
					{
						CLoading::Instance->LoadObjects(obj.TypeID);
					}
					auto pData = CLoadingExt::GetImageDataFromServer(obj.TypeID, imageName);

					if (pData->pImageBuffer)
					{
						bool HoveringUnit = Variables::Rules.GetString(obj.TypeID, "SpeedType") == "Hover"
							&& (Variables::Rules.GetString(obj.TypeID, "Locomotor") == "Hover"
								|| Variables::Rules.GetString(obj.TypeID, "Locomotor") == "{4A582742-9839-11d1-B709-00A024DDAFD1}");

						auto color = Miscs::GetColorRef(obj.House);
						CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary,
							x - pData->FullWidth / 2, y - pData->FullHeight / 2 + 15 - (HoveringUnit ? 10 : 0), pData, NULL, 255, color, 0, true);

						auto& veter = DrawVeterancies.emplace_back();
						int	VP = atoi(obj.VeterancyPercentage);
						veter.X = x;
						veter.Y = y;
						veter.VP = VP;
					}
				}
			}

			//aircrafts
			if (cell->Aircraft != -1 && CIsoViewExt::DrawAircrafts)
			{
				const auto& filter = CIsoViewExt::VisibleAircrafts;
				if (!CIsoViewExt::DrawAircraftsFilter
					|| std::find(filter.begin(), filter.end(), cell->Aircraft) != filter.end())
				{
					CAircraftData obj;
					CMapData::Instance->GetAircraftData(cell->Aircraft, obj);

					int nFacing = (atoi(obj.Facing) / 32) % 8;

					const auto& imageName = CLoadingExt::GetImageName(obj.TypeID, nFacing);

					if (!CLoadingExt::IsObjectLoaded(obj.TypeID))
					{
						CLoading::Instance->LoadObjects(obj.TypeID);
					}
					auto pData = CLoadingExt::GetImageDataFromServer(obj.TypeID, imageName);

					if (pData->pImageBuffer)
					{
						auto color = Miscs::GetColorRef(obj.House);
						CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary,
							x - pData->FullWidth / 2, y - pData->FullHeight / 2 + 15, pData, NULL, 255, color, 2, true);

						auto& veter = DrawVeterancies.emplace_back();
						int	VP = atoi(obj.VeterancyPercentage);
						veter.X = x;
						veter.Y = y;
						veter.VP = VP;
					}
				}
			}

			//infantries
			for (int i = 2; i >= 0; --i)
			{
				if (cell->Infantry[i] != -1 && CIsoViewExt::DrawInfantries)
				{
					const auto& filter = CIsoViewExt::VisibleInfantries;
					if (!CIsoViewExt::DrawInfantriesFilter
						|| std::find(filter.begin(), filter.end(), cell->Infantry[i]) != filter.end())
					{
						CInfantryData obj;
						CMapData::Instance->GetInfantryData(cell->Infantry[i], obj);
						int nFacing = 7 - (atoi(obj.Facing) / 32) % 8;

						bool water = false;
						const auto& swim = CLoadingExt::SwimableInfantries;
						if (ExtConfigs::InGameDisplay_Water && std::find(swim.begin(), swim.end(), obj.TypeID) != swim.end())
						{
							auto landType = CMapDataExt::GetLandType(cell->TileIndex, cell->TileSubIndex);
							if (landType == LandType::Water || landType == LandType::Beach)
							{
								water = true;
							}
						}
						bool deploy = ExtConfigs::InGameDisplay_Deploy
							&& obj.Status == "Unload" && Variables::Rules.GetBool(obj.TypeID, "Deployer");

						const auto& imageName = CLoadingExt::GetImageName(obj.TypeID, nFacing, false, deploy && !water, water);

						if (!CLoadingExt::IsObjectLoaded(obj.TypeID))
						{
							CLoading::Instance->LoadObjects(obj.TypeID);
						}
						auto pData = CLoadingExt::GetImageDataFromServer(obj.TypeID, imageName);

						if (pData->pImageBuffer)
						{
							int x1 = x;
							int y1 = y;
							switch (atoi(obj.SubCell))
							{
							case 2:
								x1 += 15;
								y1 += 14;
								break;
							case 3:
								x1 -= 15;
								y1 += 14;
								break;
							case 4:
								y1 += 22;
								break;
							default:
								y1 += 15;
								break;
							}
							auto color = Miscs::GetColorRef(obj.House);
							CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary,
								x1 - pData->FullWidth / 2, y1 - pData->FullHeight / 2, pData, NULL, 255, color, 1, true);

							auto& veter = DrawVeterancies.emplace_back();
							int	VP = atoi(obj.VeterancyPercentage);
							veter.X = x1 - 5;
							veter.Y = y1 - 4 - 15;
							veter.VP = VP;
						}
					}
				}
			}

			//terrains
			if (cell->Terrain != -1 && CIsoViewExt::DrawTerrains)
			{
				auto obj = Variables::GetRulesMapValueAt("TerrainTypes", cell->TerrainType);
				const auto& imageName = CLoadingExt::GetImageName(obj, 0);

				if (!CLoadingExt::IsObjectLoaded(obj))
				{
					CLoading::Instance->LoadObjects(obj);
				}
				auto pData = CLoadingExt::GetImageDataFromServer(obj, imageName);

				if (pData->pImageBuffer)
				{
					CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary,
						x - pData->FullWidth / 2, y - pData->FullHeight / 2 + (Variables::Rules.GetBool(obj, "SpawnsTiberium") ? 0 : 12),
						pData, NULL, 255, 0, -1, false);

					if (auto pAIFile = Variables::Rules.TryGetString(obj, "AlphaImage"))
					{
						ppmfc::CString AIFile = *pAIFile;
						AIFile.Trim();
						auto pAIData = CLoadingExt::GetImageDataFromServer(obj, AIFile + "\233ALPHAIMAGE");

						if (pAIData && pAIData->pImageBuffer)
						{
							CIsoViewExt::BlitSHPTransparent_AlphaImage(pThis, lpDesc->lpSurface, window, boundary,
								x - pAIData->FullWidth / 2, y - pAIData->FullHeight / 2 + (Variables::Rules.GetBool(obj, "SpawnsTiberium") ? 0 : 12), pAIData);
						}
					}
				}
			}
		}
	}

	const char* InsigniaVeteran = "FA2spInsigniaVeteran";
	const char* InsigniaElite = "FA2spInsigniaElite";
	auto veteran = CLoadingExt::GetImageDataFromMap(InsigniaVeteran);
	auto elite = CLoadingExt::GetImageDataFromMap(InsigniaElite);
	for (auto& dv : DrawVeterancies)
	{
		if (dv.VP >= 200)
			CIsoViewExt::BlitSHPTransparent(lpDesc, dv.X - elite->FullWidth / 2 + 10, dv.Y + 21 - elite->FullHeight / 2, elite);
		else if (dv.VP >= 100)
			CIsoViewExt::BlitSHPTransparent(lpDesc, dv.X - veteran->FullWidth / 2 + 10, dv.Y + 21 - veteran->FullWidth / 2, veteran);
	}

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
				x1 -= DrawOffsetX;
				y1 -= DrawOffsetY;
				x2 -= DrawOffsetX;
				y2 -= DrawOffsetY;
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
			x1 -= DrawOffsetX;
			y1 -= DrawOffsetY;
			x2 -= DrawOffsetX;
			y2 -= DrawOffsetY;
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
					int drawX = x - DrawOffsetX;
					int drawY = y - DrawOffsetY;
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

				int drawX = X - DrawOffsetX;
				int drawY = Y - DrawOffsetY;

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

	for (int XplusY = Left + Top; XplusY < Right + Bottom; XplusY++) {
		for (int X = 0; X < XplusY; X++) {
			int Y = XplusY - X;

			if (!IsCoordInWindow(X, Y))
				continue;

			if (!CMapData::Instance->IsCoordInMap(X, Y))
				continue;

			int pos = CMapData::Instance->GetCoordIndex(X, Y);
			auto cell = CMapData::Instance->GetCellAt(pos);
			int x = X;
			int y = Y;
			CIsoView::MapCoord2ScreenCoord(x, y);
			x -= DrawOffsetX;
			y -= DrawOffsetY;

			if (CIsoViewExt::DrawCelltags && cell->CellTag != -1)
			{
				if (CIsoViewExt::DrawCellTagsFilter && !CViewObjectsExt::ObjectFilterCT.empty())
				{
					ppmfc::CString id = "";
					if (CMapData::Instance().INI.SectionExists("CellTags"))
						id = CMapData::Instance().INI.GetStringAt("CellTags", cell->CellTag);

					if (id != "")
					{
						for (auto& name : CViewObjectsExt::ObjectFilterCT)
						{
							if (name == id)
							{
								pThis->DrawCelltag(x, y, lpDesc);
								break;
							}
							if (STDHelpers::IsNumber(name))
							{
								int n = atoi(name);
								if (n < 1000000)
								{
									ppmfc::CString buffer;
									buffer.Format("%08d", n + 1000000);
									if (buffer == id)
									{
										pThis->DrawCelltag(x, y, lpDesc);
										break;
									}
								}
							}
						}
					}
				}
				else
					pThis->DrawCelltag(x, y, lpDesc);
			}

			if (CIsoViewExt::DrawWaypoints && cell->Waypoint != -1)
				pThis->DrawWaypointFlag(x, y, lpDesc);

			if (CMapDataExt::HasAnnotation(pos))
			{
				pThis->DrawBitmap("annotation", x + 5, y - 2, lpDesc);
			}
		}
	}

	auto& cellDataExt = CMapDataExt::CellDataExt_FindCell;
	if (cellDataExt.drawCell)
	{
		int x = cellDataExt.X;
		int y = cellDataExt.Y;

		CIsoView::MapCoord2ScreenCoord(x, y);

		int drawX = x - DrawOffsetX;
		int drawY = y - DrawOffsetY;

		pThis->DrawBitmap("target", drawX - 20, drawY - 11, lpDesc);
	}

	if (CIsoViewExt::DrawOverlays)
	{
		SetBkMode(hDC, TRANSPARENT);
		SetTextAlign(hDC, TA_CENTER);
		SetTextColor(hDC, RGB(0, 0, 0));
		for (const auto& [coord, index] : OverlayTextsToDraw)
		{
			if (IsCoordInWindow(coord.X, coord.Y))
			{
				MapCoord mc = coord;
				CIsoView::MapCoord2ScreenCoord(mc.X, mc.Y);
				int drawX = mc.X - DrawOffsetX + 30;
				int drawY = mc.Y - DrawOffsetY - 25;
				TextOut(hDC, drawX, drawY, index, strlen(index));
			}
		}
	}

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
			for (auto& pair : pSection->GetEntities())
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

						if (IsCoordInWindow(x, y))
						{
							CIsoView::MapCoord2ScreenCoord(x, y);

							int ndrawX = x - DrawOffsetX + 30;
							int ndrawY = y - DrawOffsetY - 15;

							TextOut(hDC, ndrawX, ndrawY, key, strlen(key));
						}
					}
				}
			}
		}
	}

	if (CIsoViewExt::DrawWaypoints)
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


		for (const auto& [coord, index] : WaypointsToDraw)
		{
			if (IsCoordInWindow(coord.X, coord.Y))
			{
				MapCoord mc = coord;
				CIsoView::MapCoord2ScreenCoord(mc.X, mc.Y);
				int drawX = mc.X - DrawOffsetX + 30 + ExtConfigs::Waypoint_Text_ExtraOffset.x;
				int drawY = mc.Y - DrawOffsetY - 15 + ExtConfigs::Waypoint_Text_ExtraOffset.y;
				TextOut(hDC, drawX, drawY, index, strlen(index));
			}
		}
	}

	SetTextAlign(hDC, TA_LEFT);
	SetTextColor(hDC, RGB(0, 0, 0));

	if (auto pSection = CINI::CurrentDocument->GetSection("Annotations"))
	{
		for (const auto& [key, value] : pSection->GetEntities())
		{
			auto pos = atoi(key);
			int x = pos / 1000;
			int y = pos % 1000;
			CIsoView::MapCoord2ScreenCoord(x, y);
			x -= DrawOffsetX;
			y -= DrawOffsetY;
			x += 23;
			y -= 15;
			auto atoms = STDHelpers::SplitString(value, 6);
			int fontSize = std::min(100, atoi(atoms[0]));
			fontSize = std::max(10, fontSize);
			bool bold = STDHelpers::IsTrue(atoms[1]);
			bool folded = STDHelpers::IsTrue(atoms[2]);
			auto textColor = STDHelpers::HexStringToColorRefRGB(atoms[3]);
			auto bgColor = STDHelpers::HexStringToColorRefRGB(atoms[4]);

			ppmfc::CString text = atoms[5];
			for (int i = 6; i < atoms.size() - 1; i++)
			{
				text += ",";
				text += atoms[i];
			}
			text.Replace("\\n", "\n");
			std::string str(text.m_pchData);
			auto result = STDHelpers::StringToWString(str);

			if (folded)
			{
				int count = 3;
				if (count < result.length() - 1)
				{
					if (IS_HIGH_SURROGATE(result[count - 1]) && IS_LOW_SURROGATE(result[count])) {
						count--;
					}
					result = result.substr(0, count);
					wchar_t toRemove = L'\n';
					result.erase(std::remove(result.begin(), result.end(), toRemove), result.end());
					result += L"...";
				}
				if (fontSize > 18)
					fontSize = 18;
			}
			CIsoViewExt::BlitText(result, textColor, bgColor,
				pThis, lpDesc->lpSurface, window, boundary, x, y, fontSize, 128, folded ? false : bold);
		}
	}

	if (CIsoViewExt::PasteShowOutline && CIsoView::CurrentCommand->Command == 21 && MultiSelection::MultiPastedCoords.empty())
	{
		auto& mapData = CMapData::Instance();

		auto length = mapData.MapWidthPlusHeight;

		int copyx = CIsoViewExt::CopyEnd.X - CIsoViewExt::CopyStart.X + 1;
		int copyy = CIsoViewExt::CopyEnd.Y - CIsoViewExt::CopyStart.Y + 1;

		while (CIsoViewExt::CopyStart.X < 0)
		{
			CIsoViewExt::CopyStart.X++;
			copyx--;
		}
		while (CIsoViewExt::CopyStart.Y < 0)
		{
			CIsoViewExt::CopyStart.Y++;
			copyy--;
		}
		while (CIsoViewExt::CopyStart.X + copyx > length)
		{
			copyx--;
		}
		while (CIsoViewExt::CopyStart.Y + copyy > length)
		{
			copyy--;
		}
		for (int x = CIsoViewExt::CopyStart.X; x < CIsoViewExt::CopyStart.X + copyx; ++x)
		{
			int X = x;
			int Y1 = CIsoViewExt::CopyStart.Y;
			int Y2 = CIsoViewExt::CopyStart.Y + copyy - 1;
			CIsoView::MapCoord2ScreenCoord(X, Y1);
			X -= DrawOffsetX;
			Y1 -= DrawOffsetY;
			pThis->DrawLockedCellOutline(X, Y1, 1, 1, ExtConfigs::CopySelectionBound_Color, false, false, lpDesc, false, false, false, true);
			X = x;
			CIsoView::MapCoord2ScreenCoord(X, Y2);
			X -= DrawOffsetX;
			Y2 -= DrawOffsetY;
			pThis->DrawLockedCellOutline(X, Y2, 1, 1, ExtConfigs::CopySelectionBound_Color, false, false, lpDesc, false, true, false, false);
		}
		for (int y = CIsoViewExt::CopyStart.Y; y < CIsoViewExt::CopyStart.Y + copyy; ++y)
		{
			int Y = y;
			int X1 = CIsoViewExt::CopyStart.X;
			int X2 = CIsoViewExt::CopyStart.X + copyx - 1;
			CIsoView::MapCoord2ScreenCoord(X1, Y);
			X1 -= DrawOffsetX;
			Y -= DrawOffsetY;
			pThis->DrawLockedCellOutline(X1, Y, 1, 1, ExtConfigs::CopySelectionBound_Color, false, false, lpDesc, true, false, false, false);
			Y = y;
			CIsoView::MapCoord2ScreenCoord(X2, Y);
			X2 -= DrawOffsetX;
			Y -= DrawOffsetY;
			pThis->DrawLockedCellOutline(X2, Y, 1, 1, ExtConfigs::CopySelectionBound_Color, false, false, lpDesc, false, false, true, false);
		}
	}
	else if (CIsoViewExt::PasteShowOutline && CIsoView::CurrentCommand->Command == 21 && !MultiSelection::MultiPastedCoords.empty())
	{
		for (auto& coord : MultiSelection::MultiPastedCoords)
		{
			int X = coord.X;
			int Y = coord.Y;
			CIsoView::MapCoord2ScreenCoord(X, Y);
			X -= DrawOffsetX;
			Y -= DrawOffsetY;
			pThis->DrawLockedCellOutline(X, Y, 1, 1, ExtConfigs::CopySelectionBound_Color, false, false, lpDesc);
		}
	}

	if (CIsoViewExt::DrawBounds)
	{
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
		int drawX1 = x1 - DrawOffsetX;
		int drawY1 = y1 - DrawOffsetY;

		CIsoView::MapCoord2ScreenCoord_Flat(x2, y2);
		int drawX2 = x2 - DrawOffsetX;
		int drawY2 = y2 - DrawOffsetY;

		pThis->DrawLine(drawX1, drawY1 - 15, drawX2, drawY2 - 15, RGB(0, 0, 255), false, false, lpDesc);
	}

	pThis->lpDDBackBufferSurface->ReleaseDC(hDC);

	return 0x474DB3;
}

DEFINE_HOOK(474DDF, CIsoView_Draw_SkipBounds, 5)
{
	return CIsoViewExt::DrawBounds ? 0 : 0x4750B0;
}

DEFINE_HOOK(474FE0, CIsoView_Draw_SkipMoneyOnMap, 5)
{
	return 0x4750B0;
}

DEFINE_HOOK(475187, CIsoView_Draw_End, 7)
{
	CIsoView::GetInstance()->ViewPosition = ViewPosition;
	return 0;
}
