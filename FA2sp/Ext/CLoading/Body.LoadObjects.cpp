#include "Body.h"

#include <CINI.h>
#include <CMapData.h>
#include <CMixFile.h>
#include <Drawing.h>

#include "../../Miscs/VoxelDrawer.h"
#include "../../Miscs/Palettes.h"
#include "../../FA2sp.h"
#include "../../Algorithms/Matrix3D.h"
#include "../CMapData/Body.h"

std::vector<CLoadingExt::SHPUnionData> CLoadingExt::UnionSHP_Data[2];
std::vector<CLoadingExt::SHPUnionData> CLoadingExt::UnionSHPShadow_Data[2];
std::map<ppmfc::CString, CLoadingExt::ObjectType> CLoadingExt::ObjectTypes;
unsigned char CLoadingExt::VXL_Data[0x10000] = {0};
unsigned char CLoadingExt::VXL_Shadow_Data[0x10000] = {0};
std::vector<ppmfc::CString> CLoadingExt::LoadedOverlays;

ppmfc::CString CLoadingExt::GetImageName(ppmfc::CString ID, int nFacing, bool bShadow, bool bDeploy, bool bWater)
{
	if (bShadow || bDeploy || bWater)
		ID.Format("%s%d\233%s%s%s", ID, nFacing, bDeploy ? "DEPLOY" : "", bWater ? "WATER" : "", bShadow ? "SHADOW" : "");
	else
		ID.Format("%s%d", ID, nFacing);
	return ID;
}

ppmfc::CString CLoadingExt::GetBuildingImageName(ppmfc::CString ID, int nFacing, int state, bool bShadow)
{
	if (state == GBIN_DAMAGED)
	{
		if (bShadow)
			ID.Format("%s%d\233DAMAGEDSHADOW", ID, nFacing);
		else
			ID.Format("%s%d\233DAMAGED", ID, nFacing);
	}
	else if (state == GBIN_RUBBLE)
	{
		if (bShadow)
		{
			if (Variables::Rules.GetBool(ID, "LeaveRubble"))
				ID.Format("%s0\233RUBBLESHADOW", ID);
			else if (!ExtConfigs::HideNoRubbleBuilding)// use damaged art, save memory
				ID.Format("%s%d\233DAMAGEDSHADOW", ID, nFacing);
			else // hide rubble
				ID = "\233\144\241"; // invalid string to get it empty
		}
		else
		{
			if (Variables::Rules.GetBool(ID, "LeaveRubble"))
				ID.Format("%s0\233RUBBLE", ID);
			else if (!ExtConfigs::HideNoRubbleBuilding)// use damaged art, save memory
				ID.Format("%s%d\233DAMAGED", ID, nFacing);
			else // hide rubble
				ID = "\233\144\241"; // invalid string to get it empty
		}

	}
	else // GBIN_NORMAL
	{
		if (bShadow)
			ID.Format("%s%d\233SHADOW", ID, nFacing);
		else
			ID.Format("%s%d", ID, nFacing);
	}
	return ID;
}

CLoadingExt::ObjectType CLoadingExt::GetItemType(ppmfc::CString ID)
{
	if (ObjectTypes.size() == 0)
	{
		auto load = [](ppmfc::CString type, ObjectType e)
		{
			auto section = Variables::Rules.GetSection(type);
			for (auto& pair : section)
				ObjectTypes[pair.second] = e;
		};

		load("InfantryTypes", ObjectType::Infantry);
		load("VehicleTypes", ObjectType::Vehicle);
		load("AircraftTypes", ObjectType::Aircraft);
		load("BuildingTypes", ObjectType::Building);
		load("SmudgeTypes", ObjectType::Smudge);
		load("TerrainTypes", ObjectType::Terrain);
	}

	auto itr = ObjectTypes.find(ID);
	if (itr != ObjectTypes.end())
		return itr->second;
	return ObjectType::Unknown;
}

void CLoadingExt::LoadObjects(ppmfc::CString ID)
{
	if (ID == "")
		return;

    Logger::Debug("CLoadingExt::LoadObjects loading: %s\n", ID);

	// GlobalVars::CMapData->UpdateCurrentDocument();
	auto eItemType = GetItemType(ID);
	switch (eItemType)
	{
	case CLoadingExt::ObjectType::Infantry:
		LoadInfantry(ID);
		break;
	case CLoadingExt::ObjectType::Terrain:
		LoadTerrainOrSmudge(ID, true);
		break;
	case CLoadingExt::ObjectType::Smudge:
		LoadTerrainOrSmudge(ID, false);
		break;
	case CLoadingExt::ObjectType::Vehicle:
	{
		LoadVehicleOrAircraft(ID);
		if (ExtConfigs::InGameDisplay_Deploy)
		{
			auto unloadingClass = Variables::Rules.GetString(ID, "UnloadingClass", ID);
			if (unloadingClass != ID)
			{
				LoadVehicleOrAircraft(unloadingClass);
			}
		}
		if (ExtConfigs::InGameDisplay_Water)
		{
			auto waterImage = Variables::Rules.GetString(ID, "WaterImage", ID);
			if (waterImage != ID)
			{
				LoadVehicleOrAircraft(waterImage);
			}
		}
		break;
	}
	case CLoadingExt::ObjectType::Aircraft:
		LoadVehicleOrAircraft(ID);
		break;
	case CLoadingExt::ObjectType::Building:
		LoadBuilding(ID);
		break;
	case CLoadingExt::ObjectType::Unknown:
	default:
		break;
	}
}

void CLoadingExt::ClearItemTypes()
{
	ObjectTypes.clear();
}

ppmfc::CString CLoadingExt::GetTerrainOrSmudgeFileID(ppmfc::CString ID)
{
	ppmfc::CString ArtID = GetArtID(ID);
	if (auto ppImage = Variables::Rules.TryGetString(ID, "Image")) {

		ArtID = *ppImage;
		ArtID.Trim();
	}
	else
		ArtID = ID;

	ppmfc::CString ImageID = CINI::Art->GetString(ArtID, "Image", ArtID);

	return ImageID;
}

ppmfc::CString CLoadingExt::GetBuildingFileID(ppmfc::CString ID)
{
	ppmfc::CString ArtID = GetArtID(ID);
	ppmfc::CString ImageID = CINI::Art->GetString(ArtID, "Image", ArtID);

	ppmfc::CString backupID = ImageID;
	SetTheaterLetter(ImageID, ExtConfigs::NewTheaterType);

	ppmfc::CString validator = ImageID + ".SHP";
	int nMix = this->SearchFile(validator);
	if (!CLoading::HasFile(validator, nMix))
	{
		SetGenericTheaterLetter(ImageID);
		if (!ExtConfigs::UseStrictNewTheater)
		{
			validator = ImageID + ".SHP";
			nMix = this->SearchFile(validator);
			if (!CLoading::HasFile(validator, nMix))
				ImageID = backupID;
		}
	}
	return ImageID;
}

ppmfc::CString CLoadingExt::GetInfantryFileID(ppmfc::CString ID)
{
	ppmfc::CString ArtID = GetArtID(ID);

	ppmfc::CString ImageID = ArtID;

	if (ExtConfigs::ArtImageSwap)
		ImageID = CINI::Art->GetString(ArtID, "Image", ArtID);

	if (Variables::Rules.GetBool(ID, "AlternateTheaterArt"))
		ImageID += this->TheaterIdentifier;
	else if (Variables::Rules.GetBool(ID, "AlternateArcticArt"))
		if (this->TheaterIdentifier == 'A')
			ImageID += 'A';
	if (!CINI::Art->SectionExists(ImageID))
		ImageID = ArtID;

	return ImageID;
}

ppmfc::CString CLoadingExt::GetArtID(ppmfc::CString ID)
{
	ppmfc::CString ArtID;
	if (auto ppImage = Variables::Rules.TryGetString(ID, "Image")) {
		ArtID = *ppImage;
		ArtID.Trim();
	}
	else
		ArtID = ID;

	return ArtID;
}

ppmfc::CString CLoadingExt::GetVehicleOrAircraftFileID(ppmfc::CString ID)
{
	ppmfc::CString ArtID = GetArtID(ID);

	ppmfc::CString ImageID = ArtID;

	if (ExtConfigs::ArtImageSwap)
		ImageID = CINI::Art->GetString(ArtID, "Image", ArtID);

	return ImageID;
}

void CLoadingExt::LoadBuilding(ppmfc::CString ID)
{
	if (auto ppPowerUpBld = Variables::Rules.TryGetString(ID, "PowersUpBuilding")) // Early load
	{
		ppmfc::CString PowerUpBld = *ppPowerUpBld;
		PowerUpBld.Trim();
		ppmfc::CString SrcBldName = GetBuildingFileID(PowerUpBld) + "0";
		if (!ImageDataMapHelper::IsImageLoaded(SrcBldName))
			LoadBuilding(PowerUpBld);
	}

	LoadBuilding_Normal(ID);
	LoadBuilding_Damaged(ID);
	LoadBuilding_Rubble(ID);

	if (ExtConfigs::InGameDisplay_AlphaImage)
	{
		if (auto pAIFile = Variables::Rules.TryGetString(ID, "AlphaImage"))
		{
			ppmfc::CString AIFile = *pAIFile;
			AIFile.Trim();
			if (!ImageDataMapHelper::IsImageLoaded(AIFile))
				LoadShp(AIFile + "\233ALPHAIMAGE", AIFile + ".shp", "anim.pal", 0);
		}
	}
}

void CLoadingExt::LoadBuilding_Normal(ppmfc::CString ID)
{
	ppmfc::CString ArtID = GetArtID(ID);
	ppmfc::CString ImageID = GetBuildingFileID(ID);

	ppmfc::CString PaletteName = CINI::Art->GetString(ArtID, "Palette", "unit");
	if (CINI::Art->GetBool(ArtID, "TerrainPalette"))
		PaletteName = "iso";
	GetFullPaletteName(PaletteName);
	auto palette = PalettesManager::LoadPalette(PaletteName);

	auto loadBuildingFrameShape = [&](ppmfc::CString name, int nFrame = 0, int deltaX = 0, int deltaY = 0, bool shadow = false) -> bool
	{
		ppmfc::CString file = name + ".SHP";
		int nMix = SearchFile(file);
		// building can be displayed without the body
		if (!CLoading::HasFile(file, nMix))
			return true;

		ShapeHeader header;
		unsigned char* pBuffer;
		CMixFile::LoadSHP(file, nMix);
		CShpFile::GetSHPHeader(&header);
		if (header.FrameCount / 2 <= nFrame) {
			nFrame = 0;
		}
		CLoadingExt::LoadSHPFrameSafe(nFrame, 1, &pBuffer, header);
		UnionSHP_Add(pBuffer, header.Width, header.Height, deltaX, deltaY);

		if (shadow && ExtConfigs::InGameDisplay_Shadow)
		{
			CLoadingExt::LoadSHPFrameSafe(nFrame + header.FrameCount / 2, 1, &pBuffer, header);
			UnionSHP_Add(pBuffer, header.Width, header.Height, deltaX, deltaY, false, true);
		}

		return true;
	};

	auto loadSingleFrameShape = [&](ppmfc::CString name, int nFrame = 0, int deltaX = 0, int deltaY = 0, ppmfc::CString customPal = "", bool shadow = false) -> bool
	{
		ppmfc::CString file = name + ".SHP";
		SetTheaterLetter(file, ExtConfigs::NewTheaterType);
		int nMix = SearchFile(file);
		if (!CLoading::HasFile(file, nMix))
		{
			SetGenericTheaterLetter(file);
			nMix = SearchFile(file);
			if (!CLoading::HasFile(file, nMix))
			{
				if (!ExtConfigs::UseStrictNewTheater)
				{
					file = name + ".SHP";
					nMix = SearchFile(file);
					if (!CLoading::HasFile(file, nMix))
						return false;
				}
				else
				{
					return false;
				}
			}
		}

		ShapeHeader header;
		unsigned char* pBuffer;
		CMixFile::LoadSHP(file, nMix);
		CShpFile::GetSHPHeader(&header);
		if (header.FrameCount <= nFrame) {
			nFrame = 0;
		}
		CLoadingExt::LoadSHPFrameSafe(nFrame, 1, &pBuffer, header);
		if (customPal != "")
		{
			if (auto thisPal = PalettesManager::LoadPalette(customPal))
			{
				std::vector<int> lookupTable = GeneratePalLookupTable(thisPal, palette);
				int counter = 0;
				for (int j = 0; j < header.Height; ++j)
				{
					for (int i = 0; i < header.Width; ++i)
					{
						unsigned char& ch = pBuffer[counter];
						ch = lookupTable[ch];
						counter++;
					}
				}
			}
		}

		UnionSHP_Add(pBuffer, header.Width, header.Height, deltaX, deltaY);

		if (shadow && ExtConfigs::InGameDisplay_Shadow)
		{
			CLoadingExt::LoadSHPFrameSafe(nFrame + header.FrameCount / 2, 1, &pBuffer, header);
			UnionSHP_Add(pBuffer, header.Width, header.Height, deltaX, deltaY, false, true);
		}

		return true;
	};

	auto loadAnimFrameShape = [&](ppmfc::CString animkey, ppmfc::CString ignorekey)
	{
		if (auto ppStr = CINI::Art->TryGetString(ArtID, animkey))
		{
			if (!CINI::FAData->GetBool(ignorekey, ID))
			{
				ppmfc::CString str = *ppStr;
				str.Trim();
				int nStartFrame = CINI::Art->GetInteger(str, "LoopStart");
				ppmfc::CString customPal = "";
				if (!CINI::Art->GetBool(str, "ShouldUseCellDrawer", true)) {
					customPal = CINI::Art->GetString(str, "CustomPalette", "anim.pal");
					customPal.Replace("~~~", GetTheaterSuffix());
				}
				loadSingleFrameShape(CINI::Art->GetString(str, "Image", str), nStartFrame, 0, 0, customPal, CINI::Art->GetBool(str, "Shadow"));
			}
		}
	};

	if (auto ppPowerUpBld = Variables::Rules.TryGetString(ID, "PowersUpBuilding")) // Early load
	{
		ppmfc::CString PowerUpBld = *ppPowerUpBld;
		PowerUpBld.Trim();
		ppmfc::CString SrcBldName = GetBuildingFileID(PowerUpBld) + "0";
		if (!ImageDataMapHelper::IsImageLoaded(SrcBldName))
			LoadBuilding(PowerUpBld);
	}

	int nBldStartFrame = CINI::Art->GetInteger(ArtID, "LoopStart", 0);
	if (loadBuildingFrameShape(ImageID, nBldStartFrame, 0, 0, CINI::Art->GetBool(ArtID, "Shadow", true)))
	{
		loadAnimFrameShape("IdleAnim", "IgnoreIdleAnim");
		loadAnimFrameShape("ActiveAnim", "IgnoreActiveAnim1");
		loadAnimFrameShape("ActiveAnimTwo", "IgnoreActiveAnim2");
		loadAnimFrameShape("ActiveAnimThree", "IgnoreActiveAnim3");
		loadAnimFrameShape("ActiveAnimFour", "IgnoreActiveAnim4");
		loadAnimFrameShape("SuperAnim", "IgnoreSuperAnim1");
		loadAnimFrameShape("SuperAnimTwo", "IgnoreSuperAnim2");
		loadAnimFrameShape("SuperAnimThree", "IgnoreSuperAnim3");
		loadAnimFrameShape("SuperAnimFour", "IgnoreSuperAnim4");

		if (auto ppStr = CINI::Art->TryGetString(ArtID, "BibShape")) {
			ppmfc::CString str = *ppStr;
			str.Trim();
			loadSingleFrameShape(CINI::Art->GetString(str, "Image", str));
		}

		ppmfc::CString DictName;

		unsigned char* pBuffer;
		int width, height;
		UnionSHP_GetAndClear(pBuffer, &width, &height);

		ppmfc::CString DictNameShadow;
		unsigned char* pBufferShadow{ 0 };
		int widthShadow, heightShadow;
		if (ExtConfigs::InGameDisplay_Shadow)
			UnionSHP_GetAndClear(pBufferShadow, &widthShadow, &heightShadow, false, true);

		if (Variables::Rules.GetBool(ID, "Turret")) // Has turret
		{
			if (Variables::Rules.GetBool(ID, "TurretAnimIsVoxel"))
			{
				ppmfc::CString TurName = Variables::Rules.GetString(ID, "TurretAnim", ID + "tur");
				ppmfc::CString BarlName = ID + "barl";


				if (!VoxelDrawer::IsVPLLoaded())
					VoxelDrawer::LoadVPLFile("voxels.vpl");

				std::vector<unsigned char*> pTurImages, pBarlImages;
				pTurImages.resize(ExtConfigs::MaxVoxelFacing, nullptr);
				pBarlImages.resize(ExtConfigs::MaxVoxelFacing, nullptr);
				std::vector<VoxelRectangle> turrect, barlrect;
				turrect.resize(ExtConfigs::MaxVoxelFacing);
				barlrect.resize(ExtConfigs::MaxVoxelFacing);

				ppmfc::CString VXLName = BarlName + ".vxl";
				ppmfc::CString HVAName = BarlName + ".hva";
				if (VoxelDrawer::LoadVXLFile(VXLName))
				{
					if (VoxelDrawer::LoadHVAFile(HVAName))
					{
						for (int i = 0; i < 8; ++i)
						{
							// (13 - i) % 8 for facing fix
							bool result = VoxelDrawer::GetImageData((13 - i) % 8, pBarlImages[i], barlrect[i]);
							if (!result)
								break;
						}
					}
				}

				VXLName = TurName + ".vxl";
				HVAName = TurName + ".hva";
				if (VoxelDrawer::LoadVXLFile(VXLName))
				{
					if (VoxelDrawer::LoadHVAFile(HVAName))
					{
						for (int i = 0; i < 8; ++i)
						{
							// (13 - i) % 8 for facing fix
							bool result = VoxelDrawer::GetImageData((13 - i) % 8, pTurImages[i], turrect[i]);
							if (!result)
								break;
						}
					}
				}

				for (int i = 0; i < 8; ++i)
				{
					auto pTempBuf = GameCreateArray<unsigned char>(width * height);
					memcpy_s(pTempBuf, width * height, pBuffer, width * height);
					UnionSHP_Add(pTempBuf, width, height);

					int deltaX = Variables::Rules.GetInteger(ID, "TurretAnimX", 0);
					int deltaY = Variables::Rules.GetInteger(ID, "TurretAnimY", 0);

					if (pTurImages[i])
					{
						ppmfc::CString pKey;

						pKey.Format("%sX%d", ID, (15 - i) % 8);
						int turdeltaX = CINI::FAData->GetInteger("BuildingVoxelTurretsRA2", pKey);
						pKey.Format("%sY%d", ID, (15 - i) % 8);
						int turdeltaY = CINI::FAData->GetInteger("BuildingVoxelTurretsRA2", pKey);

						VXL_Add(pTurImages[i], turrect[i].X + turdeltaX, turrect[i].Y + turdeltaY, turrect[i].W, turrect[i].H);
						CncImgFree(pTurImages[i]);

						if (pBarlImages[i])
						{
							pKey.Format("%sX%d", ID, (15 - i) % 8);
							int barldeltaX = CINI::FAData->GetInteger("BuildingVoxelBarrelsRA2", pKey);
							pKey.Format("%sY%d", ID, (15 - i) % 8);
							int barldeltaY = CINI::FAData->GetInteger("BuildingVoxelBarrelsRA2", pKey);

							VXL_Add(pBarlImages[i], barlrect[i].X + barldeltaX, barlrect[i].Y + barldeltaY, barlrect[i].W, barlrect[i].H);
							CncImgFree(pBarlImages[i]);
						}
					}

					int nW = 0x100, nH = 0x100;
					VXL_GetAndClear(pTurImages[i], &nW, &nH);

					UnionSHP_Add(pTurImages[i], 0x100, 0x100, deltaX, deltaY);

					unsigned char* pImage;
					int width1, height1;

					UnionSHP_GetAndClear(pImage, &width1, &height1);
					DictName.Format("%s%d", ID, i);
					SetImageData(pImage, DictName, width1, height1, palette);
				}

				if (ExtConfigs::InGameDisplay_Shadow)
				{
					DictNameShadow.Format("%s%d\233SHADOW", ID, 0);
					SetImageData(pBufferShadow, DictNameShadow, widthShadow, heightShadow, &CMapDataExt::Palette_Shadow);
				}

				GameDeleteArray(pBuffer, width * height);
			}
			else //SHP anim
			{
				ppmfc::CString TurName = Variables::Rules.GetString(ID, "TurretAnim", ID + "tur");
				int nStartFrame = CINI::Art->GetInteger(TurName, "LoopStart");
				bool shadow = CINI::Art->GetBool(TurName, "Shadow", true) && ExtConfigs::InGameDisplay_Shadow;
				for (int i = 0; i < 8; ++i)
				{
					auto pTempBuf = GameCreateArray<unsigned char>(width * height);
					memcpy_s(pTempBuf, width * height, pBuffer, width * height);
					UnionSHP_Add(pTempBuf, width, height);

					if (shadow)
					{
						auto pTempBufShadow = GameCreateArray<unsigned char>(width * height);
						memcpy_s(pTempBufShadow, width * height, pBufferShadow, width * height);
						UnionSHP_Add(pTempBufShadow, width, height, 0, 0, false, true);
					}

					int deltaX = Variables::Rules.GetInteger(ID, "TurretAnimX", 0);
					int deltaY = Variables::Rules.GetInteger(ID, "TurretAnimY", 0);
					loadSingleFrameShape(CINI::Art->GetString(TurName, "Image", TurName),
						nStartFrame + i * 4, deltaX, deltaY, "", shadow);

					unsigned char* pImage;
					int width1, height1;
					UnionSHP_GetAndClear(pImage, &width1, &height1);

					DictName.Format("%s%d", ID, i);
					SetImageData(pImage, DictName, width1, height1, palette);

					if (shadow)
					{
						ppmfc::CString DictNameShadow;
						unsigned char* pImageShadow;
						int width1Shadow, height1Shadow;
						UnionSHP_GetAndClear(pImageShadow, &width1Shadow, &height1Shadow, false, true);
						DictNameShadow.Format("%s%d\233SHADOW", ID, i);
						SetImageData(pImageShadow, DictNameShadow, width1Shadow, height1Shadow, &CMapDataExt::Palette_Shadow);
					}
				}
				GameDelete(pBuffer);
				GameDelete(pBufferShadow);
			}
		}
		else // No turret
		{
			DictName.Format("%s%d", ID, 0);
			SetImageData(pBuffer, DictName, width, height, palette);
			if (ExtConfigs::InGameDisplay_Shadow)
			{
				DictNameShadow.Format("%s%d\233SHADOW", ID, 0);
				SetImageData(pBufferShadow, DictNameShadow, widthShadow, heightShadow, &CMapDataExt::Palette_Shadow);
			}
		}
	}
}

void CLoadingExt::LoadBuilding_Damaged(ppmfc::CString ID, bool loadAsRubble)
{
	ppmfc::CString ArtID = GetArtID(ID);
	ppmfc::CString ImageID = GetBuildingFileID(ID);

	ppmfc::CString PaletteName = CINI::Art->GetString(ArtID, "Palette", "unit");
	if (CINI::Art->GetBool(ArtID, "TerrainPalette"))
		PaletteName = "iso";
	GetFullPaletteName(PaletteName);
	auto palette = PalettesManager::LoadPalette(PaletteName);

	auto loadBuildingFrameShape = [&](ppmfc::CString name, int nFrame = 0, int deltaX = 0, int deltaY = 0, bool shadow = false) -> bool
	{
		ppmfc::CString file = name + ".SHP";
		int nMix = SearchFile(file);
		// building can be displayed without the body
		if (!CLoading::HasFile(file, nMix))
			return true;

		ShapeHeader header;
		unsigned char* pBuffer;
		CMixFile::LoadSHP(file, nMix);
		CShpFile::GetSHPHeader(&header);
		if (header.FrameCount / 2 <= nFrame) {
			nFrame = 0;
		}
		CLoadingExt::LoadSHPFrameSafe(nFrame, 1, &pBuffer, header);

		UnionSHP_Add(pBuffer, header.Width, header.Height, deltaX, deltaY);

		if (shadow && ExtConfigs::InGameDisplay_Shadow)
		{
			CLoadingExt::LoadSHPFrameSafe(nFrame + header.FrameCount / 2, 1, &pBuffer, header);
			UnionSHP_Add(pBuffer, header.Width, header.Height, deltaX, deltaY, false, true);
		}

		return true;
	};

	auto loadSingleFrameShape = [&](ppmfc::CString name, int nFrame = 0, int deltaX = 0, int deltaY = 0, ppmfc::CString customPal = "", bool shadow = false) -> bool
	{
		ppmfc::CString file = name + ".SHP";
		SetTheaterLetter(file, ExtConfigs::NewTheaterType);
		int nMix = SearchFile(file);
		if (!CLoading::HasFile(file, nMix))
		{
			SetGenericTheaterLetter(file);
			nMix = SearchFile(file);
			if (!CLoading::HasFile(file, nMix))
			{
				if (!ExtConfigs::UseStrictNewTheater)
				{
					file = name + ".SHP";
					nMix = SearchFile(file);
					if (!CLoading::HasFile(file, nMix))
						return false;
				}
				else
				{
					return false;
				}
			}
		}

		ShapeHeader header;
		unsigned char* pBuffer;
		CMixFile::LoadSHP(file, nMix);
		CShpFile::GetSHPHeader(&header);
		if (header.FrameCount <= nFrame) {
			nFrame = 0;
		}
		CLoadingExt::LoadSHPFrameSafe(nFrame, 1, &pBuffer, header);
		if (customPal != "")
		{
			if (auto thisPal = PalettesManager::LoadPalette(customPal))
			{
				std::vector<int> lookupTable = GeneratePalLookupTable(thisPal, palette);
				int counter = 0;
				for (int j = 0; j < header.Height; ++j)
				{
					for (int i = 0; i < header.Width; ++i)
					{
						unsigned char& ch = pBuffer[counter];
						ch = lookupTable[ch];
						counter++;
					}
				}
			}
		}

		UnionSHP_Add(pBuffer, header.Width, header.Height, deltaX, deltaY);

		if (shadow && ExtConfigs::InGameDisplay_Shadow)
		{
			CLoadingExt::LoadSHPFrameSafe(nFrame + header.FrameCount / 2, 1, &pBuffer, header);
			UnionSHP_Add(pBuffer, header.Width, header.Height, deltaX, deltaY, false, true);
		}

		return true;
	};

	auto loadAnimFrameShape = [&](ppmfc::CString animkey, ppmfc::CString ignorekey)
	{
		ppmfc::CString damagedAnimkey = animkey + "Damaged";
		if (auto ppStr = CINI::Art->TryGetString(ArtID, damagedAnimkey))
		{
			ppmfc::CString str = *ppStr;
			str.Trim();
			if (!CINI::FAData->GetBool(ignorekey, ID))
			{
				int nStartFrame = CINI::Art->GetInteger(str, "LoopStart");
				ppmfc::CString customPal = "";
				if (!CINI::Art->GetBool(str, "ShouldUseCellDrawer", true)) {
					customPal = CINI::Art->GetString(str, "CustomPalette", "anim.pal");
					customPal.Replace("~~~", GetTheaterSuffix());
				}
				loadSingleFrameShape(CINI::Art->GetString(str, "Image", str), nStartFrame, 0, 0, customPal, CINI::Art->GetBool(str, "Shadow"));
			}
		}
		else if (auto ppStr = CINI::Art->TryGetString(ArtID, animkey))
		{
			ppmfc::CString str = *ppStr;
			str.Trim();
			if (!CINI::FAData->GetBool(ignorekey, ID))
			{
				int nStartFrame = CINI::Art->GetInteger(str, "LoopStart");
				ppmfc::CString customPal = "";
				if (!CINI::Art->GetBool(str, "ShouldUseCellDrawer", true)) {
					customPal = CINI::Art->GetString(str, "CustomPalette", "anim.pal");
					customPal.Replace("~~~", GetTheaterSuffix());
				}
				loadSingleFrameShape(CINI::Art->GetString(str, "Image", str), nStartFrame, 0, 0, customPal);
			}
		}
	};

	int nBldStartFrame = CINI::Art->GetInteger(ArtID, "LoopStart", 0) + 1;
	if (loadBuildingFrameShape(ImageID, nBldStartFrame, 0, 0, CINI::Art->GetBool(ArtID, "Shadow", true)))
	{
		loadAnimFrameShape("IdleAnim", "IgnoreIdleAnim");
		loadAnimFrameShape("ActiveAnim", "IgnoreActiveAnim1");
		loadAnimFrameShape("ActiveAnimTwo", "IgnoreActiveAnim2");
		loadAnimFrameShape("ActiveAnimThree", "IgnoreActiveAnim3");
		loadAnimFrameShape("ActiveAnimFour", "IgnoreActiveAnim4");
		loadAnimFrameShape("SuperAnim", "IgnoreSuperAnim1");
		loadAnimFrameShape("SuperAnimTwo", "IgnoreSuperAnim2");
		loadAnimFrameShape("SuperAnimThree", "IgnoreSuperAnim3");
		loadAnimFrameShape("SuperAnimFour", "IgnoreSuperAnim4");

		if (auto ppStr = CINI::Art->TryGetString(ArtID, "BibShape")) {
			ppmfc::CString str = *ppStr;
			str.Trim();
			loadSingleFrameShape(CINI::Art->GetString(str, "Image", str), 1);
		}

		ppmfc::CString DictName;

		unsigned char* pBuffer;
		int width, height;
		UnionSHP_GetAndClear(pBuffer, &width, &height);

		ppmfc::CString DictNameShadow;
		unsigned char* pBufferShadow{ 0 };
		int widthShadow, heightShadow;
		if (ExtConfigs::InGameDisplay_Shadow)
			UnionSHP_GetAndClear(pBufferShadow, &widthShadow, &heightShadow, false, true);

		if (Variables::Rules.GetBool(ID, "Turret")) // Has turret
		{
			if (Variables::Rules.GetBool(ID, "TurretAnimIsVoxel"))
			{
				ppmfc::CString TurName = Variables::Rules.GetString(ID, "TurretAnim", ID + "tur");
				ppmfc::CString BarlName = ID + "barl";


				if (!VoxelDrawer::IsVPLLoaded())
					VoxelDrawer::LoadVPLFile("voxels.vpl");

				std::vector<unsigned char*> pTurImages, pBarlImages;
				pTurImages.resize(ExtConfigs::MaxVoxelFacing, nullptr);
				pBarlImages.resize(ExtConfigs::MaxVoxelFacing, nullptr);
				std::vector<VoxelRectangle> turrect, barlrect;
				turrect.resize(ExtConfigs::MaxVoxelFacing);
				barlrect.resize(ExtConfigs::MaxVoxelFacing);

				ppmfc::CString VXLName = BarlName + ".vxl";
				ppmfc::CString HVAName = BarlName + ".hva";
				if (VoxelDrawer::LoadVXLFile(VXLName))
				{
					if (VoxelDrawer::LoadHVAFile(HVAName))
					{
						for (int i = 0; i < 8; ++i)
						{
							// (13 - i) % 8 for facing fix
							bool result = VoxelDrawer::GetImageData((13 - i) % 8, pBarlImages[i], barlrect[i]);
							if (!result)
								break;
						}
					}
				}

				VXLName = TurName + ".vxl";
				HVAName = TurName + ".hva";
				if (VoxelDrawer::LoadVXLFile(VXLName))
				{
					if (VoxelDrawer::LoadHVAFile(HVAName))
					{
						for (int i = 0; i < 8; ++i)
						{
							// (13 - i) % 8 for facing fix
							bool result = VoxelDrawer::GetImageData((13 - i) % 8, pTurImages[i], turrect[i]);
							if (!result)
								break;
						}
					}
				}

				for (int i = 0; i < 8; ++i)
				{
					auto pTempBuf = GameCreateArray<unsigned char>(width * height);
					memcpy_s(pTempBuf, width * height, pBuffer, width * height);
					UnionSHP_Add(pTempBuf, width, height);

					int deltaX = Variables::Rules.GetInteger(ID, "TurretAnimX", 0);
					int deltaY = Variables::Rules.GetInteger(ID, "TurretAnimY", 0);

					if (pTurImages[i])
					{
						ppmfc::CString pKey;

						pKey.Format("%sX%d", ID, (15 - i) % 8);
						int turdeltaX = CINI::FAData->GetInteger("BuildingVoxelTurretsRA2", pKey);
						pKey.Format("%sY%d", ID, (15 - i) % 8);
						int turdeltaY = CINI::FAData->GetInteger("BuildingVoxelTurretsRA2", pKey);

						VXL_Add(pTurImages[i], turrect[i].X + turdeltaX, turrect[i].Y + turdeltaY, turrect[i].W, turrect[i].H);
						CncImgFree(pTurImages[i]);

						if (pBarlImages[i])
						{
							pKey.Format("%sX%d", ID, (15 - i) % 8);
							int barldeltaX = CINI::FAData->GetInteger("BuildingVoxelBarrelsRA2", pKey);
							pKey.Format("%sY%d", ID, (15 - i) % 8);
							int barldeltaY = CINI::FAData->GetInteger("BuildingVoxelBarrelsRA2", pKey);

							VXL_Add(pBarlImages[i], barlrect[i].X + barldeltaX, barlrect[i].Y + barldeltaY, barlrect[i].W, barlrect[i].H);
							CncImgFree(pBarlImages[i]);
						}
					}

					int nW = 0x100, nH = 0x100;
					VXL_GetAndClear(pTurImages[i], &nW, &nH);

					UnionSHP_Add(pTurImages[i], 0x100, 0x100, deltaX, deltaY);

					unsigned char* pImage;
					int width1, height1;

					UnionSHP_GetAndClear(pImage, &width1, &height1);
					if (loadAsRubble)
						DictName.Format("%s%d\233RUBBLE", ID, i);
					else
						DictName.Format("%s%d\233DAMAGED", ID, i);
					SetImageData(pImage, DictName, width1, height1, palette);
				}

				if (ExtConfigs::InGameDisplay_Shadow)
				{
					if (loadAsRubble)
						DictNameShadow.Format("%s%d\233RUBBLESHADOW", ID, 0);
					else
						DictNameShadow.Format("%s%d\233DAMAGEDSHADOW", ID, 0);
					SetImageData(pBufferShadow, DictNameShadow, widthShadow, heightShadow, &CMapDataExt::Palette_Shadow);
				}

				GameDeleteArray(pBuffer, width * height);
			}
			else //SHP anim
			{
				ppmfc::CString TurName = Variables::Rules.GetString(ID, "TurretAnim", ID + "tur");
				int nStartFrame = CINI::Art->GetInteger(TurName, "LoopStart");
				bool shadow = CINI::Art->GetBool(TurName, "Shadow", true) && ExtConfigs::InGameDisplay_Shadow;
				for (int i = 0; i < 8; ++i)
				{
					auto pTempBuf = GameCreateArray<unsigned char>(width * height);
					memcpy_s(pTempBuf, width * height, pBuffer, width * height);
					UnionSHP_Add(pTempBuf, width, height);

					if (shadow)
					{
						auto pTempBufShadow = GameCreateArray<unsigned char>(width * height);
						memcpy_s(pTempBufShadow, width * height, pBufferShadow, width * height);
						UnionSHP_Add(pTempBufShadow, width, height, 0, 0, false, true);
					}

					int deltaX = Variables::Rules.GetInteger(ID, "TurretAnimX", 0);
					int deltaY = Variables::Rules.GetInteger(ID, "TurretAnimY", 0);
					loadSingleFrameShape(CINI::Art->GetString(TurName, "Image", TurName),
						nStartFrame + i * 4, deltaX, deltaY, "", shadow);

					unsigned char* pImage;
					int width1, height1;
					UnionSHP_GetAndClear(pImage, &width1, &height1);

					if (loadAsRubble)
						DictName.Format("%s%d\233RUBBLE", ID, i);
					else
						DictName.Format("%s%d\233DAMAGED", ID, i);
					SetImageData(pImage, DictName, width1, height1, palette);

					if (shadow)
					{
						ppmfc::CString DictNameShadow;
						unsigned char* pImageShadow;
						int width1Shadow, height1Shadow;
						UnionSHP_GetAndClear(pImageShadow, &width1Shadow, &height1Shadow, false, true);
						if (loadAsRubble)
							DictNameShadow.Format("%s%d\233RUBBLESHADOW", ID, i);
						else
							DictNameShadow.Format("%s%d\233DAMAGEDSHADOW", ID, i);
						SetImageData(pImageShadow, DictNameShadow, width1Shadow, height1Shadow, &CMapDataExt::Palette_Shadow);
					}
				}
				GameDelete(pBuffer);
				GameDelete(pBufferShadow);
			}
		}
		else // No turret
		{
			if (loadAsRubble)
				DictName.Format("%s%d\233RUBBLE", ID, 0);
			else
				DictName.Format("%s%d\233DAMAGED", ID, 0);
			SetImageData(pBuffer, DictName, width, height, palette);

			if (ExtConfigs::InGameDisplay_Shadow)
			{
				if (loadAsRubble)
					DictNameShadow.Format("%s%d\233RUBBLESHADOW", ID, 0);
				else
					DictNameShadow.Format("%s%d\233DAMAGEDSHADOW", ID, 0);
				SetImageData(pBufferShadow, DictNameShadow, widthShadow, heightShadow, &CMapDataExt::Palette_Shadow);
			}
		}
	}
}

void CLoadingExt::LoadBuilding_Rubble(ppmfc::CString ID)
{
	ppmfc::CString ArtID = GetArtID(ID);
	ppmfc::CString ImageID = GetBuildingFileID(ID);
	ppmfc::CString PaletteName = "iso";
	GetFullPaletteName(PaletteName);
	auto pal = PalettesManager::LoadPalette(PaletteName);

	auto loadBuildingFrameShape = [&](ppmfc::CString name, int nFrame = 0, int deltaX = 0, int deltaY = 0, bool shadow = false) -> bool
	{
		ppmfc::CString file = name + ".SHP";
		int nMix = SearchFile(file);
		// building can be displayed without the body
		if (!CLoading::HasFile(file, nMix))
			return true;

		ShapeHeader header;
		unsigned char* pBuffer;
		CMixFile::LoadSHP(file, nMix);
		CShpFile::GetSHPHeader(&header);
		if (header.FrameCount / 2 <= nFrame) {
			return false;
		}
		CLoadingExt::LoadSHPFrameSafe(nFrame, 1, &pBuffer, header);

		UnionSHP_Add(pBuffer, header.Width, header.Height, deltaX, deltaY);

		if (shadow && ExtConfigs::InGameDisplay_Shadow)
		{
			CLoadingExt::LoadSHPFrameSafe(nFrame + header.FrameCount / 2, 1, &pBuffer, header);
			UnionSHP_Add(pBuffer, header.Width, header.Height, deltaX, deltaY, false, true);
		}

		return true;
	};

	auto loadSingleFrameShape = [&](ppmfc::CString name, int nFrame = 0, int deltaX = 0, int deltaY = 0, bool shadow = false) -> bool
	{
		ppmfc::CString file = name + ".SHP";
		SetTheaterLetter(file, ExtConfigs::NewTheaterType);
		int nMix = SearchFile(file);
		if (!CLoading::HasFile(file, nMix))
		{
			SetGenericTheaterLetter(file);
			nMix = SearchFile(file);
			if (!CLoading::HasFile(file, nMix))
			{
				if (!ExtConfigs::UseStrictNewTheater)
				{
					file = name + ".SHP";
					nMix = SearchFile(file);
					if (!CLoading::HasFile(file, nMix))
						return false;
				}
				else
				{
					return false;
				}
			}
		}

		ShapeHeader header;
		unsigned char* pBuffer;
		CMixFile::LoadSHP(file, nMix);
		CShpFile::GetSHPHeader(&header);
		if (header.FrameCount <= nFrame) {
			nFrame = 0;
		}
		CLoadingExt::LoadSHPFrameSafe(nFrame, 1, &pBuffer, header);

		UnionSHP_Add(pBuffer, header.Width, header.Height, deltaX, deltaY);

		if (shadow && ExtConfigs::InGameDisplay_Shadow)
		{
			CLoadingExt::LoadSHPFrameSafe(nFrame + header.FrameCount / 2, 1, &pBuffer, header);
			UnionSHP_Add(pBuffer, header.Width, header.Height, deltaX, deltaY, false, true);
		}

		return true;
	};

	auto loadAnimFrameShape = [&](ppmfc::CString animkey, ppmfc::CString ignorekey)
	{
		ppmfc::CString damagedAnimkey = animkey + "Damaged";
		if (auto ppStr = CINI::Art->TryGetString(ArtID, damagedAnimkey))
		{
			ppmfc::CString str = *ppStr;
			str.Trim();
			if (!CINI::FAData->GetBool(ignorekey, ID))
			{
				int nStartFrame = CINI::Art->GetInteger(str, "LoopStart");
				loadSingleFrameShape(CINI::Art->GetString(str, "Image", str), nStartFrame);
			}
		}
		else if (auto ppStr = CINI::Art->TryGetString(ArtID, animkey))
		{
			ppmfc::CString str = *ppStr;
			str.Trim();
			if (!CINI::FAData->GetBool(ignorekey, ID))
			{
				int nStartFrame = CINI::Art->GetInteger(str, "LoopStart");
				loadSingleFrameShape(CINI::Art->GetString(str, "Image", str), nStartFrame);
			}
		}
	};

	if (Variables::Rules.GetBool(ID, "LeaveRubble"))
	{
		int nBldStartFrame = CINI::Art->GetInteger(ArtID, "LoopStart", 0) + 3;
		if (loadBuildingFrameShape(ImageID, nBldStartFrame, 0, 0, CINI::Art->GetBool(ArtID, "Shadow", true)))
		{
			unsigned char* pBuffer;
			int width, height;
			UnionSHP_GetAndClear(pBuffer, &width, &height);

			ppmfc::CString DictName = ID + "0\233RUBBLE";
			SetImageData(pBuffer, DictName, width, height, pal);

			if (ExtConfigs::InGameDisplay_Shadow)
			{
				ppmfc::CString DictNameShadow;
				unsigned char* pBufferShadow{ 0 };
				int widthShadow, heightShadow;
				UnionSHP_GetAndClear(pBufferShadow, &widthShadow, &heightShadow, false, true);
				DictNameShadow.Format("%s%d\233RUBBLESHADOW", ID, 0);
				SetImageData(pBufferShadow, DictNameShadow, widthShadow, heightShadow, &CMapDataExt::Palette_Shadow);
			}
		}
		else
		{
			LoadBuilding_Damaged(ID, true);
		}
	}
}

void CLoadingExt::LoadInfantry(ppmfc::CString ID)
{	
	ppmfc::CString ArtID = GetArtID(ID);
	ppmfc::CString ImageID = GetInfantryFileID(ID);

	ppmfc::CString sequenceName = CINI::Art->GetString(ImageID, "Sequence");
	bool deployable = Variables::Rules.GetBool(ID, "Deployer") && CINI::Art->KeyExists(sequenceName, "Deployed");
	bool waterable = Variables::Rules.GetString(ID, "MovementZone") == "AmphibiousDestroyer" 
		&& CINI::Art->KeyExists(sequenceName, "Swim");
	ppmfc::CString frames = CINI::Art->GetString(sequenceName, "Guard", "0,1,1");
	int framesToRead[8];
	int frameStart, frameStep;
	sscanf_s(frames, "%d,%d,%d", &frameStart, &framesToRead[0], &frameStep);
	for (int i = 0; i < 8; ++i)
		framesToRead[i] = frameStart + i * frameStep;

	ppmfc::CString PaletteName = CINI::Art->GetString(ArtID, "Palette", "unit");
	GetFullPaletteName(PaletteName);
	auto pal = PalettesManager::LoadPalette(PaletteName);
	
	ppmfc::CString FileName = ImageID + ".shp";
	int nMix = this->SearchFile(FileName);
	if (CLoading::HasFile(FileName, nMix))
	{
		ShapeHeader header;
		unsigned char* FramesBuffers;
		CMixFile::LoadSHP(FileName, nMix);
		CShpFile::GetSHPHeader(&header);
		for (int i = 0; i < 8; ++i)
		{
			CLoadingExt::LoadSHPFrameSafe(framesToRead[i], 1, &FramesBuffers, header);
			ppmfc::CString DictName;
			DictName.Format("%s%d", ID, i);
			// DictName.Format("%s%d", ImageID, i);
			SetImageData(FramesBuffers, DictName, header.Width, header.Height, pal);

			if (ExtConfigs::InGameDisplay_Shadow)
			{
				ppmfc::CString DictNameShadow;
				unsigned char* pBufferShadow{ 0 };
				DictNameShadow.Format("%s%d\233SHADOW", ID, i);
				CLoadingExt::LoadSHPFrameSafe(framesToRead[i] + header.FrameCount / 2, 1, &pBufferShadow, header);
				SetImageData(pBufferShadow, DictNameShadow, header.Width, header.Height, &CMapDataExt::Palette_Shadow);
			}
		}

		if (ExtConfigs::InGameDisplay_Deploy && deployable)
		{
			ppmfc::CString framesDeploy = CINI::Art->GetString(sequenceName, "Deployed", "0,1,1");
			int framesToReadDeploy[8];
			int frameStartDeploy, frameStepDeploy;
			sscanf_s(framesDeploy, "%d,%d,%d", &frameStartDeploy, &framesToReadDeploy[0], &frameStepDeploy);
			for (int i = 0; i < 8; ++i)
				framesToReadDeploy[i] = frameStartDeploy + i * frameStepDeploy;
			unsigned char* FramesBuffersDeploy;
			for (int i = 0; i < 8; ++i)
			{
				CLoadingExt::LoadSHPFrameSafe(framesToReadDeploy[i], 1, &FramesBuffersDeploy, header);
				ppmfc::CString DictNameDeploy;
				DictNameDeploy.Format("%s%d\233DEPLOY", ID, i);
				SetImageData(FramesBuffersDeploy, DictNameDeploy, header.Width, header.Height, pal);

				if (ExtConfigs::InGameDisplay_Shadow)
				{
					ppmfc::CString DictNameShadow;
					unsigned char* pBufferShadow{ 0 };
					DictNameShadow.Format("%s%d\233DEPLOYSHADOW", ID, i);
					CLoadingExt::LoadSHPFrameSafe(framesToReadDeploy[i] + header.FrameCount / 2, 1, &pBufferShadow, header);
					SetImageData(pBufferShadow, DictNameShadow, header.Width, header.Height, &CMapDataExt::Palette_Shadow);
				}
			}
		}
		
		if (ExtConfigs::InGameDisplay_Water && waterable)
		{
			ppmfc::CString framesWater = CINI::Art->GetString(sequenceName, "Swim", "0,1,1");
			int framesToReadWater[8];
			int frameStartWater, frameStepWater;
			sscanf_s(framesWater, "%d,%d,%d", &frameStartWater, &framesToReadWater[0], &frameStepWater);
			for (int i = 0; i < 8; ++i)
				framesToReadWater[i] = frameStartWater + i * frameStepWater;
			unsigned char* FramesBuffersWater;
			for (int i = 0; i < 8; ++i)
			{
				CLoadingExt::LoadSHPFrameSafe(framesToReadWater[i], 1, &FramesBuffersWater, header);
				ppmfc::CString DictNameWater;
				DictNameWater.Format("%s%d\233WATER", ID, i);
				SetImageData(FramesBuffersWater, DictNameWater, header.Width, header.Height, pal);

				if (ExtConfigs::InGameDisplay_Shadow)
				{
					ppmfc::CString DictNameShadow;
					unsigned char* pBufferShadow{ 0 };
					DictNameShadow.Format("%s%d\233WATERSHADOW", ID, i);
					CLoadingExt::LoadSHPFrameSafe(framesToReadWater[i] + header.FrameCount / 2, 1, &pBufferShadow, header);
					SetImageData(pBufferShadow, DictNameShadow, header.Width, header.Height, &CMapDataExt::Palette_Shadow);
					SwimableInfantries.push_back(ID);
				}
			}
		}

		//if (auto pAIFile = Variables::Rules.TryGetString(ID, "AlphaImage"))
		//{
		//	ppmfc::CString AIFile = *pAIFile;
		//	AIFile.Trim();
		//	if (!ImageDataMapHelper::IsImageLoaded(AIFile))
		//		LoadShp(AIFile + "\233ALPHAIMAGE", AIFile + ".shp", "anim.pal", 0);
		//}
	}
}

void CLoadingExt::LoadTerrainOrSmudge(ppmfc::CString ID, bool terrain)
{
	ppmfc::CString ArtID = GetArtID(ID);
	ppmfc::CString ImageID = GetTerrainOrSmudgeFileID(ID);
	ppmfc::CString FileName = ImageID + this->GetFileExtension();
	int nMix = this->SearchFile(FileName);
	if (CLoading::HasFile(FileName, nMix))
	{
		ShapeHeader header;
		unsigned char* FramesBuffers[1];
		CMixFile::LoadSHP(FileName, nMix);
		CShpFile::GetSHPHeader(&header);
		CLoadingExt::LoadSHPFrameSafe(0, 1, &FramesBuffers[0], header);
		ppmfc::CString DictName;
		DictName.Format("%s%d", ID, 0);
		// use uppercase to distinguish them between real terrain
		ppmfc::CString PaletteName = CINI::Art->GetString(ArtID, "Palette", "iso");
		if (!CINI::Art->KeyExists(ArtID, "Palette") && Variables::Rules.GetBool(ID, "SpawnsTiberium"))
		{
			PaletteName = "unitsno.pal";
		}
		PaletteName.MakeUpper();
		GetFullPaletteName(PaletteName);
		SetImageData(FramesBuffers[0], DictName, header.Width, header.Height, PalettesManager::LoadPalette(PaletteName));

		if (ExtConfigs::InGameDisplay_Shadow && terrain)
		{
			ppmfc::CString DictNameShadow;
			unsigned char* pBufferShadow[1];
			DictNameShadow.Format("%s%d\233SHADOW", ID, 0);
			CLoadingExt::LoadSHPFrameSafe(0 + header.FrameCount / 2, 1, &pBufferShadow[0], header);
			SetImageData(pBufferShadow[0], DictNameShadow, header.Width, header.Height, &CMapDataExt::Palette_Shadow);
		}

		if (ExtConfigs::InGameDisplay_AlphaImage && terrain)
		{
			if (auto pAIFile = Variables::Rules.TryGetString(ID, "AlphaImage"))
			{
				ppmfc::CString AIFile = *pAIFile;
				AIFile.Trim();
				if (!ImageDataMapHelper::IsImageLoaded(AIFile))
					LoadShp(AIFile + "\233ALPHAIMAGE", AIFile + ".shp", "anim.pal", 0);
			}
		}
	}
}

void CLoadingExt::LoadVehicleOrAircraft(ppmfc::CString ID)
{
	ppmfc::CString ArtID = GetArtID(ID);
	ppmfc::CString ImageID = GetVehicleOrAircraftFileID(ID);
	bool bHasTurret = Variables::Rules.GetBool(ID, "Turret");
	bool bHasShadow = !Variables::Rules.GetBool(ID, "Underwater");

	if (CINI::Art->GetBool(ArtID, "Voxel")) // As VXL
	{
		ppmfc::CString FileName = ImageID + ".vxl";
		ppmfc::CString HVAName = ImageID + ".hva";

		if (!VoxelDrawer::IsVPLLoaded())
			VoxelDrawer::LoadVPLFile("voxels.vpl");

		ppmfc::CString PaletteName = CINI::Art->GetString(ArtID, "Palette", "unit");
		GetFullPaletteName(PaletteName);

		std::vector<unsigned char*> pImage, pTurretImage, pBarrelImage, pShadowImage;
		pImage.resize(ExtConfigs::MaxVoxelFacing, nullptr);
		pTurretImage.resize(ExtConfigs::MaxVoxelFacing, nullptr);
		pBarrelImage.resize(ExtConfigs::MaxVoxelFacing, nullptr);
		if (ExtConfigs::InGameDisplay_Shadow)
			pShadowImage.resize(ExtConfigs::MaxVoxelFacing, nullptr);
		std::vector<VoxelRectangle> rect, turretrect, barrelrect, shadowrect;
		rect.resize(ExtConfigs::MaxVoxelFacing);
		turretrect.resize(ExtConfigs::MaxVoxelFacing);
		barrelrect.resize(ExtConfigs::MaxVoxelFacing);
		if (ExtConfigs::InGameDisplay_Shadow)
			shadowrect.resize(ExtConfigs::MaxVoxelFacing);

		if (VoxelDrawer::LoadVXLFile(FileName))
		{
			if (VoxelDrawer::LoadHVAFile(HVAName))
			{
				for (int i = 0; i < 8; ++i)
				{
					// (i+6) % 8 to fix the facing
					bool result = false;
					if (ExtConfigs::InGameDisplay_Shadow)
					{
						result = VoxelDrawer::GetImageData((i + 6) % 8, pImage[i], rect[i])
							&& VoxelDrawer::GetImageData((i + 6) % 8, pShadowImage[i], shadowrect[i], 0, 0, 0, true);
					}
					else
					{
						result = VoxelDrawer::GetImageData((i + 6) % 8, pImage[i], rect[i]);
					}
					if (!result)
						return;
				}
			}
		}

		if (bHasTurret)
		{
			int F, L, H;
			int s_count = sscanf_s(CINI::Art->GetString(ArtID, "TurretOffset", "0,0,0"), "%d,%d,%d", &F, &L, &H);
			if (s_count == 0) F = L = H = 0;
			else if (s_count == 1) L = H = 0;
			else if (s_count == 2) H = 0;

			ppmfc::CString turFileName = ImageID + "tur.vxl";
			ppmfc::CString turHVAName = ImageID + "tur.hva";
			if (VoxelDrawer::LoadVXLFile(turFileName))
			{
				if (VoxelDrawer::LoadHVAFile(turHVAName))
				{
					for (int i = 0; i < 8; ++i)
					{
						// (i+6) % 8 to fix the facing
						bool result = VoxelDrawer::GetImageData((i + 6) % 8, pTurretImage[i], turretrect[i], F, L, H);
						if (!result)
							break;
					}
				}
			}

			ppmfc::CString barlFileName = ImageID + "barl.vxl";
			ppmfc::CString barlHVAName = ImageID + "barl.hva";
			if (VoxelDrawer::LoadVXLFile(barlFileName))
			{
				if (VoxelDrawer::LoadHVAFile(barlHVAName))
				{
					for (int i = 0; i < 8; ++i)
					{
						// (i+6) % 8 to fix the facing
						bool result = VoxelDrawer::GetImageData((i + 6) % 8, pBarrelImage[i], barrelrect[i], F, L, H);
						if (!result)
							break;
					}
				}
			}

			for (int i = 0; i < 8; ++i)
			{
				ppmfc::CString DictName;
				DictName.Format("%s%d", ID, i);
				//DictName.Format("%s%d", ImageID, i);

				unsigned char* outBuffer;
				int outW = 0x100, outH = 0x100;

				if (pImage[i])
				{
					VXL_Add(pImage[i], rect[i].X, rect[i].Y, rect[i].W, rect[i].H);
					CncImgFree(pImage[i]);
				}
				ppmfc::CString pKey;
				if (pTurretImage[i])
				{
					pKey.Format("%sX%d", ID, i);
					int turdeltaX = CINI::FAData->GetInteger("VehicleVoxelTurretsRA2", pKey);
					pKey.Format("%sY%d", ID, i);
					int turdeltaY = CINI::FAData->GetInteger("VehicleVoxelTurretsRA2", pKey);
					VXL_Add(pTurretImage[i], turretrect[i].X + turdeltaX, turretrect[i].Y + turdeltaY, turretrect[i].W, turretrect[i].H);
					CncImgFree(pTurretImage[i]);

					if (pBarrelImage[i])
					{
						pKey.Format("%sX%d", ID, i);
						int barldeltaX = CINI::FAData->GetInteger("VehicleVoxelBarrelsRA2", pKey);
						pKey.Format("%sY%d", ID, i);
						int barldeltaY = CINI::FAData->GetInteger("VehicleVoxelBarrelsRA2", pKey);

						VXL_Add(pBarrelImage[i], barrelrect[i].X + barldeltaX, barrelrect[i].Y + barldeltaY, barrelrect[i].W, barrelrect[i].H);
						CncImgFree(pBarrelImage[i]);
					}
				}

				VXL_GetAndClear(outBuffer, &outW, &outH);

				SetImageData(outBuffer, DictName, outW, outH, PalettesManager::LoadPalette(PaletteName));
			}
		}
		else
		{
			for (int i = 0; i < 8; ++i)
			{
				ppmfc::CString DictName;
				DictName.Format("%s%d", ID, i);
				// DictName.Format("%s%d", ImageID, i);

				unsigned char* outBuffer;
				int outW = 0x100, outH = 0x100;

				VXL_Add(pImage[i], rect[i].X, rect[i].Y, rect[i].W, rect[i].H);
				delete[] pImage[i];
				VXL_GetAndClear(outBuffer, &outW, &outH);

				SetImageData(outBuffer, DictName, outW, outH, PalettesManager::LoadPalette(PaletteName));
			}
		}
		if (ExtConfigs::InGameDisplay_Shadow)
			for (int i = 0; i < 8; ++i)
			{
				ppmfc::CString DictShadowName;
				DictShadowName.Format("%s%d\233SHADOW", ID, i);

				unsigned char* outBuffer;
				int outW = 0x100, outH = 0x100;

				VXL_Add(pShadowImage[i], shadowrect[i].X, shadowrect[i].Y, shadowrect[i].W, shadowrect[i].H, true);
				delete[] pShadowImage[i];
				VXL_GetAndClear(outBuffer, &outW, &outH, true);

				SetImageData(outBuffer, DictShadowName, outW, outH, &CMapDataExt::Palette_Shadow);
			}
	}
	else // As SHP
	{
		int facingCount = CINI::Art->GetInteger(ArtID, "Facings", 8);
		if (facingCount % 8 != 0)
			facingCount = (facingCount + 7) / 8 * 8;
		int framesToRead[8];
		if (CINI::Art->KeyExists(ArtID, "StandingFrames"))
		{
			int nStartStandFrame = CINI::Art->GetInteger(ArtID, "StartStandFrame", 0);
			int nStandingFrames = CINI::Art->GetInteger(ArtID, "StandingFrames", 1);
			for (int i = 0; i < 8; ++i)
				framesToRead[i] = nStartStandFrame + (i * facingCount / 8) * nStandingFrames;
		}
		else
		{
			int nStartWalkFrame = CINI::Art->GetInteger(ArtID, "StartWalkFrame", 0);
			int nWalkFrames = CINI::Art->GetInteger(ArtID, "WalkFrames", 1);
			for (int i = 0; i < 8; ++i) {
				framesToRead[i] = nStartWalkFrame + (i * facingCount / 8) * nWalkFrames;
			}
		}

		std::rotate(framesToRead, framesToRead + 1, framesToRead + 8);

		ppmfc::CString FileName = ImageID + ".shp";
		int nMix = this->SearchFile(FileName);
		if (CLoading::HasFile(FileName, nMix))
		{
			ShapeHeader header;
			unsigned char* FramesBuffers[2];
			unsigned char* FramesBuffersShadow[2];
			CMixFile::LoadSHP(FileName, nMix);
			CShpFile::GetSHPHeader(&header);
			for (int i = 0; i < 8; ++i)
			{
				CLoadingExt::LoadSHPFrameSafe(framesToRead[i], 1, &FramesBuffers[0], header);
				ppmfc::CString DictName;
				DictName.Format("%s%d", ID, i);
				// DictName.Format("%s%d", ImageID, i);
				ppmfc::CString PaletteName = CINI::Art->GetString(ArtID, "Palette", "unit");
				GetFullPaletteName(PaletteName);
				
				if (bHasTurret)
				{
					int F, L, H;
					int s_count = sscanf_s(CINI::Art->GetString(ArtID, "TurretOffset", "0,0,0"), "%d,%d,%d", &F, &L, &H);
					if (s_count == 0) F = L = H = 0;
					else if (s_count == 1) L = H = 0;
					else if (s_count == 2) H = 0;

					int nStartWalkFrame = CINI::Art->GetInteger(ArtID, "StartWalkFrame", 0);
					int nWalkFrames = CINI::Art->GetInteger(ArtID, "WalkFrames", 1);
					int turretFrameToRead;
					
					// turret start from 0 + WalkFrames * Facings, ignore StartWalkFrame
					// and always has 32 facings
					turretFrameToRead = facingCount * nWalkFrames + ((1 + i) % 8) * 32 / 8;

					CLoadingExt::LoadSHPFrameSafe(turretFrameToRead, 1, &FramesBuffers[1], header);
					Matrix3D mat(F, L, H, i);

					UnionSHP_Add(FramesBuffers[0], header.Width, header.Height);
					UnionSHP_Add(FramesBuffers[1], header.Width, header.Height, mat.OutputX, mat.OutputY);
					unsigned char* outBuffer;
					int outW, outH;
					UnionSHP_GetAndClear(outBuffer, &outW, &outH);
					
					SetImageData(outBuffer, DictName, outW, outH, PalettesManager::LoadPalette(PaletteName));

					if (ExtConfigs::InGameDisplay_Shadow && bHasShadow)
					{
						ppmfc::CString DictNameShadow;
						DictNameShadow.Format("%s%d\233SHADOW", ID, i);
						CLoadingExt::LoadSHPFrameSafe(framesToRead[i] + header.FrameCount / 2, 1, &FramesBuffersShadow[0], header);
						CLoadingExt::LoadSHPFrameSafe(turretFrameToRead + header.FrameCount / 2, 1, &FramesBuffersShadow[1], header);
						UnionSHP_Add(FramesBuffersShadow[0], header.Width, header.Height, 0, 0, false, true);
						UnionSHP_Add(FramesBuffersShadow[1], header.Width, header.Height, mat.OutputX, mat.OutputY, false, true);
						unsigned char* outBufferShadow;
						int outWShadow, outHShadow;
						UnionSHP_GetAndClear(outBufferShadow, &outWShadow, &outHShadow, false, true);

						SetImageData(outBufferShadow, DictNameShadow, outWShadow, outHShadow, &CMapDataExt::Palette_Shadow);
					}
				}
				else
				{
					SetImageData(FramesBuffers[0], DictName, header.Width, header.Height, PalettesManager::LoadPalette(PaletteName));
					if (ExtConfigs::InGameDisplay_Shadow && bHasShadow)
					{
						ppmfc::CString DictNameShadow;
						DictNameShadow.Format("%s%d\233SHADOW", ID, i);
						CLoadingExt::LoadSHPFrameSafe(framesToRead[i] + header.FrameCount / 2, 1, &FramesBuffersShadow[0], header);
						SetImageData(FramesBuffersShadow[0], DictNameShadow, header.Width, header.Height, &CMapDataExt::Palette_Shadow);
					}
				}
			}
		}
	}

	//if (auto pAIFile = Variables::Rules.TryGetString(ID, "AlphaImage"))
	//{
	//	ppmfc::CString AIFile = *pAIFile;
	//	AIFile.Trim();
	//	if (!ImageDataMapHelper::IsImageLoaded(AIFile))
	//		LoadShp(AIFile + "\233ALPHAIMAGE", AIFile + ".shp", "anim.pal", 0);
	//}
}

void CLoadingExt::SetImageData(unsigned char* pBuffer, ppmfc::CString NameInDict, int FullWidth, int FullHeight, Palette* pPal)
{
	auto pData = ImageDataMapHelper::GetImageDataFromMap(NameInDict);
	SetImageData(pBuffer, pData, FullWidth, FullHeight, pPal);
}

void CLoadingExt::SetImageData(unsigned char* pBuffer, ImageDataClass* pData, int FullWidth, int FullHeight, Palette* pPal)
{
	if (pData->pImageBuffer)
		GameDeleteArray(pData->pImageBuffer, pData->FullWidth * pData->FullHeight);
	if (pData->pPixelValidRanges)
		GameDeleteArray(pData->pPixelValidRanges, pData->FullHeight);

	pData->pImageBuffer = pBuffer;
	pData->FullHeight = FullHeight;
	pData->FullWidth = FullWidth;
	SetValidBuffer(pData, FullWidth, FullHeight);

	// Get available area
	int counter = 0;
	int validFirstX = FullWidth - 1;
	int validFirstY = FullHeight - 1;
	int validLastX = 0;
	int validLastY = 0;
	for (int j = 0; j < FullHeight; ++j)
	{
		for (int i = 0; i < FullWidth; ++i)
		{
			unsigned char ch = pBuffer[counter++];
			if (ch != 0)
			{
				if (i < validFirstX)
					validFirstX = i;
				if (j < validFirstY)
					validFirstY = j;
				if (i > validLastX)
					validLastX = i;
				if (j > validLastY)
					validLastY = j;
			}
		}
	}

	pData->ValidX = validFirstX;
	pData->ValidY = validFirstY;
	pData->ValidWidth = validLastX - validFirstX + 1;
	pData->ValidHeight = validLastY - validFirstY + 1;

	pData->Flag = ImageDataFlag::SHP;
	pData->IsOverlay = false;
	pData->pPalette = pPal ? pPal : Palette::PALETTE_UNIT;
}
// This function will shrink it to fit.
// Also will delete the origin buffer and create a new buffer.
void CLoadingExt::ShrinkSHP(unsigned char* pIn, int InWidth, int InHeight, unsigned char*& pOut, int* OutWidth, int* OutHeight)
{
	int counter = 0;
	int validFirstX = InWidth - 1;
	int validFirstY = InHeight - 1;
	int validLastX = 0;
	int validLastY = 0;
	for (int j = 0; j < InHeight; ++j)
	{
		for (int i = 0; i < InWidth; ++i)
		{
			unsigned char ch = pIn[counter++];
			if (ch != 0)
			{
				if (i < validFirstX)
					validFirstX = i;
				if (j < validFirstY)
					validFirstY = j;
				if (i > validLastX)
					validLastX = i;
				if (j > validLastY)
					validLastY = j;
			}
		}
	}

	counter = 0;
	*OutWidth = validLastX - validFirstX + 1;
	*OutHeight = validLastY - validFirstY + 1;
	pOut = GameCreateArray<unsigned char>(*OutWidth * *OutHeight);
	for (int j = 0; j < *OutHeight; ++j)
		memcpy_s(&pOut[j * *OutWidth], *OutWidth, &pIn[(j + validFirstY) * InWidth + validFirstX], *OutWidth);

	GameDeleteArray(pIn, InWidth * InHeight);
}

void CLoadingExt::UnionSHP_Add(unsigned char* pBuffer, int Width, int Height, int DeltaX, int DeltaY, bool UseTemp, bool bShadow)
{
	if (bShadow)
		UnionSHPShadow_Data[UseTemp].push_back(SHPUnionData{ pBuffer,Width,Height,DeltaX,DeltaY });
	else
		UnionSHP_Data[UseTemp].push_back(SHPUnionData{ pBuffer,Width,Height,DeltaX,DeltaY });
}

void CLoadingExt::UnionSHP_GetAndClear(unsigned char*& pOutBuffer, int* OutWidth, int* OutHeight, bool UseTemp, bool bShadow)
{
	// never calls it when UnionSHP_Data is empty
	auto& data = bShadow ? UnionSHPShadow_Data : UnionSHP_Data;
	if (data[UseTemp].size() == 1)
	{
		pOutBuffer = data[UseTemp][0].pBuffer;
		*OutWidth = data[UseTemp][0].Width;
		*OutHeight = data[UseTemp][0].Height;
		data[UseTemp].clear();
		return;
	}

	// For each shp, we make their center at the same point, this will give us proper result.
	int W = 0, H = 0;

	for (auto& data : data[UseTemp])
	{
		if (W < data.Width + 2 * abs(data.DeltaX)) W = data.Width + 2 * abs(data.DeltaX);
		if (H < data.Height + 2 * abs(data.DeltaY)) H = data.Height + 2 * abs(data.DeltaY);
	}

	// just make it work like unsigned char[W][H];
	pOutBuffer = GameCreateArray<unsigned char>(W * H);
	*OutWidth = W;
	*OutHeight = H;

	int ImageCenterX = W / 2;
	int ImageCenterY = H / 2;

	// Image[X][Y] <=> pOutBuffer[Y * W + X];
	for (auto& data : data[UseTemp])
	{
		int nStartX = ImageCenterX - data.Width / 2 + data.DeltaX;
		int nStartY = ImageCenterY - data.Height / 2 + data.DeltaY;

		for (int j = 0; j < data.Height; ++j)
			for (int i = 0; i < data.Width; ++i)
				if (auto nPalIdx = data.pBuffer[j * data.Width + i])
					pOutBuffer[(nStartY + j) * W + nStartX + i] = nPalIdx;

		GameDeleteArray(data.pBuffer, data.Width * data.Height);
	}

	data[UseTemp].clear();
}

void CLoadingExt::VXL_Add(unsigned char* pCache, int X, int Y, int Width, int Height, bool shadow)
{
	if (shadow)
	{
		for (int j = 0; j < Height; ++j)
			for (int i = 0; i < Width; ++i)
				if (auto ch = pCache[j * Width + i])
					VXL_Shadow_Data[(j + Y) * 0x100 + X + i] = ch;
	}
	else
	{
		for (int j = 0; j < Height; ++j)
			for (int i = 0; i < Width; ++i)
				if (auto ch = pCache[j * Width + i])
					VXL_Data[(j + Y) * 0x100 + X + i] = ch;
	}
}

void CLoadingExt::VXL_GetAndClear(unsigned char*& pBuffer, int* OutWidth, int* OutHeight, bool shadow)
{
	/* TODO : Save memory
	int validFirstX = 0x100 - 1;
	int validFirstY = 0x100 - 1;
	int validLastX = 0;
	int validLastY = 0;

	for (int j = 0; j < 0x100; ++j)
	{
		for (int i = 0; i < 0x100; ++i)
		{
			unsigned char ch = VXL_Data[j * 0x100 + i];
			if (ch != 0)
			{
				if (i < validFirstX)
					validFirstX = i;
				if (j < validFirstY)
					validFirstY = j;
				if (i > validLastX)
					validLastX = i;
				if (j > validLastY)
					validLastY = j;
			}
		}
	}
	*/
	if (shadow)
	{
		pBuffer = GameCreateArray<unsigned char>(0x10000);
		memcpy_s(pBuffer, 0x10000, VXL_Shadow_Data, 0x10000);
		memset(VXL_Shadow_Data, 0, 0x10000);
	}
	else
	{
		pBuffer = GameCreateArray<unsigned char>(0x10000);
		memcpy_s(pBuffer, 0x10000, VXL_Data, 0x10000);
		memset(VXL_Data, 0, 0x10000);
	}

}

void CLoadingExt::SetValidBuffer(ImageDataClass* pData, int Width, int Height)
{
	pData->pPixelValidRanges = GameCreateArray<ImageDataClass::ValidRangeData>(Height);
	for (int i = 0; i < Height; ++i)
	{
		int begin, end;
		this->GetSHPValidRange(pData->pImageBuffer, Width, i, &begin, &end);
		pData->pPixelValidRanges[i].First = begin;
		pData->pPixelValidRanges[i].Last = end;
	}
}

void CLoadingExt::GetFullPaletteName(ppmfc::CString& PaletteName)
{
	const int len = PaletteName.GetLength();
	if (len >= 4 &&
		PaletteName[len - 4] == '.' &&
		(PaletteName[len - 3] == 'p' || PaletteName[len - 3] == 'P') &&
		(PaletteName[len - 2] == 'a' || PaletteName[len - 2] == 'A') &&
		(PaletteName[len - 1] == 'l' || PaletteName[len - 1] == 'L'))
	{
		return;
	}

	switch (this->TheaterIdentifier)
	{
	case 'A':
		PaletteName += "sno.pal";
		return;
	case 'U':
		PaletteName += "urb.pal";
		return;
	case 'N':
		PaletteName += "ubn.pal";
		return;
	case 'D':
		PaletteName += "des.pal";
		return;
	case 'L':
		PaletteName += "lun.pal";
		return;
	case 'T':
	default:
		PaletteName += "tem.pal";
		return;
	}
}

int CLoadingExt::ColorDistance(const ColorStruct& color1, const ColorStruct& color2)
{
	int diffRed = color1.red - color2.red;
	int diffGreen = color1.green - color2.green;
	int diffBlue = color1.blue - color2.blue;
	return diffRed * diffRed + diffGreen * diffGreen + diffBlue * diffBlue;
}

std::vector<int> CLoadingExt::GeneratePalLookupTable(Palette* first, Palette* second)
{
	if (!first || !second) {
		std::vector<int> lookupTable;
		return lookupTable;
	}
	std::vector<int> lookupTable(256);

	lookupTable[0] = 0;
	for (int i = 1; i < 256; ++i) {
		int minDistance = std::numeric_limits<int>::max();
		int bestMatchIndex = 0;

		for (int j = 1; j < 256; ++j) {
			if (j >= 16 && j < 32) { // skip house colors
				continue;
			}
			int distance = ColorDistance(first->GetByteColor(i), second->GetByteColor(j));
			if (distance < minDistance) {
				minDistance = distance;
				bestMatchIndex = j;
			}
		}

		lookupTable[i] = bestMatchIndex;
	}

	return lookupTable;
}

void CLoadingExt::LoadSHPFrameSafe(int nFrame, int nFrameCount, unsigned char** ppBuffer, const ShapeHeader& header)
{
	if (nFrame < 0 || nFrame + nFrameCount - 1 >= header.FrameCount)
	{
		GameAllocator<BYTE> alloc;
		using AllocTraits = std::allocator_traits<GameAllocator<BYTE>>;
		// write an empty image
		for (int pic = 0; pic < nFrameCount; pic++) {
			size_t capacity = static_cast<size_t>(header.Width) * static_cast<size_t>(header.Height);
			if (header.Width > 0 && header.Height > 0 && capacity > 0) {
				ppBuffer[pic] = AllocTraits::allocate(alloc, capacity);
				std::fill_n(ppBuffer[pic], capacity, 0);
			}
			else {
				ppBuffer[pic] = nullptr;
			}
		}
		return;
	}

	CShpFile::LoadFrame(nFrame, nFrameCount, ppBuffer);
}

void CLoadingExt::LoadShp(ppmfc::CString ImageID, ppmfc::CString FileName, ppmfc::CString PalName, int nFrame)
{
	auto loadingExt = (CLoadingExt*)CLoading::Instance();
	loadingExt->GetFullPaletteName(PalName);
	if (auto pal = PalettesManager::LoadPalette(PalName))
	{
		int nMix = loadingExt->SearchFile(FileName);
		if (loadingExt->HasFile(FileName, nMix))
		{
			ShapeHeader header;
			unsigned char* FramesBuffers;
			CMixFile::LoadSHP(FileName, nMix);
			CShpFile::GetSHPHeader(&header);
			CLoadingExt::LoadSHPFrameSafe(nFrame, 1, &FramesBuffers, header);
			loadingExt->SetImageData(FramesBuffers, ImageID, header.Width, header.Height, pal);
		}
	}
}

void CLoadingExt::LoadShpToBitmap(ppmfc::CString ImageID, ppmfc::CString FileName, ppmfc::CString PalName, int nFrame)
{
	auto loadingExt = (CLoadingExt*)CLoading::Instance();
	loadingExt->GetFullPaletteName(PalName);
	if (auto pal = PalettesManager::LoadPalette(PalName))
	{
		int nMix = loadingExt->SearchFile(FileName);
		if (loadingExt->HasFile(FileName, nMix))
		{
			ShapeHeader header;
			unsigned char* FramesBuffers;
			CMixFile::LoadSHP(FileName, nMix);
			CShpFile::GetSHPHeader(&header);
			CLoadingExt::LoadSHPFrameSafe(nFrame, 1, &FramesBuffers, header);

			CBitmap bitmap;
			if (bitmap.CreateBitmap(header.Width, header.Height, 1, 32, NULL))
			{
				CDC memDC;
				memDC.CreateCompatibleDC(NULL);
				CBitmap* pOldBitmap = memDC.SelectObject(&bitmap);

				LOGPALETTE* pLogPalette = (LOGPALETTE*)malloc(sizeof(LOGPALETTE) + 256 * sizeof(PALETTEENTRY));
				pLogPalette->palVersion = 0x300;
				pLogPalette->palNumEntries = 256;

				for (int i = 0; i < 256; i++)
				{
					pLogPalette->palPalEntry[i].peRed = pal->Data[i].R;
					pLogPalette->palPalEntry[i].peGreen = pal->Data[i].G;
					pLogPalette->palPalEntry[i].peBlue = pal->Data[i].B;
					pLogPalette->palPalEntry[i].peFlags = pal->Data[i].Zero;
				}
				CPalette paletteObj;
				paletteObj.CreatePalette(pLogPalette);
				free(pLogPalette);
				CPalette* pOldPalette = memDC.SelectPalette(&paletteObj, FALSE);
				memDC.RealizePalette();
				for (int y = 0; y < header.Height; y++)
				{
					for (int x = 0; x < header.Width; x++)
					{
						memDC.SetPixel(x, y, PALETTEINDEX(FramesBuffers[y * header.Width + x]));
					}
				}
				memDC.SelectPalette(pOldPalette, FALSE);
				memDC.SelectObject(pOldBitmap);

				auto pIsoView = reinterpret_cast<CFinalSunDlg*>(CFinalSunApp::Instance->m_pMainWnd)->MyViewFrame.pIsoView;
				auto pData = ImageDataMapHelper::GetImageDataFromMap(ImageID);
				pData->lpSurface = CIsoViewExt::BitmapToSurface(pIsoView->lpDD7, bitmap);

				DDSURFACEDESC2 desc;
				memset(&desc, 0, sizeof(DDSURFACEDESC2));
				desc.dwSize = sizeof(DDSURFACEDESC2);
				desc.dwFlags = DDSD_HEIGHT | DDSD_WIDTH;
				pData->lpSurface->GetSurfaceDesc(&desc);
				pData->ValidHeight = desc.dwHeight;
				pData->ValidWidth = desc.dwWidth;
				pData->FullWidth = desc.dwWidth;
				pData->FullHeight = desc.dwHeight;
				pData->Flag = ImageDataFlag::SurfaceData;

				CIsoView::SetColorKey(pData->lpSurface, -1);
			}	
		}
	}
}

void CLoadingExt::LoadShpToBitmap(ppmfc::CString ImageID, unsigned char* pBuffer, int Width, int Height, Palette* pPal)
{
	auto loadingExt = (CLoadingExt*)CLoading::Instance();
	CBitmap bitmap;
	if (bitmap.CreateBitmap(Width, Height, 1, 32, NULL))
	{
		CDC memDC;
		memDC.CreateCompatibleDC(NULL);
		CBitmap* pOldBitmap = memDC.SelectObject(&bitmap);

		LOGPALETTE* pLogPalette = (LOGPALETTE*)malloc(sizeof(LOGPALETTE) + 256 * sizeof(PALETTEENTRY));
		pLogPalette->palVersion = 0x300;
		pLogPalette->palNumEntries = 256;

		for (int i = 0; i < 256; i++)
		{
			pLogPalette->palPalEntry[i].peRed = pPal->Data[i].R;
			pLogPalette->palPalEntry[i].peGreen = pPal->Data[i].G;
			pLogPalette->palPalEntry[i].peBlue = pPal->Data[i].B;
			pLogPalette->palPalEntry[i].peFlags = pPal->Data[i].Zero;
		}
		CPalette paletteObj;
		paletteObj.CreatePalette(pLogPalette);
		free(pLogPalette);
		CPalette* pOldPalette = memDC.SelectPalette(&paletteObj, FALSE);
		memDC.RealizePalette();
		for (int y = 0; y < Height; y++)
		{
			for (int x = 0; x < Width; x++)
			{
				memDC.SetPixel(x, y, PALETTEINDEX(pBuffer[y * Width + x]));
			}
		}
		memDC.SelectPalette(pOldPalette, FALSE);
		memDC.SelectObject(pOldBitmap);

		auto pIsoView = reinterpret_cast<CFinalSunDlg*>(CFinalSunApp::Instance->m_pMainWnd)->MyViewFrame.pIsoView;
		auto pData = ImageDataMapHelper::GetImageDataFromMap(ImageID);
		pData->lpSurface = CIsoViewExt::BitmapToSurface(pIsoView->lpDD7, bitmap);

		DDSURFACEDESC2 desc;
		memset(&desc, 0, sizeof(DDSURFACEDESC2));
		desc.dwSize = sizeof(DDSURFACEDESC2);
		desc.dwFlags = DDSD_HEIGHT | DDSD_WIDTH;
		pData->lpSurface->GetSurfaceDesc(&desc);
		pData->ValidHeight = desc.dwHeight;
		pData->ValidWidth = desc.dwWidth;
		pData->FullWidth = desc.dwWidth;
		pData->FullHeight = desc.dwHeight;
		pData->Flag = ImageDataFlag::SurfaceData;

		CIsoView::SetColorKey(pData->lpSurface, -1);
	}	
}