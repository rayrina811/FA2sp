#include "Body.h"

#include <CFinalSunApp.h>
#include <CMixFile.h>
#include <CLoading.h>
#include <CINI.h>
#include <Drawing.h>
#include <string>
#include "..\..\Miscs\Palettes.h"
#include "..\CMapData\Body.h"
#include "../CIsoView/Body.h"

DEFINE_HOOK(486B00, CLoading_InitMixFiles, 7)
{
	GET(CLoadingExt*, pThis, ECX);

	bool result = pThis->InitMixFilesFix();
	R->EAX(result);

	return 0x48A26A;
}

DEFINE_HOOK(48FDA0, CLoading_LoadOverlayGraphic_NewTheaterFix, 8)
{
	GET(int, hMix, ESI);
	GET(CLoadingExt*, pThis, ECX);
	GET_STACK(const char*, lpBuffer, STACK_OFFS(0x1C4, 0x1B4));
	GET_STACK(const char*, artID, STACK_OFFS(0x1C4, 0x1AC));
	if (hMix != NULL) 
		return 0x48FE68;
	ppmfc::CString filename = lpBuffer;
	hMix = pThis->SearchFile(filename);
	if (hMix == NULL) 
		hMix = 1919810;
	bool findFile = false;
	findFile = pThis->HasFile(filename);

	if (CINI::Art->GetBool(artID, "NewTheater") && strlen(artID) >= 2)
	{
		auto searchNewTheater = [&](char t)
			{
				if (!findFile)
				{
					filename.SetAt(1, t);
					hMix = pThis->SearchFile(filename);
					if (hMix == NULL)
						hMix = 1919810;
					findFile = pThis->HasFile(filename);
				}
			};
		searchNewTheater('G');
		searchNewTheater(artID[1]);
		if (!ExtConfigs::UseStrictNewTheater)
		{
			searchNewTheater('T');
			searchNewTheater('A');
			searchNewTheater('U');
			searchNewTheater('N');
			searchNewTheater('L');
			searchNewTheater('D');
		}
	}
	
	if (findFile)
	{
		R->ESI(hMix);
		R->Stack(STACK_OFFS(0x1C4, 0x1B4), filename);
		return 0x48FE68;
	}

	return 0x48FDA8;
}

DEFINE_HOOK(48A650, CLoading_SearchFile, 6)
{
	GET(CLoadingExt*, pThis, ECX);
	GET_STACK(const char*, Filename, 0x4);
	GET_STACK(unsigned char*, pTheaterType, 0x8);
	
	for (int i = 0; i < CMixFile::ArraySize; ++i)
	{
		auto& mix = CMixFile::Array[i];
		if (!mix.is_open())
			break;
		if (CMixFile::HasFile(Filename, i + 1))
		{
#ifndef NDEBUG
			Logger::Debug("[SearchFile] %s - %d\n", Filename, i + 1);
#endif
			R->EAX(i + 1);
			return 0x48AA63;
		}
	}

#ifndef NDEBUG
	Logger::Debug("[SearchFile] %s - NOT FOUND\n", Filename);
#endif
	R->EAX(0);
	return 0x48AA63;
}

DEFINE_HOOK(49DD64, CMapData_LoadMap_InitMixFiles_Removal, 5)
{
	CLoading::Instance->CSCBuiltby.~CStatic();
	CLoading::Instance->CSCLoading.~CStatic();
	CLoading::Instance->CSCVersion.~CStatic();
	CLoading::Instance->CPCProgress.~CProgressCtrl();
	struct CLoadingHelper
	{
		void DTOR()
		{
			JMP_STD(0x551A1D);
		}
	};
	reinterpret_cast<CLoadingHelper*>(CLoading::Instance())->DTOR();
	return 0x49DD74;
}

DEFINE_HOOK(4B8CFC, CMapData_CreateMap_InitMixFiles_Removal, 5)
{
	CLoading::Instance->CSCBuiltby.~CStatic();
	CLoading::Instance->CSCLoading.~CStatic();
	CLoading::Instance->CSCVersion.~CStatic();
	CLoading::Instance->CPCProgress.~CProgressCtrl();
	struct CLoadingHelper
	{
		void DTOR()
		{
			JMP_STD(0x551A1D);
		}
	};
	reinterpret_cast<CLoadingHelper*>(CLoading::Instance())->DTOR();
	return 0x4B8D0C;
}

DEFINE_HOOK(4903F3, CLoading_DrawOverlay_Palette, 7)
{
	GET(CLoadingExt*, pThis, EDI);
	REF_STACK(ImageDataClass, pDrawData, STACK_OFFS(0x1C8, 0xC8));
	GET_STACK(int, nOverlayIndex, STACK_OFFS(0x1C8, -0x8));

	if (!CMapData::Instance->MapWidthPlusHeight) {
		return 0;
	}

	if (nOverlayIndex >= 0 && nOverlayIndex < 256)
	{
		auto const& typeData = CMapDataExt::OverlayTypeDatas[nOverlayIndex];

		if (typeData.Wall)
		{
			auto palName = typeData.WallPaletteName;
			pThis->GetFullPaletteName(palName);
			if (auto pal = PalettesManager::LoadPalette(palName)) {
				BGRStruct houseColor{ 0,0,255 };
				auto lightingPal = PalettesManager::GetPalette(pal, houseColor);
				pDrawData.pPalette = lightingPal;
			}
		}
	}

	return 0;
}

DEFINE_HOOK(48E541, CLoading_InitTMPs_UpdateTileDatas, 7)
{
	auto thisTheater = CINI::CurrentDocument().GetString("Map", "Theater");
	if (thisTheater == "TEMPERATE")
	{
		CMapDataExt::TileData = CTileTypeInfo::Temperate().Datas;
		CMapDataExt::TileDataCount = CTileTypeInfo::Temperate().Count;
	}
	if (thisTheater == "SNOW")
	{
		CMapDataExt::TileData = CTileTypeInfo::Snow().Datas;
		CMapDataExt::TileDataCount = CTileTypeInfo::Snow().Count;
	}
	if (thisTheater == "URBAN")
	{
		CMapDataExt::TileData = CTileTypeInfo::Urban().Datas;
		CMapDataExt::TileDataCount = CTileTypeInfo::Urban().Count;
	}
	if (thisTheater == "NEWURBAN")
	{
		CMapDataExt::TileData = CTileTypeInfo::NewUrban().Datas;
		CMapDataExt::TileDataCount = CTileTypeInfo::NewUrban().Count;
	}
	if (thisTheater == "LUNAR")
	{
		CMapDataExt::TileData = CTileTypeInfo::Lunar().Datas;
		CMapDataExt::TileDataCount = CTileTypeInfo::Lunar().Count;
	}
	if (thisTheater == "DESERT")
	{
		CMapDataExt::TileData = CTileTypeInfo::Desert().Datas;
		CMapDataExt::TileDataCount = CTileTypeInfo::Desert().Count;
	}

	return 0;
}

DEFINE_HOOK(48E970, CLoading_LoadTile_SkipTranspInsideCheck, 6)
{
	return 0x48EA44;
}

static bool DrawTranspInsideTilesChanged = false;
DEFINE_HOOK(4F36DD, CTileSetBrowserView_RenderTile_DrawTranspInsideTiles, 5)
{
	GET(unsigned int, tileIndex, EBX);
	tileIndex = CMapDataExt::GetSafeTileIndex(tileIndex);
	if (0 <= tileIndex && tileIndex < CMapDataExt::TileDataCount)
	{
		if (CMapDataExt::TileSetCumstomPalette[CMapDataExt::TileData[tileIndex].TileSet])
		{
			ppmfc::CString section;
			section.Format("TileSet%04d", CMapDataExt::TileData[tileIndex].TileSet);
			auto customPal = CINI::CurrentTheater->GetString(section, "CustomPalette");
			DrawTranspInsideTilesChanged = true;
			memcpy(&CLoadingExt::TempISOPalette, Palette::PALETTE_ISO, sizeof(Palette));
			BGRStruct empty;
			if (auto pPal = PalettesManager::LoadPalette(customPal))
				memcpy(Palette::PALETTE_ISO, PalettesManager::GetPalette(pPal, empty, false), sizeof(Palette));
		}
	}
	return 0;
}

DEFINE_HOOK(4F35CE, CTileSetBrowserView_RenderTile_DrawTranspInsideTiles_2, 6)
{
	if (DrawTranspInsideTilesChanged)
	{
		DrawTranspInsideTilesChanged = false;
		memcpy(Palette::PALETTE_ISO, &CLoadingExt::TempISOPalette, sizeof(Palette));
	}
	return 0;
}

DEFINE_HOOK(47AB50, CLoading_InitPics_LoadDLLBitmaps, 7)
{
	HBITMAP hBmp = (HBITMAP)LoadImage(static_cast<HINSTANCE>(FA2sp::hInstance), MAKEINTRESOURCE(1001),
		IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	CBitmap cBitmap;
	cBitmap.Attach(hBmp);
	CLoadingExt::LoadBitMap("FA2spAnnotation", cBitmap);

	return 0;
}
