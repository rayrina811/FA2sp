#include "Body.h"

#include <Helpers/Macro.h>

#include "../../FA2sp.h"
#include "../../Helpers/TheaterHelpers.h"
#include "../../Helpers/STDHelpers.h"

#include <Miscs/Miscs.h>
#include "../CIsoView/Body.h"
#include "../CFinalSunDlg/Body.h"
#include "../../Miscs/MultiSelection.h"
#include "../CTileSetBrowserFrame/TabPages/TriggerSort.h"
#include "../CTileSetBrowserFrame/TabPages/TeamSort.h"
#include "../CTileSetBrowserFrame/TabPages/WaypointSort.h"
#include "../CTileSetBrowserFrame/TabPages/TaskForceSort.h"
#include "../CTileSetBrowserFrame/TabPages/ScriptSort.h"
#include "../../Miscs/Palettes.h"
#include "../CLoading/Body.h"

DEFINE_HOOK(4C3E20, CMapData_CalculateMoneyCount, 7)
{
    auto pExt = CMapDataExt::GetExtension();

    int nCount = 0;

    for (int i = 0; i < pExt->CellDataCount; ++i)
        nCount += pExt->GetOreValueAt(pExt->CellDatas[i]);

    R->EAX(nCount);

    return 0x4C4460;
}

DEFINE_HOOK(4A1DB0, CMapData_AddTiberium, 6)
{
    auto pExt = CMapDataExt::GetExtension();

    GET_STACK(unsigned char, nOverlay, 0x4);
    GET_STACK(unsigned char, nOverlayData, 0x8);
    
    pExt->MoneyCount += CMapDataExt::GetExtension()->GetOreValue(nOverlay, nOverlayData);

    return 0x4A238E;
}

DEFINE_HOOK(4A17C0, CMapData_DeleteTiberium, 6)
{
    auto pExt = CMapDataExt::GetExtension();

    GET_STACK(unsigned char, nOverlay, 0x4);
    GET_STACK(unsigned char, nOverlayData, 0x8);

    pExt->MoneyCount -= CMapDataExt::GetExtension()->GetOreValue(nOverlay, nOverlayData);

    return 0x4A1D9E;
}

DEFINE_HOOK(4BB04A, CMapData_AddTube_IgnoreUselessNegativeOne, 7)
{
    GET(CTubeData*, pTubeData, ESI);
    REF_STACK(ppmfc::CString, lpBuffer, STACK_OFFS(0x134, 0x124));

    for (int i = 0; i < 100; ++i)
    {
        if (pTubeData->Directions[i] == -1)
        {
            lpBuffer += ",-1";
            break;
        }

        lpBuffer += ',';
        lpBuffer += pTubeData->Directions[i] + '0';
    }

    return 0x4BB083;
}

//DEFINE_HOOK(47AB50, CLoading_InitPics_InitOverlayTypeDatas, 7)
//{
//    CMapDataExt::OverlayTypeDatas.clear();
//
//    for (const auto& id : Variables::Rules.ParseIndicies("OverlayTypes", true))
//    {
//        auto& item = CMapDataExt::OverlayTypeDatas.emplace_back();
//        item.Rock = Variables::Rules.GetBool(id, "IsARock");
//        item.Wall = Variables::Rules.GetBool(id, "Wall");
//		item.WallPaletteName = CINI::Art->GetString(id, "Palette", "unit");
//    }
//
//    return 0;
//}


DEFINE_HOOK(49DFB4, CMapData_LoadMap_InvalidTheater, 6)
{
    GET(const char*, lpTheaterName, EDI);

    for (const auto& lpStr : TheaterHelpers::GetEnabledTheaterNames()) 
    {
        if (_strcmpi(lpStr, lpTheaterName) == 0)
            return 0;
    }

    return 0x49EDD9;
}

#define DEFINE_THEATER_NAME_FIX(addr, name, reg) \
DEFINE_HOOK(addr, CMapData_LoadMap_SupportLowerCaseTheaterName_##name, 5) \
{R->EAX(_strcmpi(R->reg<const char*>(), #name)); return (0x##addr + 0xE);}

DEFINE_THEATER_NAME_FIX(49DFB4, TEMPERATE, EDI);
DEFINE_THEATER_NAME_FIX(49E1D9, SNOW, EDI);
DEFINE_THEATER_NAME_FIX(49E3FF, URBAN, EDI);
DEFINE_THEATER_NAME_FIX(49E631, NEWURBAN, EDI);
DEFINE_THEATER_NAME_FIX(49E863, LUNAR, EDI);
DEFINE_THEATER_NAME_FIX(49EAA4, DESERT, ESI);

#undef DEFINE_THEATER_NAME_FIX


DEFINE_HOOK(4A4A40, CMapData_UpdateMapFieldData, 6)
{
	CMapDataExt::UpdateFieldStructureData_Optimized(-1);
	return 0x4A583F;
}


//DEFINE_HOOK(4A4AA6, CMapData_UpdateMapFieldData_Structures_RenderData_Clear, 6)
//{
//    CMapDataExt::BuildingRenderDatasFix.clear();
//    return 0;
//}
//
//DEFINE_HOOK(4A4C8E, CMapData_UpdateMapFieldData_Structures_RenderData_Load, 6)
//{
//    GET(ppmfc::CString*, pString, EAX);
//
//    const auto& splits = STDHelpers::SplitString(*pString);
//    
//    BuildingRenderData data;
//    data.HouseColor = Miscs::GetColorRef(splits[0]);
//    data.ID = splits[1];
//    data.Strength = std::clamp(atoi(splits[2]), 0, 256);
//    data.Y = atoi(splits[3]);
//    data.X = atoi(splits[4]);
//    data.Facing = atoi(splits[5]);
//    data.PowerUpCount = atoi(splits[10]);
//    data.PowerUp1 = splits[12];
//    data.PowerUp2 = splits[13];
//    data.PowerUp3 = splits[14];
//	int size = CMapDataExt::BuildingRenderDatasFix.size();
//    CMapDataExt::BuildingRenderDatasFix[size] = data;
//
//    R->ESP(R->ESP() + 0xC);
//    return 0x4A4F6D;
//}
//
//DEFINE_HOOK(4A4F7A, CMapData_UpdateMapFieldData_Structures_SKIPCOPY, 9)
//{
//    return 0x4A4F83;
//}
//
//DEFINE_HOOK(4A57CD, CMapData_UpdateMapFieldData_Structures_SKIPDTOR, F)
//{
//    return 0x4A5817;
//}

DEFINE_HOOK(4C3C20, CMapData_GetStructureRenderData, 6)
{
    GET_STACK(int, ID, 0x4);
    GET_STACK(CBuildingRenderData*, pRet, 0x8);

    if (ID >= 0 && CMapDataExt::BuildingRenderDatasFix.find(ID) != CMapDataExt::BuildingRenderDatasFix.end())
    {
        const auto& data = CMapDataExt::BuildingRenderDatasFix[ID];
        *reinterpret_cast<unsigned int*>(&pRet->HouseColor) = data.HouseColor;
        pRet->ID = data.ID;
        pRet->Strength = static_cast<unsigned char>(data.Strength);
        pRet->X = data.X;
        pRet->Y = data.Y;
        pRet->Facing = data.Facing;
        pRet->PowerUpCount = data.PowerUpCount;
        pRet->PowerUp1 = data.PowerUp1;
        pRet->PowerUp2 = data.PowerUp2;
        pRet->PowerUp3 = data.PowerUp3;

        CMapDataExt::CurrentRenderBuildingStrength = data.Strength;
    }

    return 0x4C3CCC;
}

DEFINE_HOOK(4A7CA7, CMapData_DeleteInfantry_UpdateIni, 6)//DeleteInfantry
{
	CMapData::Instance->UpdateFieldInfantryData(TRUE);
	return 0;
}

DEFINE_HOOK(4AC210, CMapData_Update_InfantrySubCell3, 7)//AddInfantry
{

	GET_STACK(CInfantryData*, lpInfantry, 0x4);
	GET_STACK(LPCTSTR, lpType, 0x8);
	GET_STACK(LPCTSTR, lpHouse, 0xC);
	GET_STACK(DWORD, dwPos, 0x10);
	GET_STACK(int, suggestedIndex, 0x14);


	auto Map = &CMapData::Instance();
	auto& m_infantry = Map->InfantryDatas;
	auto& fielddata_size = Map->CellDataCount;
	auto& fielddata = Map->CellDatas;
	CInfantryData infantry;


	if (lpInfantry != NULL)
	{
		infantry = *lpInfantry;
		dwPos = atoi(infantry.X) + atoi(infantry.Y) * Map->MapWidthPlusHeight;

		// MW Bugfix: not checking if infantry.SubCell does already exist caused crashes with user scripts!
		if (CMapDataExt::GetInfantryAt(dwPos, atoi(infantry.SubCell)) >= 0)
			infantry.SubCell = "-1";
	}
	else
	{
		char cx[10], cy[10];
		_itoa(dwPos % Map->MapWidthPlusHeight, cx, 10);
		_itoa(dwPos / Map->MapWidthPlusHeight, cy, 10);



		infantry.Status = "Guard";
		infantry.Tag = "None";
		infantry.Facing = "64";
		infantry.VeterancyPercentage = "0";
		infantry.Group = "-1";
		infantry.IsAboveGround = "0";
		infantry.AutoNORecruitType = "0";
		infantry.AutoYESRecruitType = "0";
		infantry.Health = "256";
		infantry.House = lpHouse;
		infantry.SubCell = "-1";
		infantry.TypeID = lpType;
		infantry.X = cx;
		infantry.Y = cy;
	}
	if (CIsoViewExt::AutoPropertyBrush[2])
	{
		CViewObjectsExt::ApplyPropertyBrush_Infantry(infantry);
	}

	if (infantry.SubCell == "-1")
	{
		int subpos = -1;
		int i;

		if (Map->GetInfantryCountAt(dwPos) == 0)
		{
			if (ExtConfigs::InfantrySubCell_GameDefault)
				subpos = 4;
			else
				subpos = 0;
		}

		else
		{
			int oldInf = Map->GetInfantryAt_pos(dwPos, 0);
			if (oldInf > -1)
			{
				CInfantryData inf;
				Map->GetInfantryData(oldInf, inf);



				if (inf.SubCell == "0")
					for (i = 1; i < 3; i++)
					{
						if (Map->GetInfantryAt_pos(dwPos, i) == -1)
						{
							char c[50];
							_itoa(i, c, 10);
							inf.SubCell = c;
							Map->DeleteInfantryData(oldInf);
							Map->SetInfantryData(&inf, NULL, NULL, 0, -1);
							break;
						}
					}
			}

			for (i = 0; i < 3; i++)
			{
				if (Map->GetInfantryAt_pos(dwPos, i) == -1)
				{
					subpos = i + 1;
					if (ExtConfigs::InfantrySubCell_GameDefault && i == 0)
						subpos = 4;
					break;
				}
			}
		}

		if (subpos < 0) return 0x4ACA49;
		char c[50];
		_itoa(subpos, c, 10);

		infantry.SubCell = c;
	}

	infantry.Flag = 0;

	if (ExtConfigs::InfantrySubCell_OccupationBits)
	{

		auto cell = Map->TryGetCellAt(atoi(infantry.X), atoi(infantry.Y));
		if (cell->Terrain > -1)
		{
			std::vector<int> availableSubcells;
			std::vector<int> availableAndEmptySubcells;
			ppmfc::CString name;
			ppmfc::CString key = "TemperateOccupationBits";
			auto theaterIg = CINI::CurrentDocument->GetString("Map", "Theater");
			if (theaterIg == "SNOW")
				key = "SnowOccupationBits";

			if (auto pTerrain = CINI::Rules().GetSection("TerrainTypes"))
				name = *pTerrain->GetValueAt(cell->TerrainType);
			int bits = (CINI::Rules().GetInteger(name, key, 7) % 8 + 8) % 8;
			switch (bits)
			{
			case 0:
				availableSubcells.push_back(2);
				availableSubcells.push_back(3);
				availableSubcells.push_back(4);
				break;
			case 1:
				availableSubcells.push_back(3);
				availableSubcells.push_back(4);
				break;
			case 2:
				availableSubcells.push_back(2);
				availableSubcells.push_back(4);
				break;
			case 3:
				availableSubcells.push_back(4);
				break;
			case 4:
				availableSubcells.push_back(2);
				availableSubcells.push_back(3);
				break;
			case 5:
				availableSubcells.push_back(3);
				break;
			case 6:
				availableSubcells.push_back(2);
				break;
			case 7:
				break;
			default:
				break;
			}

			if (availableSubcells.empty())
				return 0x4ACA49;

			for (int subcell : availableSubcells)
			{
				bool available = true;
				for (int i = 0; i < 3; i++)
				{
					int pos = CMapData::Instance->CellDatas[dwPos].Infantry[i];
					if (pos > -1)
					{
						CInfantryData inf;
						Map->GetInfantryData(pos, inf);
						if (atoi(inf.SubCell) == subcell)
							available = false;
					}
				}
				if (available)
					availableAndEmptySubcells.push_back(subcell);
			}

			if (availableAndEmptySubcells.empty())
				return 0x4ACA49;

			int decidedSC = availableAndEmptySubcells[0];
			for (auto subcell : availableAndEmptySubcells)
			{
				if (atoi(infantry.SubCell) == subcell)
				{
					decidedSC = subcell;
					break;
				}
			}

			infantry.SubCell.Format("%d", decidedSC);
		}
	}


	int sp = atoi(infantry.SubCell);
	if (sp > 0) sp--;
	if (sp == 3)
		sp = 0;

	int i;
	bool bFound = false;
	if (suggestedIndex >= 0 && suggestedIndex < m_infantry.size())
	{
		if (m_infantry[suggestedIndex].Flag)
		{
			m_infantry[suggestedIndex] = infantry;
			if (dwPos < fielddata_size) fielddata[dwPos].Infantry[sp] = suggestedIndex;
			bFound = true;

		}
	}

	if (!bFound)
		for (i = 0; i < m_infantry.size(); i++)
		{
			if (m_infantry[i].Flag) // yep, found one, replace it
			{
				m_infantry[i] = infantry;
				if (dwPos < fielddata_size) fielddata[dwPos].Infantry[sp] = i;
				bFound = true;
				break;
			}
		}

	if (!bFound)
	{
		m_infantry.push_back(infantry);//默认exe中100个步兵会崩溃，需要修补004A2B62和004A2BC8（60*n）
		if (dwPos < fielddata_size) fielddata[dwPos].Infantry[sp] = (short)m_infantry.size() - 1;
	}

	Map->UpdateFieldInfantryData(true);
	CMapData::Instance->UpdateMapPreviewAt(atoi(infantry.X), atoi(infantry.Y));

	return 0x4ACA49;
}
std::vector<int>GetStructureSize(ppmfc::CString structure)
{
	std::vector<int> result;
	MultimapHelper mmh;
	mmh.AddINI(&CINI::Rules());
	mmh.AddINI(&CINI::CurrentDocument());
	auto art = &CINI::Art();
	auto image = mmh.GetString(structure, "Image", structure);
	std::string foundation = std::string(art->GetString(image, "Foundation", "1X1"));
	if (foundation == "")
		foundation = "1X1";
	std::transform(foundation.begin(), foundation.end(), foundation.begin(), (int(*)(int))tolower);

	if (foundation == "custom")
	{
		std::string x = std::string(art->GetString(image, "Foundation.X", "5"));
		std::string y = std::string(art->GetString(image, "Foundation.Y", "5"));
		foundation = x + "x" + y;
	}
	auto found = STDHelpers::SplitString(foundation.c_str(), "x");
	result.push_back(atoi(found[1]));
	result.push_back(atoi(found[0]));
	return result;
}

DEFINE_HOOK(4ACB60, CMapData_Update_AddBuilding, 7)
{

	GET_STACK(CBuildingData*, lpStructure, 0x4);
	GET_STACK(LPCTSTR, lpType, 0x8);
	GET_STACK(LPCTSTR, lpHouse, 0xC);
	GET_STACK(DWORD, dwPos, 0x10);
	GET_STACK(ppmfc::CString, suggestedID, 0x14);


	auto Map = &CMapData::Instance();
	auto& fielddata_size = Map->CellDataCount;
	auto& fielddata = Map->CellDatas;
	CMapData::Instance->UpdateCurrentDocument();
	auto m_mapfile = Map->GetMapDocument();

	bool skipOverlapping = false;
	bool overlappingWarning = false;

	CBuildingData structure;
	if (lpStructure != NULL)
	{
		structure = *lpStructure;
		skipOverlapping = true;
		
	}
	else
	{
		char cx[10], cy[10];
		_itoa(dwPos % Map->MapWidthPlusHeight, cx, 10);
		_itoa(dwPos / Map->MapWidthPlusHeight, cy, 10);

		structure.Tag = "None";
		structure.Facing = "0";
		if (ExtConfigs::AISellableDefaultYes)
			structure.AISellable = "1";
		else
			structure.AISellable = "0";
		structure.AIRebuildable = "0";
		structure.SpotLight = "0";
		if (ExtConfigs::AIRepairDefaultYes)
			structure.AIRepairable = "1";
		else
			structure.AIRepairable = "0";
		structure.Nominal = "0";
		structure.PoweredOn = "1";
		structure.Upgrade1 = "None";
		structure.Upgrade2 = "None";
		structure.Upgrade3 = "None";
		structure.Upgrades = "0";
		structure.Health = "256";
		structure.House = lpHouse;
		structure.TypeID = lpType;
		structure.X = cx;
		structure.Y = cy;
		
	}
	if (ExtConfigs::PlaceStructureUpgrades) 
	{
		int upgradeCount = 0;
		if (structure.Upgrade1 != "None")
			upgradeCount++;
		if (structure.Upgrade2 != "None")
			upgradeCount++;
		if (structure.Upgrade3 != "None")
			upgradeCount++;
		structure.Upgrades = STDHelpers::IntToString(upgradeCount);
	}
	if (ExtConfigs::PlaceStructureUpgradeStrength)
	{
		int upgradeCount = 0;
		if (structure.Upgrade1 != "None")
			upgradeCount++;
		if (structure.Upgrade2 != "None")
			upgradeCount++;
		if (structure.Upgrade3 != "None")
			upgradeCount++;
		if (upgradeCount > 0)
		{
			structure.Health = "256";
		}
	}


	if (CIsoViewExt::AutoPropertyBrush[1])
	{
		CViewObjectsExt::ApplyPropertyBrush_Building(structure);
	}

	auto pSection = m_mapfile->GetSection("Structures");
	if (pSection)
	{
		// check overlap
		auto& ignoreList = CINI::FAData->GetSection("StructureOverlappingCheckIgnores")->GetEntities();
		bool skipCheck = false;
		for (const auto& pair : ignoreList)
		{
			if (pair.second == structure.TypeID)
				skipCheck = true;

		}

		if (!skipCheck && ExtConfigs::PlaceStructureOverlappingCheck)
		{
			std::map<int, int> Occupied;
			int length = CMapData::Instance->MapWidthPlusHeight;
			length *= length;

			if (1)
			{
				const int Index = CMapData::Instance->GetBuildingTypeID(structure.TypeID);
				const int Y = atoi(structure.Y);
				const int X = atoi(structure.X);
				const auto& DataExt = CMapDataExt::BuildingDataExts[Index];

				if (!DataExt.IsCustomFoundation())
				{
					for (int dx = 0; dx < DataExt.Height; ++dx)
					{
						for (int dy = 0; dy < DataExt.Width; ++dy)
						{
							MapCoord coord = { X + dx, Y + dy };
							if (CMapData::Instance->IsCoordInMap(coord.X, coord.Y))
							{
								Occupied[CMapData::Instance->GetCoordIndex(coord.X, coord.Y)] = 114514;
							}
								
						}
					}
				}
				else
				{
					for (const auto& block : *DataExt.Foundations)
					{
						MapCoord coord = { X + block.Y, Y + block.X };
						if (CMapData::Instance->IsCoordInMap(coord.X, coord.Y))
						{
							Occupied[CMapData::Instance->GetCoordIndex(coord.X, coord.Y)] = 114514;
						}
							
					}
				}
			}


			for (const auto& pair : pSection->GetEntities())
			{
				bool skipThis = false;
				auto values = STDHelpers::SplitString(pair.second, 4);
				auto& type = values[1];

				for (const auto& pair2 : ignoreList)
					if (pair2.second == type)
						skipThis = true;
				if (skipThis)
					continue;

				const int Index = CMapData::Instance->GetBuildingTypeID(type);
				const int Y = atoi(values[3]);
				const int X = atoi(values[4]);
				const auto& DataExt = CMapDataExt::BuildingDataExts[Index];

				if (!DataExt.IsCustomFoundation())
				{
					for (int dx = 0; dx < DataExt.Height; ++dx)
					{
						for (int dy = 0; dy < DataExt.Width; ++dy)
						{
							MapCoord coord = { X + dx, Y + dy };
							if (CMapData::Instance->IsCoordInMap(coord.X, coord.Y))
								if (Occupied.find(CMapData::Instance->GetCoordIndex(coord.X, coord.Y)) != Occupied.end())
									if (Occupied[CMapData::Instance->GetCoordIndex(coord.X, coord.Y)] >= 114514)
										Occupied[CMapData::Instance->GetCoordIndex(coord.X, coord.Y)] += 1;
						}
					}
				}
				else
				{
					for (const auto& block : *DataExt.Foundations)
					{
						MapCoord coord = { X + block.Y, Y + block.X };
						if (CMapData::Instance->IsCoordInMap(coord.X, coord.Y))
							if (Occupied.find(CMapData::Instance->GetCoordIndex(coord.X, coord.Y)) != Occupied.end())
								if (Occupied[CMapData::Instance->GetCoordIndex(coord.X, coord.Y)] >= 114514)
									Occupied[CMapData::Instance->GetCoordIndex(coord.X, coord.Y)] += 1;
					}
				}
			}
			for (auto& o : Occupied)
			{
				if (o.second > 114514)
					if (!skipOverlapping)
						return 0x4AD921;
					else
						overlappingWarning = true;
			}
		}

	}
	if (overlappingWarning)
	{
		ppmfc::CString pMessage2;
		ppmfc::CString pMessage = Translations::TranslateOrDefault("PlaceStructureOverlapMessage", 
			"The current operating structure %s overlaps with other structures.\nDelete or undo is strongly recommended!");
		pMessage2.Format(pMessage, structure.TypeID);

		MessageBox(CFinalSunDlg::Instance->m_hWnd, pMessage2, Translations::TranslateOrDefault("PlaceStructureOverlapTitle",
			"Structure Overlap") , MB_OK | MB_ICONEXCLAMATION);
	}
		
	ppmfc::CString id = CINI::GetAvailableKey("Structures");
	

	if (suggestedID.GetLength() > 0)
	{
		if (pSection->GetEntities().find(suggestedID) == pSection->GetEntities().end())
			id = suggestedID;
	}

	ppmfc::CString value;
	value = structure.House + "," + structure.TypeID + "," + structure.Health + "," + structure.Y +
		"," + structure.X + "," + structure.Facing + "," + structure.Tag + "," + structure.AISellable + "," +
		structure.AIRebuildable + "," + structure.PoweredOn + "," + structure.Upgrades + "," + structure.SpotLight + ","
		+ structure.Upgrade1 + "," + structure.Upgrade2 + "," + structure.Upgrade3 + "," + structure.AIRepairable + "," + structure.Nominal;

	if (!pSection)
		pSection = m_mapfile->AddOrGetSection("Structures");

	m_mapfile->WriteString("Structures", id, value);

	int realid = 0;
	for (const auto& pair : pSection->GetEntities())
	{
		if (pair.first == id)
			break;

		realid++;
	}

	bool isLamp = LightingSourceTint::IsLamp(structure.TypeID);

	if (!MultiSelection::AddBuildingOptimize)
		CMapDataExt::UpdateFieldStructureData_Optimized(realid, isLamp);
	else if (isLamp)
		LightingSourceTint::CalculateMapLamps();

	return 0x4AD921;
}

DEFINE_HOOK(466FE0, CIsoView_Update_DragBuilding, 5)
{
	GET_STACK(UINT, nFlags, 0x218);
	GET_STACK(int, screenCoordX, 0x21C);
	GET_STACK(int, screenCoordY, 0x220);

	auto Map = &CMapData::Instance();
	auto isoView = CIsoView::GetInstance();
	auto m_id = isoView->CurrentCellObjectIndex;
	
	int X = screenCoordX;
	int	Y = screenCoordY;
	CPoint point(X, Y);

	auto mapCoord = isoView->GetCurrentMapCoord(point);


	CBuildingData structure;
	Map->GetBuildingData(m_id, structure);
	structure.X = std::to_string(mapCoord.X).c_str();
	structure.Y = std::to_string(mapCoord.Y).c_str();

	if ((nFlags != MK_SHIFT))
	{
		CMapDataExt::SkipUpdateBuildingInfo = true;
		Map->DeleteBuildingData(m_id);
	}
	Map->SetBuildingData(&structure, NULL, NULL, 0, "");


	return 0x467682;
}

DEFINE_HOOK(45FA55, CIsoView_Update_SetBuildingProperty, 6)
{
	CMapDataExt::SkipUpdateBuildingInfo = true;
	return 0;
}
DEFINE_HOOK(46CD45, CIsoView_Update_ChangeBuildingOwner, 6)
{
	CMapDataExt::SkipUpdateBuildingInfo = true;
	return 0;
}

DEFINE_HOOK(4A8FB0, CMapData_DeleteStructure, 7)
{
	GET_STACK(unsigned int, m_id, 0x4);
	auto& mapdata = CMapData::Instance();
	auto& ini = CINI::CurrentDocument;
	if (!ini->SectionExists("Structures"))
		return 0x4A98AC;

	auto pSection = ini->GetSection("Structures");
	if (m_id >= pSection->GetEntities().size())
		return 0x4A98AC;

	ppmfc::CString key = *pSection->GetKeyAt(m_id);
	ppmfc::CString value = *pSection->GetValueAt(m_id);
	auto splits =STDHelpers::SplitString(value, 4);
	bool isLamp = LightingSourceTint::IsLamp(splits[1]);
	int Index = CMapData::Instance->GetBuildingTypeID(splits[1]);
	int Y = atoi(splits[3]);
	int X = atoi(splits[4]);
	auto& DataExt = CMapDataExt::BuildingDataExts[Index];

	if (!DataExt.IsCustomFoundation())
	{
		for (int dy = 0; dy < DataExt.Width; ++dy)
		{
			for (int dx = 0; dx < DataExt.Height; ++dx)
			{
				const int x = X + dx;
				const int y = Y + dy;
				const int pos = CMapData::Instance->GetCoordIndex(x, y);
				if (pos < CMapData::Instance->CellDataCount)
				{
					auto pCell = CMapData::Instance->GetCellAt(x, y);
					pCell->Structure = -1;
					pCell->TypeListIndex = -1;
					CMapData::Instance->UpdateMapPreviewAt(x, y);
				}
			}
		}
	}
	else
	{
		for (const auto& block : *DataExt.Foundations)
		{
			const int x = X + block.Y;
			const int y = Y + block.X;
			if (CMapData::Instance->GetCoordIndex(x, y) < CMapData::Instance->CellDataCount)
			{
				auto pCell = CMapData::Instance->GetCellAt(x, y);
				pCell->Structure = -1;
				pCell->TypeListIndex = -1;
				CMapData::Instance->UpdateMapPreviewAt(x, y);
			}
		}
	}


	bool overlap = false;
	if (pSection)
	{
		std::map<int, int> Occupied;
		int length = CMapData::Instance->MapWidthPlusHeight;
		length *= length;

		if (1)
		{
			const int Index = CMapData::Instance->GetBuildingTypeID(splits[1]);
			const int Y = atoi(splits[3]);
			const int X = atoi(splits[4]);
			const auto& DataExt = CMapDataExt::BuildingDataExts[Index];

			if (!DataExt.IsCustomFoundation())
			{
				for (int dx = 0; dx < DataExt.Height; ++dx)
				{
					for (int dy = 0; dy < DataExt.Width; ++dy)
					{
						MapCoord coord = { X + dx, Y + dy };
						if (CMapData::Instance->IsCoordInMap(coord.X, coord.Y))
						{
							Occupied[CMapData::Instance->GetCoordIndex(coord.X, coord.Y)] = 114514;
						}

					}
				}
			}
			else
			{
				for (const auto& block : *DataExt.Foundations)
				{
					MapCoord coord = { X + block.Y, Y + block.X };
					if (CMapData::Instance->IsCoordInMap(coord.X, coord.Y))
					{
						Occupied[CMapData::Instance->GetCoordIndex(coord.X, coord.Y)] = 114514;
					}

				}
			}
		}

		for (const auto& pair : pSection->GetEntities())
		{
			if (pair.first == key)
				continue;
			bool skipThis = false;
			auto values = STDHelpers::SplitString(pair.second, 4);
			auto& type = values[1];

			const int Index = CMapData::Instance->GetBuildingTypeID(type);
			const int Y = atoi(values[3]);
			const int X = atoi(values[4]);
			const auto& DataExt = CMapDataExt::BuildingDataExts[Index];

			if (!DataExt.IsCustomFoundation())
			{
				for (int dx = 0; dx < DataExt.Height; ++dx)
				{
					for (int dy = 0; dy < DataExt.Width; ++dy)
					{
						MapCoord coord = { X + dx, Y + dy };
						if (CMapData::Instance->IsCoordInMap(coord.X, coord.Y))
							if (Occupied.find(CMapData::Instance->GetCoordIndex(coord.X, coord.Y)) != Occupied.end())
								if (Occupied[CMapData::Instance->GetCoordIndex(coord.X, coord.Y)] >= 114514)
									Occupied[CMapData::Instance->GetCoordIndex(coord.X, coord.Y)] += 1;
					}
				}
			}
			else
			{
				for (const auto& block : *DataExt.Foundations)
				{
					MapCoord coord = { X + block.Y, Y + block.X };
					if (CMapData::Instance->IsCoordInMap(coord.X, coord.Y))
						if (Occupied.find(CMapData::Instance->GetCoordIndex(coord.X, coord.Y)) != Occupied.end())
							if (Occupied[CMapData::Instance->GetCoordIndex(coord.X, coord.Y)] >= 114514)
								Occupied[CMapData::Instance->GetCoordIndex(coord.X, coord.Y)] += 1;
				}
			}
		}

		for (auto& o : Occupied)
		{
			if (o.second > 114514)
				overlap = true;
		}
	}

	if (overlap)
	{
		ini->DeleteKey("Structures", key);
		CMapDataExt::UpdateFieldStructureData_Optimized(0, isLamp);
		CMapDataExt::SkipUpdateBuildingInfo = false;
	}
	else if (!CMapDataExt::SkipUpdateBuildingInfo)
	{
		ini->DeleteKey("Structures", key);
		CMapDataExt::UpdateFieldStructureData_Optimized(m_id, isLamp);
	}
	else
	{
		ini->DeleteKey("Structures", key);
		CMapDataExt::SkipUpdateBuildingInfo = false;
	}

	if (isLamp)
	{
		LightingSourceTint::CalculateMapLamps();
	}

	return 0x4A98AC;
}

DEFINE_HOOK(466E00, CIsoView_OnLButtonUp_DragFacing, 7)
{
	GET_STACK(UINT, nFlags, 0x218);
	GET_STACK(int, screenCoordX, 0x21C);
	GET_STACK(int, screenCoordY, 0x220);

	if (nFlags == MK_CONTROL)
	{
		auto Map = &CMapData::Instance();
		auto isoView = CIsoView::GetInstance();
		auto m_id = isoView->CurrentCellObjectIndex;
		auto m_type = isoView->CurrentCellObjectType;

		CBuildingData structure;
		CInfantryData infantry;
		CUnitData unit;
		CAircraftData aircraft;

		int X = screenCoordX;
		int	Y = screenCoordY;
		CPoint point(X, Y);
		auto newMapCoord = isoView->GetCurrentMapCoord(point);

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
DEFINE_HOOK(4B4996, CMapData_UpdateMapFieldData_NoRndForBridge, 6)
{
	GET(DWORD, dwID6, EAX);

	int dwID = dwID6 >> 6;
	if (dwID < *CTileTypeClass::InstanceCount)
		if (CMapDataExt::TileData[dwID].TileSet == CMapDataExt::WoodBridgeSet)
			return 0x4B4B7E;

	return 0x4B499C;
}

DEFINE_HOOK(49ED34, CMapData_LoadMap_InitializeMapDataExt, 5)
{
	Logger::Debug("CMapData::LoadMap(): About to call InitializeAllHdmEdition()\n");
	CMapDataExt::InitializeAllHdmEdition();
	return 0;
}

DEFINE_HOOK(4B9F7A, CMapData_CreateMap_InitializeMapDataExt, 5)
{
	Logger::Debug("CMapData::CreateMap(): About to call InitializeAllHdmEdition()\n");
	CMapDataExt::InitializeAllHdmEdition();
	return 0;
}

DEFINE_HOOK(4C6456, CMapData_ResizeMap_ResizeCellDataExts, 8)
{
	// resize for addbuilding
	CMapDataExt::CellDataExts.resize(CMapData::Instance->CellDataCount);
	return 0;
}

DEFINE_HOOK(4C7DAF, CMapData_ResizeMap_InitializeMapDataExt, 7)
{
	// load objects to avoid weird palette issue
	CIsoView::GetInstance()->PrimarySurfaceLost();

	for (int i = 0; i < CMapData::Instance->MapWidthPlusHeight; i++) {
		for (int j = 0; j < CMapData::Instance->MapWidthPlusHeight; j++) {
			CMapData::Instance->UpdateMapPreviewAt(i, j);
		}
	}

	return 0;
}

DEFINE_HOOK(4A2872, CMapData_UpdateMapPreviewAt_OverlayColor, 7)
{
	GET_STACK(unsigned char, Overlay, STACK_OFFS(0x78, 0x20));
	GET_STACK(RGBTRIPLE*, Color_r, STACK_OFFS(0x78, 0x68));
	GET(RGBTRIPLE*, Color, EBP);
	GET_STACK(CellData, cell, STACK_OFFS(0x78, 0x4C));
	auto setColor = [&](unsigned char R, unsigned char G, unsigned char B) {
		Color->rgbtBlue = B;
		Color->rgbtGreen = G;
		Color->rgbtRed = R;
		Color_r->rgbtBlue = B;
		Color_r->rgbtGreen = G;
		Color_r->rgbtRed = R;
		};
	if (Overlay != 0xFF) 
	{
		const auto& color = CMapDataExt::OverlayTypeDatas[Overlay].RadarColor;
		setColor(color.R, color.G, color.B);
	}

	return 0;
}
DEFINE_HOOK(4A28B1, CMapData_UpdateMapPreviewAt_Terrain, 7)
{
	GET_STACK(RGBTRIPLE*, Color_r, STACK_OFFS(0x78, 0x68));
	GET(RGBTRIPLE*, Color, EBP);
	GET_STACK(CellData, cell, STACK_OFFS(0x78, 0x4C));
	auto setColor = [&](unsigned char R, unsigned char G, unsigned char B) {
		Color->rgbtBlue = B;
		Color->rgbtGreen = G;
		Color->rgbtRed = R;
		Color_r->rgbtBlue = B;
		Color_r->rgbtGreen = G;
		Color_r->rgbtRed = R;
		};
	auto safeColorBtye = [](int x)
		{
			if (x > 255)
				x = 255;
			if (x < 0)
				x = 0;
			return (byte)x;
		};

	RGBTRIPLE result;
	auto& ret = LightingStruct::CurrentLighting;
	bool realColor = true;
	switch (CFinalSunDlgExt::CurrentLighting)
	{
	case 31001:
	case 31002:
	case 31003:
		break;
	default:
		result.rgbtRed = safeColorBtye(Color->rgbtRed + cell.Height * 2);
		result.rgbtGreen = safeColorBtye(Color->rgbtGreen + cell.Height * 2);
		result.rgbtBlue = safeColorBtye(Color->rgbtBlue + cell.Height * 2);
		realColor = false;
		break;
	}
	if (realColor) {
		result.rgbtRed = safeColorBtye((Color->rgbtRed * (ret.Ambient - ret.Ground + ret.Level * cell.Height)) * ret.Red);
		result.rgbtGreen = safeColorBtye((Color->rgbtGreen * (ret.Ambient - ret.Ground + ret.Level * cell.Height)) * ret.Green);
		result.rgbtBlue = safeColorBtye((Color->rgbtBlue * (ret.Ambient - ret.Ground + ret.Level * cell.Height)) * ret.Blue);
	}

	setColor(result.rgbtRed, result.rgbtGreen, result.rgbtBlue);

	return CMapData::Instance->IsMultiOnly() ? 0x4A28C0 : 0x4A2968;
}