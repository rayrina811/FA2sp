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
	if (hMix != NULL) return 0x48FE68;

	ppmfc::CString filename = lpBuffer;
	filename.SetAt(1, 'G');
	hMix = pThis->SearchFile(filename);
	if (hMix != NULL)
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
				BGRStruct empty;
				auto lightingPal = PalettesManager::GetPalette(pal, empty);
				pDrawData.pPalette = lightingPal;
			}
		}
	}

	return 0;
}

DEFINE_HOOK(48EE60, CLoading_LoadOverlayGraphic_RecordHistory, 7)
{
	GET_STACK(const char*, overlayID, 0x4);
	auto it = std::find(CLoadingExt::LoadedOverlays.begin(), CLoadingExt::LoadedOverlays.end(), overlayID);
	if (it == CLoadingExt::LoadedOverlays.end()) {
		CLoadingExt::LoadedOverlays.push_back(overlayID);
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

DEFINE_HOOK(469410, CLoading_ReInitializeDDraw_ReloadFA2SPHESettings, 6)
{
	auto currentLighting = CFinalSunDlgExt::CurrentLighting;
	CMapDataExt::InitializeAllHdmEdition(false);
	CFinalSunDlgExt::CurrentLighting = currentLighting;
	if (CFinalSunDlgExt::CurrentLighting != 31000)
	{
		CheckMenuRadioItem(*CFinalSunDlg::Instance->GetMenu(), 31000, 31003, CFinalSunDlgExt::CurrentLighting, MF_UNCHECKED);
		PalettesManager::ManualReloadTMP = true;
		PalettesManager::CacheAndTintCurrentIso();
		CLoading::Instance->FreeTMPs();
		CLoading::Instance->InitTMPs();
		int oli = 0;
		for (const auto& ol : Variables::GetRulesMapSection("OverlayTypes"))
		{
			auto it = std::find(CLoadingExt::LoadedOverlays.begin(), CLoadingExt::LoadedOverlays.end(), ol.second);
			if (it != CLoadingExt::LoadedOverlays.end()) {
				CLoading::Instance->DrawOverlay(ol.second, oli);
			}
			oli++;
		}
		PalettesManager::RestoreCurrentIso();
		PalettesManager::ManualReloadTMP = false;

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
DEFINE_HOOK(48C3FB, CLoading_InitTMPs_StoreISOPal, 7)
{
	memcpy(&CLoadingExt::TempISOPalette, Palette::PALETTE_ISO, sizeof(Palette));
	return 0;
}

DEFINE_HOOK(48CBEB, CLoading_InitTMPs_CustomPalSupport, 7)
{
	GET_STACK(int, iTileSet, STACK_OFFS(0x59C, 0x578));
	ppmfc::CString section;
	section.Format("TileSet%04d", iTileSet);
	ppmfc::CString pDefault = "iso~~~.pal";
	pDefault.Replace("~~~", CLoading::Instance->GetTheaterSuffix());
	auto customPal = CINI::CurrentTheater->GetString(section, "CustomPalette", pDefault);
	if (customPal != pDefault)
	{
		BGRStruct empty;
		auto pPal = PalettesManager::LoadPalette(customPal);
		memcpy(Palette::PALETTE_ISO, PalettesManager::GetPalette(pPal, empty, false), sizeof(Palette));
	}
	else
	{
		memcpy(Palette::PALETTE_ISO, &CLoadingExt::TempISOPalette, sizeof(Palette));
	}
	return 0;
}

static bool DrawTranspInsideTilesChanged = false;
DEFINE_HOOK_AGAIN(46F767, CIsoView_DrawMap_DrawTranspInsideTiles, 8)
DEFINE_HOOK(46EC1B, CIsoView_DrawMap_DrawTranspInsideTiles, 8)
{
	GET_STACK(unsigned short, tileIndex, STACK_OFFS(0x0D18, 0xC32));
	tileIndex = CMapDataExt::GetSafeTileIndex(tileIndex);
	if (0 <= tileIndex && tileIndex < CMapDataExt::TileDataCount)
	{
		ppmfc::CString section;
		section.Format("TileSet%04d", CMapDataExt::TileData[tileIndex].TileSet);
		ppmfc::CString pDefault = "iso~~~.pal";
		pDefault.Replace("~~~", CLoading::Instance->GetTheaterSuffix());
		auto customPal = CINI::CurrentTheater->GetString(section, "CustomPalette", pDefault);
		if (customPal != pDefault)
		{
			DrawTranspInsideTilesChanged = true;
			memcpy(&CLoadingExt::TempISOPalette, Palette::PALETTE_ISO, sizeof(Palette));
			BGRStruct empty;
			auto pPal = PalettesManager::LoadPalette(customPal);
			memcpy(Palette::PALETTE_ISO, PalettesManager::GetPalette(pPal, empty, false), sizeof(Palette));
		}
	}
	return 0;
}

DEFINE_HOOK_AGAIN(4700BA, CIsoView_DrawMap_DrawTranspInsideTiles_2, 7)
DEFINE_HOOK(46F5AF, CIsoView_DrawMap_DrawTranspInsideTiles_2, 7)
{
	if (DrawTranspInsideTilesChanged)
	{
		DrawTranspInsideTilesChanged = false;
		memcpy(Palette::PALETTE_ISO, &CLoadingExt::TempISOPalette, sizeof(Palette));
	}
	return 0;
}

DEFINE_HOOK(4F36A0, CTileSetBrowserView_RenderTile_DrawTranspInsideTiles, 5)
{
	GET_STACK(unsigned int, tileIndex, 0x4);
	tileIndex = CMapDataExt::GetSafeTileIndex(tileIndex);
	if (0 <= tileIndex && tileIndex < CMapDataExt::TileDataCount)
	{
		ppmfc::CString section;
		section.Format("TileSet%04d", CMapDataExt::TileData[tileIndex].TileSet);
		ppmfc::CString pDefault = "iso~~~.pal";
		pDefault.Replace("~~~", CLoading::Instance->GetTheaterSuffix());
		auto customPal = CINI::CurrentTheater->GetString(section, "CustomPalette", pDefault);
		if (customPal != pDefault)
		{
			DrawTranspInsideTilesChanged = true;
			memcpy(&CLoadingExt::TempISOPalette, Palette::PALETTE_ISO, sizeof(Palette));
			BGRStruct empty;
			auto pPal = PalettesManager::LoadPalette(customPal);
			memcpy(Palette::PALETTE_ISO, PalettesManager::GetPalette(pPal, empty, false), sizeof(Palette));
		}
	}
	return 0;
}

DEFINE_HOOK(4F3BE2, CTileSetBrowserView_RenderTile_DrawTranspInsideTiles_2, 5)
{
	if (DrawTranspInsideTilesChanged)
	{
		DrawTranspInsideTilesChanged = false;
		memcpy(Palette::PALETTE_ISO, &CLoadingExt::TempISOPalette, sizeof(Palette));
	}
	return 0;
}

//
//DEFINE_HOOK(48E970, CLoading_LoadTile_SkipTranspInsideCheck, 6)
//{
//	return 0x48E976;
//}